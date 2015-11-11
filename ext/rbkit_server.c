//
//  rbkit.c
//  gather_stats
//
//  Created by Hemant Kumar on 26/04/14.
//  Copyright (c) 2014 Codemancers. All rights reserved.
//

#include "rbkit_server.h"
#include "rbkit_allocation_info.h"
#include "rbkit_event.h"
#include "rbkit_event_packer.h"
#include "rbkit_object_graph.h"
#include "rbkit_message_aggregator.h"
#include "rbkit_sampling_profiler.h"
#include "rbkit_test_helper.h"
#include "rbkit_object_tracer.h"

static rbkit_logger *logger;
static void *zmq_publisher = NULL;
static void *zmq_context = NULL;
static void *zmq_response_socket = NULL;
static zmq_pollitem_t items[1];
static int test_mode_enabled = 0;
static VALUE server_instance = Qnil;

static rbkit_logger * get_trace_logger() {
  if (logger == 0) {
    logger = ALLOC_N(rbkit_logger, 1);
    logger->enabled = Qfalse;
    logger->sampling_profiler_enabled = Qfalse;
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

static void publish_message(char *buf, size_t size) {
  VALUE publish_callback;
  if(zmq_publisher)
    zmq_send(zmq_publisher, buf, size, 0);
  publish_callback = rb_ivar_get(server_instance, rb_intern("@publish_callback"));
  if (rb_obj_is_proc(publish_callback) == Qtrue) {
    VALUE message = rb_str_new(buf, size);
    VALUE argv[] = { message };
    rb_proc_call_with_block(publish_callback, 1, argv, Qnil);
  }
}

static void respond_with_message(char *buf, size_t size) {
  VALUE respond_callback;
  if(zmq_response_socket)
    zmq_send(zmq_response_socket, buf, size, 0);
  respond_callback = rb_ivar_get(server_instance, rb_intern("@respond_callback"));
  if (rb_obj_is_proc(respond_callback) == Qtrue) {
    VALUE message = rb_str_new(buf, size);
    VALUE argv[] = { message };
    rb_proc_call_with_block(respond_callback, 1, argv, Qnil);
  }
}

static void trigger_publish_callback() {
  msgpack_sbuffer *sbuf;
  //Get all aggregated messages as payload of a single event.
  sbuf = msgpack_sbuffer_new();
  get_event_collection_message(sbuf);
  //Send the msgpack array over PUB socket
  if(sbuf && sbuf->size > 0)
    publish_message(sbuf->data, sbuf->size);
  // Clear the aggregated messages
  message_list_clear();
  msgpack_sbuffer_free(sbuf);
}

/*
 * Creates a msgpack array which contains all the messages packed after
 * the last time send_messages() was called, and sends it over the PUB socket.
 */
static VALUE send_messages() {
  if(test_mode_enabled)
    return Qnil; //NOOP

  rb_postponed_job_register(0, trigger_publish_callback, (void *)server_instance);
  return Qnil;
}

// Adds the message to the queue and sends the queued messages if
// the queue becomes full.
static void send_message(msgpack_sbuffer *buffer) {
  queue_message(buffer);
  if(queued_message_count() == MESSAGE_BATCH_SIZE)
    send_messages();
}

static void
gc_start_i(VALUE tpval, void *data)
{
  rbkit_logger * arg = (rbkit_logger *)data;
  rbkit_event_header *event = malloc(sizeof(rbkit_event_header));
  event->event_type = gc_start;
  pack_event(event, arg->msgpacker);
  free(event);
  send_message(arg->sbuf);
}

static void
gc_end_mark_i(VALUE tpval, void *data)
{
  rbkit_logger * arg = (rbkit_logger *)data;
  rbkit_event_header *event = malloc(sizeof(rbkit_event_header));
  event->event_type = gc_end_m;
  pack_event(event, arg->msgpacker);
  free(event);
  send_message(arg->sbuf);
}

static void
gc_end_sweep_i(VALUE tpval, void *data)
{
  rbkit_logger * arg = (rbkit_logger *)data;
  rbkit_event_header *event = malloc(sizeof(rbkit_event_header));
  event->event_type = gc_end_s;
  pack_event(event, arg->msgpacker);
  free(event);
  send_message(arg->sbuf);
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
  rbkit_object_allocations_event *event;
  rbkit_logger * arg = (rbkit_logger *)data;
  rb_trace_arg_t *tparg = rb_tracearg_from_tracepoint(tpval);
  VALUE obj = rb_tracearg_object(tparg);
  rbkit_allocation_info *info = new_rbkit_allocation_info(tparg, arg->str_table, arg->object_table);
  rbkit_new_object_info *obj_info = malloc(sizeof(rbkit_new_object_info));
  VALUE klass;
  obj_info->file = info->path;
  obj_info->line = info->line;
  obj_info->object_id = FIX2ULONG(rb_obj_id(obj));
  klass = RBASIC_CLASS(obj);
  obj_info->klass = NULL;
  obj_info->size = 0;
  if (!NIL_P(klass) && BUILTIN_TYPE(obj) != T_NONE && BUILTIN_TYPE(obj) != T_ZOMBIE && BUILTIN_TYPE(obj) != T_ICLASS) {
    obj_info->klass = rb_class2name(klass);
    if (BUILTIN_TYPE(obj) != T_NODE) {
      obj_info->size = rb_obj_memsize_of(obj);
    }
  }

  collect_stack_trace(&(obj_info->stacktrace));

  if(object_allocation_info_full()) {
    event = new_rbkit_object_allocations_event(get_object_allocation_infos());
    pack_event((rbkit_event_header *)event, arg->msgpacker);
    send_message(arg->sbuf);
    free(event);
  }
  push_new_object_allocation_info(obj_info);
}

// Refer Ruby source ext/objspace/object_tracing.c::freeobj_i
static void freeobj_i(VALUE tpval, void *data) {
  rb_trace_arg_t *tparg = rb_tracearg_from_tracepoint(tpval);
  rbkit_obj_destroyed_event *event;
  VALUE obj = rb_tracearg_object(tparg);
  rbkit_logger *arg = (rbkit_logger *)data;

  // Delete allocation info of freed object
  delete_rbkit_allocation_info(tparg, obj, arg->str_table, arg->object_table);

  event = new_rbkit_obj_destroyed_event(FIX2ULONG(rb_obj_id(obj)));
  pack_event((rbkit_event_header *)event, arg->msgpacker);
  free(event);
  send_message(arg->sbuf);
}

static VALUE start_stat_server(int argc, VALUE *argv, VALUE self) {
  VALUE pub_port;
  VALUE request_port;
  int bind_result, pub_port_int, req_port_int;
  char zmq_endpoint[21], zmq_request_endpoint[21];
  server_instance = self;

  rb_scan_args(argc, argv, "02", &pub_port, &request_port);
  pub_port_int = FIX2INT(pub_port);
  req_port_int = FIX2INT(request_port);
  if(pub_port_int != 0 || request_port != 0) {
    zmq_context = zmq_ctx_new();
  }

  if(pub_port_int != 0) {
    sprintf(zmq_endpoint, "tcp://127.0.0.1:%d", FIX2INT(pub_port));
    zmq_publisher = zmq_socket(zmq_context, ZMQ_PUB);
    bind_result = zmq_bind(zmq_publisher, zmq_endpoint);
    if(bind_result != 0)
      return Qfalse;
  }

  if(req_port_int != 0) {
    sprintf(zmq_request_endpoint, "tcp://127.0.0.1:%d", FIX2INT(request_port));
    zmq_response_socket = zmq_socket(zmq_context, ZMQ_REP);
    bind_result = zmq_bind(zmq_response_socket, zmq_request_endpoint);
    if(bind_result != 0)
      return Qfalse;

    items[0].socket = zmq_response_socket;
    items[0].events = ZMQ_POLLIN;
  }

  // Creates a list which aggregates messages
  message_list_new();
  logger = get_trace_logger();
  logger->newobj_trace = rb_tracepoint_new(0, RUBY_INTERNAL_EVENT_NEWOBJ, newobj_i, logger);
  logger->freeobj_trace = rb_tracepoint_new(0, RUBY_INTERNAL_EVENT_FREEOBJ, freeobj_i, logger);
  rb_gc_register_mark_object(logger->newobj_trace);
  rb_gc_register_mark_object(logger->freeobj_trace);
  create_gc_hooks();

  return Qtrue;
}

char * tracer_string_recv(void *socket) {
  zmq_msg_t msg;
  size_t message_size;
  char *message;
  int rc = zmq_msg_init(&msg);
  assert(rc == 0);

  rc = zmq_msg_recv(&msg, socket, 0);
  assert(rc != -1);
  message_size = zmq_msg_size(&msg);
  message = (char *)malloc(message_size +1);
  memcpy(message, zmq_msg_data(&msg), message_size);
  message[message_size] = 0;
  zmq_msg_close(&msg);
  return message;
}


static VALUE rbkit_status_as_hash(VALUE self) {
  VALUE status = rb_hash_new();
  VALUE pid = rb_funcall(rb_path2class("Process"), rb_intern("pid"), 0, 0);
  VALUE processName = rb_funcall(rb_path2class("Process"), rb_intern("argv0"), 0, 0);
  VALUE rbkitModule = rb_define_module("Rbkit");
  int object_trace_enabled = (logger && logger->enabled) ? 1 : 0;
  int cpu_profiling_enabled = (logger && logger->sampling_profiler_enabled) ? 1 : 0;

  rb_hash_aset(status, ID2SYM(rb_intern("rbkit_server_version")), rb_const_get(rbkitModule, rb_intern("VERSION")));
  rb_hash_aset(status, ID2SYM(rb_intern("rbkit_protocol_version")), rbkit_protocol_version());
  rb_hash_aset(status, ID2SYM(rb_intern("process_name")), processName);
  rb_hash_aset(status, ID2SYM(rb_intern("pwd")), rb_dir_getwd());
  rb_hash_aset(status, ID2SYM(rb_intern("pid")), pid);
  rb_hash_aset(status, ID2SYM(rb_intern("object_trace_enabled")), INT2FIX(object_trace_enabled));
  rb_hash_aset(status, ID2SYM(rb_intern("cpu_profiling_enabled")), INT2FIX(cpu_profiling_enabled));
  rb_hash_aset(status, ID2SYM(rb_intern("clock_type")), rb_ivar_get(self, rb_intern("@clock_type")));
  rb_hash_aset(status, ID2SYM(rb_intern("cpu_profiling_mode")), rb_ivar_get(self, rb_intern("@cpu_profiling_mode")));
  return status;
}

static VALUE send_handshake_response(VALUE self) {
  msgpack_sbuffer *buffer = msgpack_sbuffer_new();
  msgpack_packer *packer = msgpack_packer_new(buffer, msgpack_sbuffer_write);

  rbkit_hash_event *event = new_rbkit_hash_event(handshake, rbkit_status_as_hash(self));
  pack_event((rbkit_event_header *)event, packer);
  free(event);

  if(buffer && buffer->size > 0)
    respond_with_message(buffer->data, buffer->size);

  msgpack_sbuffer_free(buffer);
  msgpack_packer_free(packer);
  return Qnil;
}

static VALUE poll_for_request(VALUE self) {
  VALUE command_ruby_string;
  char *message = NULL;
  // Wait for 100 millisecond and check if there is a message
  // we can't wait here indefenitely because ruby is not aware this is a
  // blocking operation. Remember ruby releases GVL in a thread
  // whenever it encounters a known blocking operation.
  zmq_poll(items, 1, 100);
  if (items[0].revents && ZMQ_POLLIN) {
    message = tracer_string_recv(zmq_response_socket);
    command_ruby_string = rb_str_new_cstr(message);
    free(message);
    return command_ruby_string;
  } else {
    return Qnil;
  }
}

static VALUE send_command_ack(VALUE self) {
  char *response = (char *)"ok";
  respond_with_message(response, strlen(response));
  return Qnil;
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

static void queue_cpu_sample_for_sending(rbkit_cpu_sample *sample) {
  msgpack_sbuffer* buffer = msgpack_sbuffer_new();
  msgpack_packer* packer = msgpack_packer_new(buffer, msgpack_sbuffer_write);

  rbkit_cpu_sample_event *event = new_rbkit_cpu_sample_event(sample);

  pack_event((rbkit_event_header *)event, packer);
  free(event);

  send_message(buffer);
  msgpack_sbuffer_free(buffer);
  msgpack_packer_free(packer);
}

static VALUE start_sampling_profiler(VALUE self, VALUE clock_type, VALUE interval) {
  if (logger->sampling_profiler_enabled == Qtrue)
    return Qnil;
  if(!SYMBOL_P(clock_type))
    rb_raise(rb_eArgError, "clock_type should be a symbol, either :wall or :cpu");
  rbkit_install_sampling_profiler(clock_type == ID2SYM(rb_intern("wall")), FIX2INT(interval), queue_cpu_sample_for_sending);
  logger->sampling_profiler_enabled = Qtrue;
  return Qnil;
}

static VALUE stop_sampling_profiler() {
  if (logger->sampling_profiler_enabled == Qfalse)
    return Qnil;
  rbkit_uninstall_sampling_profiler();
  logger->sampling_profiler_enabled = Qfalse;
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
  msgpack_sbuffer *buffer;
  msgpack_packer *packer;
  rbkit_hash_event *event;

  /*
   * Check if the server connection is available before trying to pack GC stat
   * information into msgpack buffers.
   */
  if(logger == 0)
    return Qfalse;
  rb_scan_args(argc, argv, "20", &hash_object, &event_type);

  buffer = msgpack_sbuffer_new();
  packer = msgpack_packer_new(buffer, msgpack_sbuffer_write);

  event = new_rbkit_hash_event(FIX2INT(event_type), hash_object);
  pack_event((rbkit_event_header *)event, packer);
  free(event);

  send_message(buffer);
  msgpack_sbuffer_free(buffer);
  msgpack_packer_free(packer);
  return Qnil;
}

static VALUE start_stat_tracing() {
  int i;
  if (logger->enabled == Qtrue)
    return Qnil;
  rb_tracepoint_enable(logger->newobj_trace);
  rb_tracepoint_enable(logger->freeobj_trace);
  for (i=0; i<3; i++) {
    rb_tracepoint_enable(logger->hooks[i]);
  }
  logger->enabled = Qtrue;
  return Qnil;
}

static VALUE send_objectspace_dump(VALUE self) {
  msgpack_sbuffer* buffer = msgpack_sbuffer_new();
  msgpack_packer* pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);

  rbkit_object_dump * dump = get_object_dump(logger->object_table);
  rbkit_object_space_dump_event *event = new_rbkit_object_space_dump_event(dump);

  // Object space dump can span across messages.
  // So we keep creating and queueing the messages
  // until we've packed all objects.
  while(event->packed_objects < event->object_count) {
    pack_event((rbkit_event_header *)event, pk);
    send_message(buffer);
  }

  free(event->current_page);
  free(event);
  free(dump);
  msgpack_sbuffer_free(buffer);
  msgpack_packer_free(pk);
  return Qnil;
}

static VALUE enable_test_mode() {
  test_mode_enabled = 1;
  Init_rbkit_test_helper();
  return Qnil;
}

static VALUE disable_test_mode() {
  test_mode_enabled = 0;
  return Qnil;
}

void Init_rbkit_server(void) {
  VALUE rbkit_module, rbkit_server;

  rbkit_module = rb_define_module("Rbkit");
  rb_define_const(rbkit_module, "EVENT_TYPES", rbkit_event_types_as_hash());
  rb_define_const(rbkit_module, "MESSAGE_FIELDS", rbkit_message_fields_as_hash());
  rb_define_const(rbkit_module, "PROTOCOL_VERSION", rbkit_protocol_version());
  rb_define_module_function(rbkit_module, "enable_test_mode", enable_test_mode, 0);
  rb_define_module_function(rbkit_module, "disable_test_mode", disable_test_mode, 0);

  rbkit_server = rb_define_class_under(rbkit_module, "Server", rb_cObject);
  rb_define_method(rbkit_server, "start_stat_server", start_stat_server, -1);
  rb_define_method(rbkit_server, "stop_stat_server", stop_stat_server, 0);
  rb_define_method(rbkit_server, "start_stat_tracing", start_stat_tracing, 0);
  rb_define_method(rbkit_server, "stop_stat_tracing", stop_stat_tracing, 0);
  rb_define_method(rbkit_server, "start_sampling_profiler", start_sampling_profiler, 2);
  rb_define_method(rbkit_server, "stop_sampling_profiler", stop_sampling_profiler, 0);
  rb_define_method(rbkit_server, "poll_for_request", poll_for_request, 0);
  rb_define_method(rbkit_server, "send_objectspace_dump", send_objectspace_dump, 0);
  rb_define_method(rbkit_server, "send_hash_as_event", send_hash_as_event, -1);
  rb_define_method(rbkit_server, "send_messages", send_messages, 0);
  rb_define_method(rbkit_server, "status", rbkit_status_as_hash, 0);
  rb_define_method(rbkit_server, "send_handshake_response", send_handshake_response, 0);
  rb_define_method(rbkit_server, "send_command_ack", send_command_ack, 0);
}
