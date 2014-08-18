#include <ruby.h>
#include "rbkit_message_aggregator.h"

static VALUE noop_send_messages() {
  //NOOP
  return Qnil;
}

static VALUE get_queued_messages() {
  msgpack_sbuffer * sbuf = (msgpack_sbuffer *)get_event_collection_message();
  fprintf(stderr, "Queued message size = %d\n", sbuf->size);
  /*return rb_str_new(sbuf->data, sbuf->size);*/
  return Qnil;
}

void Init_rbkit_test_helper(void) {
  VALUE rbkit_module = rb_define_module("Rbkit");
  rb_define_module_function(rbkit_module, "send_messages", noop_send_messages, 0);
  rb_define_module_function(rbkit_module, "get_queued_messages", get_queued_messages, 0);
}
