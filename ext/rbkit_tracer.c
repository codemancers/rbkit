//
//  rbkit.c
//  gather_stats
//
//  Created by Hemant Kumar on 26/04/14.
//  Copyright (c) 2014 Codemancers. All rights reserved.
//

#include "rbkit_tracer.h"
#include "rbkit_allocation_info.h"
#include "rbkit_event.h"
#include "rbkit_event_packer.h"
#include "rbkit_object_graph.h"
#include "rbkit_message_aggregator.h"
#include "rbkit_test_helper.h"
#include <sys/time.h>

static const char *event_names[] = {
  "gc_start",
  "gc_end_m",
  "gc_end_s",
  "obj_created",
  "obj_destroyed"
};

static rbkit_logger *logger;
static void *zmq_publisher;
static void *zmq_context;
static void *zmq_response_socket;
static zmq_pollitem_t items[1];

static rbkit_logger * get_trace_logger() {
  int i = 0;
  if (logger == 0) {
    logger = ALLOC_N(rbkit_logger, 1);
    logger->enabled = Qfalse;
    logger->newobj_trace = 0;
    logger->freeobj_trace = 0;
    logger->object_table = st_init_numtable();
    logger->str_table = st_init_strtable();

    logger->sbuf = msgpack_sbuffer_new();
    logger->msgpacker = msgpack_packer_new(logger->sbuf, msgpack_sbuffer_write);
    logger->data = 0;
  }
  return logger;
}

static void
gc_start_i(VALUE tpval, void *data)
{
  rbkit_logger * arg = (rbkit_logger *)data;
  rbkit_event_header *event = malloc(sizeof(rbkit_event_header));
  event->event_type = gc_start;
  pack_event(event, arg->msgpacker);
  free(event);
  add_message(arg->sbuf);
}

static void
gc_end_mark_i(VALUE tpval, void *data)
{
  rbkit_logger * arg = (rbkit_logger *)data;
  rbkit_event_header *event = malloc(sizeof(rbkit_event_header));
  event->event_type = gc_end_m;
  pack_event(event, arg->msgpacker);
  free(event);
  add_message(arg->sbuf);
}

static void
gc_end_sweep_i(VALUE tpval, void *data)
{
  rbkit_logger * arg = (rbkit_logger *)data;
  rbkit_event_header *event = malloc(sizeof(rbkit_event_header));
  event->event_type = gc_end_s;
  pack_event(event, arg->msgpacker);
  free(event);
  add_message(arg->sbuf);
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

// Refer Ruby source ext/objspace/object_tracing.c::newobj_i
static void newobj_i(VALUE tpval, void *data) {
  rbkit_logger * arg = (rbkit_logger *)data;
  rb_trace_arg_t *tparg = rb_tracearg_from_tracepoint(tpval);
  rbkit_allocation_info *info = new_rbkit_allocation_info(tparg, arg->str_table, arg->object_table);

  VALUE obj = rb_tracearg_object(tparg);
  VALUE klass = RBASIC_CLASS(obj);
  const char *class_name = NULL;
  if (!NIL_P(klass) && BUILTIN_TYPE(obj) != T_NONE && BUILTIN_TYPE(obj) != T_ZOMBIE && BUILTIN_TYPE(obj) != T_ICLASS)
    class_name = rb_class2name(klass);

  rbkit_obj_created_event *event = new_rbkit_obj_created_event(FIX2ULONG(rb_obj_id(obj)), class_name, info);
  pack_event((rbkit_event_header *)event, arg->msgpacker);
  free(event);
  add_message(arg->sbuf);
}

// Refer Ruby source ext/objspace/object_tracing.c::freeobj_i
static void freeobj_i(VALUE tpval, void *data) {
  rb_trace_arg_t *tparg = rb_tracearg_from_tracepoint(tpval);
  VALUE obj = rb_tracearg_object(tparg);
  rbkit_logger *arg = (rbkit_logger *)data;

  // Delete allocation info of freed object
  delete_rbkit_allocation_info(tparg, obj, arg->str_table, arg->object_table);

  rbkit_obj_destroyed_event *event = new_rbkit_obj_destroyed_event(FIX2ULONG(rb_obj_id(obj)));
  pack_event((rbkit_event_header *)event, arg->msgpacker);
  free(event);
  add_message(arg->sbuf);
}

static VALUE start_stat_server(int argc, VALUE *argv, VALUE self) {
  VALUE pub_port;
  VALUE request_port;
  int bind_result;

  rb_scan_args(argc, argv, "02", &pub_port, &request_port);

  char zmq_endpoint[14];
  sprintf(zmq_endpoint, "tcp://*:%d", FIX2INT(pub_port));
  zmq_context = zmq_ctx_new();
  zmq_publisher = zmq_socket(zmq_context, ZMQ_PUB);
  bind_result = zmq_bind(zmq_publisher, zmq_endpoint);
  if(bind_result != 0)
    return Qfalse;

  char zmq_request_endpoint[14];
  sprintf(zmq_request_endpoint, "tcp://*:%d", FIX2INT(request_port));
  zmq_response_socket = zmq_socket(zmq_context, ZMQ_REP);
  bind_result = zmq_bind(zmq_response_socket, zmq_request_endpoint);
  if(bind_result != 0)
    return Qfalse;

  // Creates a list which aggregates messages
  message_list_new();
  logger = get_trace_logger();
  logger->newobj_trace = rb_tracepoint_new(0, RUBY_INTERNAL_EVENT_NEWOBJ, newobj_i, logger);
  logger->freeobj_trace = rb_tracepoint_new(0, RUBY_INTERNAL_EVENT_FREEOBJ, freeobj_i, logger);
  rb_gc_register_mark_object(logger->newobj_trace);
  rb_gc_register_mark_object(logger->freeobj_trace);
  create_gc_hooks();

  items[0].socket = zmq_response_socket;
  items[0].events = ZMQ_POLLIN;
  return Qtrue;
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

static VALUE rbkit_status_as_hash() {
  VALUE status = rb_hash_new();
  VALUE pid = rb_funcall(rb_path2class("Process"), rb_intern("pid"), 0, 0);
  VALUE processName = rb_funcall(rb_path2class("Process"), rb_intern("argv0"), 0, 0);
  int object_trace_enabled = (logger && logger->enabled) ? 1 : 0;
  rb_hash_aset(status, ID2SYM(rb_intern("process_name")), processName);
  rb_hash_aset(status, ID2SYM(rb_intern("pwd")), rb_dir_getwd());
  rb_hash_aset(status, ID2SYM(rb_intern("pid")), pid);
  rb_hash_aset(status, ID2SYM(rb_intern("object_trace_enabled")), INT2FIX(object_trace_enabled));
  return status;
}

static void send_handshake_response() {
  msgpack_sbuffer *buffer = msgpack_sbuffer_new();
  msgpack_packer *packer = msgpack_packer_new(buffer, msgpack_sbuffer_write);

  rbkit_hash_event *event = new_rbkit_hash_event(handshake, rbkit_status_as_hash());
  pack_event((rbkit_event_header *)event, packer);
  free(event);

  if(buffer && buffer->size > 0)
    zmq_send(zmq_response_socket, buffer->data, buffer->size, 0);

  msgpack_sbuffer_free(buffer);
  msgpack_packer_free(packer);
}

static VALUE poll_for_request() {
  // Wait for 100 millisecond and check if there is a message
  // we can't wait here indefenitely because ruby is not aware this is a
  // blocking operation. Remember ruby releases GVL in a thread
  // whenever it encounters a known blocking operation.
  zmq_poll(items, 1, 100);
  if (items[0].revents && ZMQ_POLLIN) {
    char *message = tracer_string_recv(zmq_response_socket);
    if(strcmp(message, "handshake") == 0) {
      send_handshake_response();
    } else {
      tracer_string_send(zmq_response_socket, "ok");
    }
    VALUE command_ruby_string = rb_str_new_cstr(message);
    free(message);
    return command_ruby_string;
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
  free((void *)key);
  return ST_CONTINUE;
}

static int free_values_i(st_data_t key, st_data_t value, void *data) {
  free((void *)value);
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

  msgpack_sbuffer_free(logger->sbuf);
  msgpack_packer_free(logger->msgpacker);
  zmq_close(zmq_publisher);
  zmq_close(zmq_response_socket);
  zmq_ctx_destroy(zmq_context);
  free(logger);
  logger = 0;
  return Qnil;
}

static VALUE send_hash_as_event(int argc, VALUE *argv, VALUE self) {
  VALUE hash_object;
  VALUE event_type;

  rb_scan_args(argc, argv, "20", &hash_object, &event_type);

  msgpack_sbuffer *buffer = msgpack_sbuffer_new();
  msgpack_packer *packer = msgpack_packer_new(buffer, msgpack_sbuffer_write);

  rbkit_hash_event *event = new_rbkit_hash_event(FIX2INT(event_type), hash_object);
  pack_event((rbkit_event_header *)event, packer);
  free(event);

  add_message(buffer);
  msgpack_sbuffer_free(buffer);
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

  rbkit_object_dump * dump = get_object_dump(logger->object_table);
  rbkit_object_space_dump_event *event = new_rbkit_object_space_dump_event(dump);
  pack_event((rbkit_event_header *)event, pk);
  free(event);
  add_message(buffer);

  free(dump);
  msgpack_sbuffer_free(buffer);
  msgpack_packer_free(pk);
  return Qnil;
}

/*
 * Creates a msgpack array which contains all the messages packed after
 * the last time send_messages() was called, and sends it over the PUB socket.
 */
static VALUE send_messages() {
  //Get all aggregated messages as payload of a single event.
  msgpack_sbuffer * sbuf = msgpack_sbuffer_new();
  get_event_collection_message(sbuf);
  //Send the msgpack array over zmq PUB socket
  if(sbuf && sbuf->size > 0)
    zmq_send(zmq_publisher, sbuf->data, sbuf->size, 0);
  // Clear the aggregated messages
  message_list_clear();
  msgpack_sbuffer_free(sbuf);
  return Qnil;
}

static VALUE enable_test_mode() {
  Init_rbkit_test_helper();
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
  rb_define_module_function(objectStatsModule, "enable_test_mode", enable_test_mode, 0);
  rb_define_module_function(objectStatsModule, "status", rbkit_status_as_hash, 0);
  rb_define_const(objectStatsModule, "EVENT_TYPES", rbkit_event_types_as_hash());
  rb_define_const(objectStatsModule, "MESSAGE_FIELDS", rbkit_message_fields_as_hash());
}
