//
//  rbkit.c
//  gather_stats
//
//  Created by Hemant Kumar on 26/04/14.
//  Copyright (c) 2014 Codemancers. All rights reserved.
//

#include "rbkit_tracer.h"
#include "object_graph.h"

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

static int event_message(struct event_info event_details, char **full_event) {
  int message_size;

  if (event_details.class_name != NULL && event_details.object_id != 0) {
    message_size =
      asprintf(full_event, "%s %s %p",
               event_details.event_name,
               event_details.class_name,
               (void *)event_details.object_id);
  } else if(event_details.class_name != NULL && event_details.object_id == 0) {
    message_size = asprintf(full_event, "%s %s %d", event_details.event_name, event_details.class_name, 0);
  } else {
    message_size = asprintf(full_event, "%s", event_details.event_name);
  }
  return message_size;
}

static struct event_info get_event_info(int event_index, const char *class_name, VALUE object_id)
{
  struct event_info event_details = { event_names[event_index], class_name, object_id };
  return event_details;
}

static void send_event(struct event_info event_details) {
  char *full_event;
  int message_size = event_message(event_details, &full_event);

  msgpack_sbuffer_clear(logger->sbuf);
  msgpack_pack_raw(logger->msgpacker, message_size);
  msgpack_pack_raw_body(logger->msgpacker, full_event, message_size);
  zmq_send(zmq_publisher, logger->sbuf->data, logger->sbuf->size, 0);
  free(full_event);
}

static void trace_gc_invocation(void *data, int event_index) {
  if (event_index == 0) {
    struct event_info event_details = get_event_info(0, NULL, 0);
    send_event(event_details);
  } else if (event_index == 2) {
    struct event_info event_details = get_event_info(2, NULL, 0);
    send_event(event_details);
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

static void newobj_i(VALUE tpval, void *data) {
  rb_trace_arg_t *tparg = rb_tracearg_from_tracepoint(tpval);
  VALUE obj = rb_tracearg_object(tparg);
  VALUE klass = RBASIC_CLASS(obj);
  if (!NIL_P(klass) && BUILTIN_TYPE(obj) != T_NONE && BUILTIN_TYPE(obj) != T_ZOMBIE && BUILTIN_TYPE(obj) != T_ICLASS) {
    struct event_info event_details = get_event_info(3, rb_class2name(klass), obj);
    send_event(event_details);
  } else {
    struct event_info event_details = get_event_info(3, NULL, obj);
    send_event(event_details);
  }
}

static void freeobj_i(VALUE tpval, void *data) {
  rb_trace_arg_t *tparg = rb_tracearg_from_tracepoint(tpval);
  VALUE obj = rb_tracearg_object(tparg);
  VALUE klass = RBASIC_CLASS(obj);
  if (!NIL_P(klass) && BUILTIN_TYPE(obj) != T_NONE && BUILTIN_TYPE(obj) != T_ZOMBIE && BUILTIN_TYPE(obj) != T_ICLASS && BUILTIN_TYPE(obj) != T_OBJECT) {
    struct event_info event_details = get_event_info(4, rb_class2name(klass), obj);
    send_event(event_details);
  } else {
    struct event_info event_details = get_event_info(4, NULL, obj);
    send_event(event_details);
  }
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

  logger = get_trace_logger();

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
  
  logger->newobj_trace = rb_tracepoint_new(0, RUBY_INTERNAL_EVENT_NEWOBJ, newobj_i, logger);
  logger->freeobj_trace = rb_tracepoint_new(0, RUBY_INTERNAL_EVENT_FREEOBJ, freeobj_i, logger);
  create_gc_hooks();
  return Qnil;
}

static char * tracer_string_recv(void *socket) {
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


static int tracer_string_send(void *socket, const char *message) {
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

static VALUE stop_stat_server() {
  if (logger->enabled == Qtrue)
    stop_stat_tracing();

  msgpack_sbuffer_destroy(logger->sbuf);
  msgpack_packer_free(logger->msgpacker);
  zmq_close(zmq_publisher);
  zmq_close(zmq_response_socket);
  zmq_ctx_destroy(zmq_context);
  free(logger);
  return Qnil;
}

static VALUE start_stat_tracing() {
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

  struct ObjectDump * dump = get_object_dump();

  // Set size of array to hold all objects
  msgpack_pack_array(pk, dump->size);

  // Iterate through all object data
  struct ObjectData * data = dump->first ;
  for(;data != NULL; data = data->next ) {
    /* ObjectData is a map that looks like this :
     * {
     *   object_id: <OBJECT_ID_IN_HEX>,
     *   class_name: <CLASS_NAME>,
     *   references: [<OBJECT_ID_IN_HEX>, <OBJECT_ID_IN_HEX>, ...],
     *   TODO: file: <FILE_PATH>,
     *   TODO: line: <LINE_NO>
     * }
     */

    msgpack_pack_map(pk, 3);

    // Key1 : "object_id"
    msgpack_pack_raw(pk, strlen("object_id"));
    msgpack_pack_raw_body(pk, "object_id", strlen("object_id"));

    // Value1 : pointer address of object
    char * object_id;
    asprintf(&object_id, "%p", data->object_id);
    msgpack_pack_raw(pk, strlen(object_id));
    msgpack_pack_raw_body(pk, object_id, strlen(object_id));
    free(object_id);

    // Key2 : "class_name"
    msgpack_pack_raw(pk, strlen("class_name"));
    msgpack_pack_raw_body(pk, "class_name", strlen("class_name"));

    // Value2 : Class name of object
    if(data->class_name == NULL) {
      msgpack_pack_nil(pk);
    } else {
      msgpack_pack_raw(pk, strlen(data->class_name));
      msgpack_pack_raw_body(pk, data->class_name, strlen(data->class_name));
    }

    // Key3 : "references"
    msgpack_pack_raw(pk, strlen("references"));
    msgpack_pack_raw_body(pk, "references", strlen("class_name"));

    // Value3 : References held by the object
    msgpack_pack_array(pk, data->reference_count);
    if(data->reference_count != 0) {
      size_t count = 0;
      for(; count < data->reference_count; count++ ) {
        char * object_id;
        asprintf(&object_id, "%p", data->references[count]);
        msgpack_pack_raw(pk, strlen(object_id));
        msgpack_pack_raw_body(pk, object_id, strlen(object_id));
      }
      free(data->references);
    }

    free(data);
  }

  // Send packed message over zmq
  zmq_send(zmq_publisher, buffer->data, buffer->size, 0);

  //Cleanup
  free(dump);
  msgpack_sbuffer_destroy(buffer);
  msgpack_packer_free(pk);

  return Qnil;
}

void Init_rbkit_tracer(void) {
  VALUE objectStatsModule = rb_define_module("Rbkit");
  rb_define_module_function(objectStatsModule, "start_server", start_stat_server, -1);
  rb_define_module_function(objectStatsModule, "stop_server", stop_stat_server, 0);
  rb_define_module_function(objectStatsModule, "start_stat_tracing", start_stat_tracing, 0);
  rb_define_module_function(objectStatsModule, "stop_stat_tracing", stop_stat_tracing, 0);
  rb_define_module_function(objectStatsModule, "poll_for_request", poll_for_request, 0);
  rb_define_module_function(objectStatsModule, "send_objectspace_dump", send_objectspace_dump, 0);
}
