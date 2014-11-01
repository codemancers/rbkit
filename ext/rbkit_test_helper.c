#include <ruby.h>
#include "rbkit_message_aggregator.h"

static VALUE get_queued_messages() {
  msgpack_sbuffer * sbuf = msgpack_sbuffer_new();
  get_event_collection_message(sbuf);
  if(sbuf && sbuf->size > 0) {
    VALUE str = rb_str_new(sbuf->data, sbuf->size);
    message_list_clear();
    msgpack_sbuffer_destroy(sbuf);
    return str;
  }
  return Qnil;
}

void Init_rbkit_test_helper(void) {
  VALUE rbkit_module = rb_define_module("Rbkit");
  rb_define_module_function(rbkit_module, "get_queued_messages", get_queued_messages, 0);
}
