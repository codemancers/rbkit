//
//  rbkit.c
//  gather_stats
//
//  Created by Hemant Kumar on 26/04/14.
//  Copyright (c) 2014 Codemancers. All rights reserved.
//

#include "rbkit_tracer.h"

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
  if (!NIL_P(klass)) {
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
  if (!NIL_P(klass)) {
    struct event_info event_details = get_event_info(4, rb_class2name(klass), obj);
    send_event(event_details);
  } else {
    struct event_info event_details = get_event_info(4, NULL, obj);
    send_event(event_details);
  }
}

static VALUE start_stat_server(int argc, VALUE *argv, VALUE self) {
  int default_port = 5555;
  VALUE port;

  rb_scan_args(argc, argv, "01", &port);
  if (!NIL_P(port)) {
    default_port = FIX2INT(port);
    if (default_port < 1024 || default_port > 65000)
      rb_raise(rb_eArgError, "invalid port value");
  }

  logger = get_trace_logger();

  char zmq_endpoint[14];
  sprintf(zmq_endpoint, "tcp://*:%d", default_port);

  zmq_context = zmq_ctx_new();
  zmq_publisher = zmq_socket(zmq_context, ZMQ_PUB);
  int bind_result = zmq_bind(zmq_publisher, zmq_endpoint);
  assert(bind_result == 0);

  logger->newobj_trace = rb_tracepoint_new(0, RUBY_INTERNAL_EVENT_NEWOBJ, newobj_i, logger);
  logger->freeobj_trace = rb_tracepoint_new(0, RUBY_INTERNAL_EVENT_FREEOBJ, freeobj_i, logger);
  create_gc_hooks();
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

static VALUE stop_stat_server() {
  if (logger->enabled == Qtrue)
    stop_stat_tracing();

  msgpack_sbuffer_destroy(logger->sbuf);
  msgpack_packer_free(logger->msgpacker);
  zmq_close(zmq_publisher);
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

void Init_rbkit_tracer(void) {
  VALUE objectStatsModule = rb_define_module("Rbkit");
  rb_define_module_function(objectStatsModule, "start_server", start_stat_server, -1);
  rb_define_module_function(objectStatsModule, "stop_server", stop_stat_server, 0);
  rb_define_module_function(objectStatsModule, "start_stat_tracing", start_stat_tracing, 0);
  rb_define_module_function(objectStatsModule, "stop_stat_tracing", stop_stat_tracing, 0);
}
