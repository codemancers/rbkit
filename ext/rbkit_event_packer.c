#include "rbkit_event_packer.h"
#include <sys/time.h>

static void pack_string(msgpack_packer *packer, char *string) {
  if(string == NULL) {
    msgpack_pack_nil(packer);
  } else {
    int length = strlen(string);
    msgpack_pack_raw(packer, length);
    msgpack_pack_raw_body(packer, string, length);
  }
}

static void pack_timestamp(msgpack_packer *packer) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    double time_in_milliseconds = (tv.tv_sec)*1000 + (tv.tv_usec)/1000;
    msgpack_pack_double(packer, time_in_milliseconds);
}

static void pack_pointer(msgpack_packer *packer, void * pointer) {
  char *pointer_string;
  asprintf(&pointer_string, "%p", pointer);
  pack_string(packer, pointer_string);
  free(pointer_string);
}

static void pack_event_header(msgpack_packer* packer, rbkit_event_type event_type)
{
  pack_string(packer, "event_type");
  msgpack_pack_int(packer, event_type);

  pack_string(packer, "timestamp");
  pack_timestamp(packer);
}

static void pack_obj_created_event(rbkit_obj_created_event *event, msgpack_sbuffer *sbuf, msgpack_packer *packer) {
  msgpack_pack_map(packer, 3);
  pack_event_header(packer, event->event_header.event_type);

  pack_string(packer, "payload");
  msgpack_pack_map(packer, 2);
  pack_string(packer, "object_id");
  pack_pointer(packer, event->object_id);
  pack_string(packer, "class");
  pack_string(packer, event->klass);
  //TODO: pack allocation info as well
}

static void pack_obj_destroyed_event(rbkit_obj_created_event *event, msgpack_sbuffer *sbuf, msgpack_packer *packer) {
  msgpack_pack_map(packer, 3);
  pack_event_header(packer, event->event_header.event_type);

  pack_string(packer, "payload");
  msgpack_pack_map(packer, 1);
  pack_string(packer, "object_id");
  pack_pointer(packer, event->object_id);
}

static void pack_event_header_only(rbkit_event_header *event_header, msgpack_sbuffer *sbuf, msgpack_packer *packer) {
  msgpack_pack_map(packer, 2);
  pack_event_header(packer, event_header->event_type);
}

static void pack_value_object(msgpack_packer *packer, VALUE value) {
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

static int hash_pack_iterator(VALUE key, VALUE value, VALUE hash_arg) {
  msgpack_packer *packer = (msgpack_packer *)hash_arg;

  // pack the key
  pack_value_object(packer,key);
  // pack the value
  pack_value_object(packer, value);
  return ST_CONTINUE;
}

static void pack_gc_stats_event(rbkit_hash_event *event, msgpack_sbuffer *sbuf, msgpack_packer *packer) {
  msgpack_pack_map(packer, 3);
  pack_event_header(packer, event->event_header.event_type);
  VALUE hash = event->hash;
  int size = RHASH_SIZE(hash);
  pack_string(packer, "payload");
  msgpack_pack_map(packer, size);
  rb_hash_foreach(hash, hash_pack_iterator, (VALUE)packer);
}

void pack_event(rbkit_event_header *event_header, msgpack_sbuffer *sbuf, msgpack_packer *packer) {
  msgpack_sbuffer_clear(sbuf);
  switch (event_header->event_type) {
    case obj_created:
      pack_obj_created_event(event_header, sbuf, packer);
      break;
    case obj_destroyed:
      pack_obj_destroyed_event(event_header, sbuf, packer);
      break;
    case gc_start:
      pack_event_header_only(event_header, sbuf, packer);
      break;
    case gc_end_m:
      pack_event_header_only(event_header, sbuf, packer);
      break;
    case gc_end_s:
      pack_event_header_only(event_header, sbuf, packer);
      break;
    case object_space_dump:
      //TODO
      break;
    case gc_stats:
      pack_gc_stats_event(event_header, sbuf, packer);
      break;
    default:
      fprintf(stderr, "Don't know how to pack event type : %u\n", event_header->event_type);
  }
}
