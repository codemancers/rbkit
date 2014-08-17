//
//  rbkit.c
//  gather_stats
//
//  Created by Hemant Kumar on 26/04/14.
//  Copyright (c) 2014 Codemancers. All rights reserved.
//

#include "rbkit_tracer.h"
#include "rbkit_object_graph.h"
#include "rbkit_message_aggregator.h"
#include <sys/time.h>

static const char *event_names[] = {
  "gc_start",
  "gc_end_m",
  "gc_end_s",
  "obj_created",
  "obj_destroyed"
};

static struct gc_hooks *logger;
static int tmp_keep_remains;
static void *zmq_publisher;
static void *zmq_context;
static void *zmq_response_socket;
static zmq_pollitem_t items[1];

void pack_event_header(msgpack_packer* packer, const char *event_type, int map_size)
{
  msgpack_pack_map(packer, map_size);
  pack_string(packer, "event_type");
  pack_string(packer, event_type);

  pack_string(packer, "timestamp");
  pack_timestamp(packer);
}


static void trace_gc_invocation(void *data, int event_index) {
  if (event_index == 0) {
    msgpack_sbuffer_clear(logger->sbuf);
    pack_event_header(logger->msgpacker, event_names[event_index], 2);
    add_message(logger->sbuf);
  } else if (event_index == 2) {
    msgpack_sbuffer_clear(logger->sbuf);
    pack_event_header(logger->msgpacker, event_names[event_index], 2);
    add_message(logger->sbuf);
  }
}

static struct gc_hooks * get_trace_logger() {
  int i = 0;
  if (logger == 0) {
    logger = ALLOC_N(struct gc_hooks, 1);
    logger->enabled = Qfalse;
    logger->newobj_trace = 0;
    logger->freeobj_trace = 0;
    logger->keep_remains = tmp_keep_remains;
    logger->object_table = st_init_numtable();
    logger->str_table = st_init_strtable();

    for (i = 0; i < 3; i++) {
      logger->funcs[i] = trace_gc_invocation;
      logger->args[i] = (void *)event_names[i];
    }
    logger->sbuf = msgpack_sbuffer_new();
    logger->msgpacker = msgpack_packer_new(logger->sbuf, msgpack_sbuffer_write);
    logger->data = 0;
  }
  return logger;
}


static void
gc_start_i(VALUE tpval, void *data)
{
  struct gc_hooks *hooks = (struct gc_hooks *)data;
  (*hooks->funcs[0])(hooks->args[0], 0);
}

static void
gc_end_mark_i(VALUE tpval, void *data)
{
  struct gc_hooks *hooks = (struct gc_hooks *)data;
  (*hooks->funcs[1])(hooks->args[1], 1);
}

static void
gc_end_sweep_i(VALUE tpval, void *data)
{
  struct gc_hooks *hooks = (struct gc_hooks *)data;
  (*hooks->funcs[2])(hooks->args[2], 2);
}

static void
create_gc_hooks(void)
{
  int i;
  logger->hooks[0] =
    rb_tracepoint_new(0, RUBY_INTERNAL_EVENT_GC_START,     gc_start_i,     logger);
  logger->hooks[1] =
    rb_tracepoint_new(0, RUBY_INTERNAL_EVENT_GC_END_MARK,  gc_end_mark_i,  logger);
  logger->hooks[2] =
    rb_tracepoint_new(0, RUBY_INTERNAL_EVENT_GC_END_SWEEP, gc_end_sweep_i, logger);
  /* mark for GC */
  for (i=0; i<3; i++) rb_gc_register_mark_object(logger->hooks[i]);
}

void pack_pointer(msgpack_packer *packer, VALUE object_id) {
  char *object_string;
  asprintf(&object_string, "%p", object_id);
  pack_string(packer, object_string);
  free(object_string);
}
/*
 * make_unique_str helps to reuse memory by allocating memory for a string
 * only once and keeping track of how many times that string is referenced.
 * It does so by creating a map of strings to their no of references.
 * A new map is created for a string on its first use, and for further usages
 * the reference count is incremented.
 */
static const char * make_unique_str(st_table *tbl, const char *str, long len) {
  if (!str) {
    return NULL;
  }
  else {
    st_data_t n;
    char *result;

    if (st_lookup(tbl, (st_data_t)str, &n)) {
      st_insert(tbl, (st_data_t)str, n+1);
      st_get_key(tbl, (st_data_t)str, (st_data_t *)&result);
    }
    else {
      result = (char *)ruby_xmalloc(len+1);
      strncpy(result, str, len);
      result[len] = 0;
      st_add_direct(tbl, (st_data_t)result, 1);
    }
    return result;
  }
}

/*
 * Used to free allocation of string when it's not referenced anymore.
 * Decrements the reference count of a string if it's still used, else
 * the map is removed completely.
 */
static void delete_unique_str(st_table *tbl, const char *str) {
  if (str) {
    st_data_t n;

    st_lookup(tbl, (st_data_t)str, &n);
    if (n == 1) {
      st_delete(tbl, (st_data_t *)&str, 0);
      ruby_xfree((char *)str);
    }
    else {
      st_insert(tbl, (st_data_t)str, n-1);
    }
  }
}

// Refer Ruby source ext/objspace/object_tracing.c::newobj_i
static void newobj_i(VALUE tpval, void *data) {
  struct gc_hooks * arg = (struct gc_hooks *)data;
  rb_trace_arg_t *tparg = rb_tracearg_from_tracepoint(tpval);
  VALUE obj = rb_tracearg_object(tparg);
  VALUE klass = RBASIC_CLASS(obj);
  VALUE path = rb_tracearg_path(tparg);
  VALUE line = rb_tracearg_lineno(tparg);
  VALUE method_id = rb_tracearg_method_id(tparg);
  VALUE defined_klass = rb_tracearg_defined_class(tparg);

  struct allocation_info *info;
  const char *path_cstr = RTEST(path) ? make_unique_str(arg->str_table, RSTRING_PTR(path), RSTRING_LEN(path)) : 0;
  VALUE class_path = (RTEST(defined_klass) && !OBJ_FROZEN(defined_klass)) ? rb_class_path_cached(defined_klass) : Qnil;
  const char *class_path_cstr = RTEST(class_path) ? make_unique_str(arg->str_table, RSTRING_PTR(class_path), RSTRING_LEN(class_path)) : 0;

  if (st_lookup(arg->object_table, (st_data_t)obj, (st_data_t *)&info)) {
    /* reuse info */
    delete_unique_str(arg->str_table, info->path);
    delete_unique_str(arg->str_table, info->class_path);
  }
  else {
    info = (struct allocation_info *)ruby_xmalloc(sizeof(struct allocation_info));
  }

  info->path = path_cstr;
  info->line = NUM2INT(line);
  info->method_id = method_id;
  info->class_path = class_path_cstr;
  info->generation = rb_gc_count();
  st_insert(arg->object_table, (st_data_t)obj, (st_data_t)info);

  msgpack_sbuffer_clear(arg->sbuf);
  pack_event_header(arg->msgpacker, event_names[3], 3);
  pack_string(arg->msgpacker, "payload");
  msgpack_pack_map(arg->msgpacker, 2);
  pack_string(arg->msgpacker, "object_id");
  pack_pointer(arg->msgpacker, obj);
  pack_string(arg->msgpacker, "class");
  if (!NIL_P(klass) && BUILTIN_TYPE(obj) != T_NONE && BUILTIN_TYPE(obj) != T_ZOMBIE && BUILTIN_TYPE(obj) != T_ICLASS) {
    pack_string(arg->msgpacker, rb_class2name(klass));

  } else {
    msgpack_pack_nil(arg->msgpacker);
  }
  add_message(arg->sbuf);
}

// Refer Ruby source ext/objspace/object_tracing.c::freeobj_i
static void freeobj_i(VALUE tpval, void *data) {
  rb_trace_arg_t *tparg = rb_tracearg_from_tracepoint(tpval);
  VALUE obj = rb_tracearg_object(tparg);

  struct gc_hooks *arg = (struct gc_hooks *)data;
  struct allocation_info *info;

  if (st_lookup(arg->object_table, (st_data_t)obj, (st_data_t *)&info)) {
    st_delete(arg->object_table, (st_data_t *)&obj, (st_data_t *)&info);
    delete_unique_str(arg->str_table, info->path);
    delete_unique_str(arg->str_table, info->class_path);
    ruby_xfree(info);
  }

  msgpack_sbuffer_clear(logger->sbuf);
  pack_event_header(logger->msgpacker, event_names[4], 3);
  pack_string(logger->msgpacker, "payload");
  msgpack_pack_map(logger->msgpacker, 1);
  pack_string(logger->msgpacker, "object_id");
  pack_pointer(logger->msgpacker, obj);
  add_message(logger->sbuf);
}

static VALUE start_stat_server(int argc, VALUE *argv, VALUE self) {
  int default_pub_port = 5555;
  int default_request_port = 5556;
  VALUE pub_port;
  VALUE request_port;
  int bind_result;

  rb_scan_args(argc, argv, "02", &pub_port, &request_port);
  if (!NIL_P(pub_port)) {
    default_pub_port = FIX2INT(pub_port);
    if (default_pub_port < 1024 || default_pub_port > 65000)
      rb_raise(rb_eArgError, "invalid port value");
  }

  if (!NIL_P(request_port)) {
    default_request_port = FIX2INT(request_port);
    if(default_request_port < 1024 || default_request_port > 65000)
      rb_raise(rb_eArgError, "invalid port value");
  }

  // Creates a list which aggregates messages
  message_list_new();
  logger = get_trace_logger();
  logger->newobj_trace = rb_tracepoint_new(0, RUBY_INTERNAL_EVENT_NEWOBJ, newobj_i, logger);
  logger->freeobj_trace = rb_tracepoint_new(0, RUBY_INTERNAL_EVENT_FREEOBJ, freeobj_i, logger);
  rb_gc_register_mark_object(logger->newobj_trace);
  rb_gc_register_mark_object(logger->freeobj_trace);
  create_gc_hooks();

  char zmq_endpoint[14];
  sprintf(zmq_endpoint, "tcp://*:%d", default_pub_port);

  zmq_context = zmq_ctx_new();
  zmq_publisher = zmq_socket(zmq_context, ZMQ_PUB);
  bind_result = zmq_bind(zmq_publisher, zmq_endpoint);
  assert(bind_result == 0);

  char zmq_request_endpoint[14];
  sprintf(zmq_request_endpoint, "tcp://*:%d", default_request_port);

  zmq_response_socket = zmq_socket(zmq_context, ZMQ_REP);
  bind_result = zmq_bind(zmq_response_socket, zmq_request_endpoint);
  assert(bind_result == 0);

  items[0].socket = zmq_response_socket;
  items[0].events = ZMQ_POLLIN;
  return Qnil;
}

char * tracer_string_recv(void *socket) {
  zmq_msg_t msg;
  int rc = zmq_msg_init(&msg);
  assert(rc == 0);

  rc = zmq_msg_recv(&msg, socket, 0);
  assert(rc != -1);
  size_t message_size = zmq_msg_size(&msg);
  char *message = (char *)malloc(message_size +1);
  memcpy(message, zmq_msg_data(&msg), message_size);
  message[message_size] = 0;
  zmq_msg_close(&msg);
  return message;
}


int tracer_string_send(void *socket, const char *message) {
   int size = zmq_send (socket, message, strlen (message), 0);
   return size;
}

static VALUE poll_for_request() {
  // Wait for 100 millisecond and check if there is a message
  // we can't wait here indefenitely because ruby is not aware this is a 
  // blocking operation. Remember ruby releases GVL in a thread
  // whenever it encounters a known blocking operation.
  zmq_poll(items, 1, 100);
  if (items[0].revents && ZMQ_POLLIN) {
    char *message = tracer_string_recv(zmq_response_socket);
    tracer_string_send(zmq_response_socket, "ok");
    return rb_str_new_cstr(message);
  } else {
    return Qnil;
  }
}

static VALUE stop_stat_tracing() {
  if (logger->hooks[0] != 0) {
    rb_tracepoint_disable(logger->hooks[0]);
    rb_tracepoint_disable(logger->hooks[1]);
    rb_tracepoint_disable(logger->hooks[2]);
  }

  if (logger->newobj_trace) {
    rb_tracepoint_disable(logger->newobj_trace);
    rb_tracepoint_disable(logger->freeobj_trace);
  }
  logger->enabled = Qfalse;
  return Qnil;
}


static int free_keys_i(st_data_t key, st_data_t value, void *data) {
  ruby_xfree((void *)key);
  return ST_CONTINUE;
}

static int free_values_i(st_data_t key, st_data_t value, void *data) {
  ruby_xfree((void *)value);
  return ST_CONTINUE;
}

static VALUE stop_stat_server() {
  if (logger->enabled == Qtrue)
    stop_stat_tracing();

  // Destroy the list which aggregates messages
  message_list_destroy();
  // Clear object_table which holds object allocation info
  st_foreach(logger->object_table, free_values_i, 0);
  st_clear(logger->object_table);
  st_foreach(logger->str_table, free_keys_i, 0);
  st_clear(logger->str_table);

  msgpack_sbuffer_destroy(logger->sbuf);
  msgpack_packer_free(logger->msgpacker);
  zmq_close(zmq_publisher);
  zmq_close(zmq_response_socket);
  zmq_ctx_destroy(zmq_context);
  free(logger);
  return Qnil;
}

void pack_value_object(msgpack_packer *packer, VALUE value) {
  switch (TYPE(value)) {
    case T_FIXNUM:
      msgpack_pack_long(packer, FIX2LONG(value));
      break;
    case T_FLOAT:
      msgpack_pack_double(packer, rb_num2dbl(value));
      break;
    default:
      ;
      VALUE rubyString = rb_funcall(value, rb_intern("to_s"), 0, 0);
      char *keyString = StringValueCStr(rubyString);
      pack_string(packer, keyString);
      break;
  }
}

static int hash_iterator(VALUE key, VALUE value, VALUE hash_arg) {
  msgpack_packer *packer = (msgpack_packer *)hash_arg;

  // pack the key
  pack_value_object(packer,key);
  // pack the value
  pack_value_object(packer, value);
  return ST_CONTINUE;
}


void pack_string(msgpack_packer *packer, char *string) {
  if(string == NULL) {
    msgpack_pack_nil(packer);
  } else {
    int length = strlen(string);
    msgpack_pack_raw(packer, length);
    msgpack_pack_raw_body(packer, string, length);
  }
}

void pack_timestamp(msgpack_packer *packer) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    double time_in_milliseconds = (tv.tv_sec)*1000 + (tv.tv_usec)/1000;
    msgpack_pack_double(packer, time_in_milliseconds);
}

static VALUE send_hash_as_event(int argc, VALUE *argv, VALUE self) {
  VALUE hash_object;
  VALUE event_name;

  rb_scan_args(argc, argv, "20", &hash_object, &event_name);

  int size = RHASH_SIZE(hash_object);
  msgpack_sbuffer *buffer = msgpack_sbuffer_new();
  msgpack_packer *packer = msgpack_packer_new(buffer, msgpack_sbuffer_write);
  pack_event_header(packer, StringValueCStr(event_name), 3);

  pack_string(packer, "payload");
  msgpack_pack_map(packer, size);

  rb_hash_foreach(hash_object, hash_iterator, (VALUE)packer);
  add_message(buffer);
  msgpack_sbuffer_destroy(buffer);
  msgpack_packer_free(packer);
  return Qnil;
}

static VALUE start_stat_tracing() {
  if (logger->enabled == Qtrue)
    return Qnil;
  rb_tracepoint_enable(logger->newobj_trace);
  rb_tracepoint_enable(logger->freeobj_trace);
  int i = 0;
  for (i=0; i<3; i++) {
    rb_tracepoint_enable(logger->hooks[i]);
  }
  logger->enabled = Qtrue;
  return Qnil;
}

static VALUE send_objectspace_dump() {
  msgpack_sbuffer* buffer = msgpack_sbuffer_new();
  msgpack_packer* pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);

  struct ObjectDump * dump = get_object_dump(logger->object_table);
  pack_event_header(pk, "object_space_dump", 3);
  pack_string(pk, "payload");
  // Set size of array to hold all objects
  msgpack_pack_array(pk, dump->size);

  // Iterate through all object data
  struct ObjectData * data = dump->first ;
  for(;data != NULL; data = data->next ) {
    /* ObjectData is a map that looks like this :
     * {
     *   object_id: <OBJECT_ID_IN_HEX>,
     *   class: <CLASS_NAME>,
     *   references: [<OBJECT_ID_IN_HEX>, <OBJECT_ID_IN_HEX>, ...],
     *   file: <FILE_PATH>,
     *   line: <LINE_NO>,
     *   size: <SIZE>
     * }
     */


    msgpack_pack_map(pk, 6);

    // Key1 : "object_id"
    pack_string(pk, "object_id");

    // Value1 : pointer address of object
    char * object_id;
    asprintf(&object_id, "%p", data->object_id);
    pack_string(pk, object_id); 
    free(object_id);

    // Key2 : "class_name"
    pack_string(pk, "class_name");

    // Value2 : Class name of object
    if(data->class_name == NULL) {
      msgpack_pack_nil(pk);
    } else {
      pack_string(pk, data->class_name);
    }

    // Key3 : "references"
    pack_string(pk, "references");

    // Value3 : References held by the object
    msgpack_pack_array(pk, data->reference_count);
    if(data->reference_count != 0) {
      size_t count = 0;
      for(; count < data->reference_count; count++ ) {
        char * object_id;
        asprintf(&object_id, "%p", data->references[count]);
        pack_string(pk, object_id);
      }
      free(data->references);
    }

    // Key4 : "file"
    pack_string(pk, "file");

    // Value4 : File path where object is defined
    pack_string(pk, data->file);

    // Key5 : "line"
    pack_string(pk, "line");

    // Value5 : Line no where object is defined
    if(data->line == 0)
      msgpack_pack_nil(pk);
    else
      msgpack_pack_unsigned_long(pk, data->line);

    // Key6 : "size"
    pack_string(pk, "size");

    // Value6 : Size of the object in memory
    if(data->size == 0)
      msgpack_pack_nil(pk);
    else
      msgpack_pack_uint32(pk, data->size);

    free(data);
  }

  // Send packed message over zmq
  add_message(buffer);

  //Cleanup
  free(dump);
  msgpack_sbuffer_destroy(buffer);
  msgpack_packer_free(pk);

  return Qnil;
}

/*
 * Creates a msgpack array which contains all the messages packed after
 * the last time send_messages() was called, and sends it over the PUB socket.
 */
static VALUE send_messages() {
  //Get all aggregated messages as payload of a single event.
  msgpack_sbuffer * sbuf = (msgpack_sbuffer *)get_event_collection_message();
  //Send the msgpack array over zmq PUB socket
  if(sbuf && sbuf->size > 0)
    zmq_send(zmq_publisher, sbuf->data, sbuf->size, 0);
  // Clear the aggregated messages
  message_list_clear();
  msgpack_sbuffer_destroy(sbuf);
  return Qnil;
}

void Init_rbkit_tracer(void) {
  VALUE objectStatsModule = rb_define_module("Rbkit");
  rb_define_module_function(objectStatsModule, "start_stat_server", start_stat_server, -1);
  rb_define_module_function(objectStatsModule, "stop_stat_server", stop_stat_server, 0);
  rb_define_module_function(objectStatsModule, "start_stat_tracing", start_stat_tracing, 0);
  rb_define_module_function(objectStatsModule, "stop_stat_tracing", stop_stat_tracing, 0);
  rb_define_module_function(objectStatsModule, "poll_for_request", poll_for_request, 0);
  rb_define_module_function(objectStatsModule, "send_objectspace_dump", send_objectspace_dump, 0);
  rb_define_module_function(objectStatsModule, "send_hash_as_event", send_hash_as_event, -1);
  rb_define_module_function(objectStatsModule, "send_messages", send_messages, 0);
}
