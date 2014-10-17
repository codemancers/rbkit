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


static void pack_event_header(msgpack_packer* packer, rbkit_event_type event_type)
{
  pack_string(packer, "event_type");
  msgpack_pack_int(packer, event_type);

  pack_string(packer, "timestamp");
  pack_timestamp(packer);
}

static void pack_obj_created_event(rbkit_obj_created_event *event, msgpack_sbuffer *sbuf, msgpack_packer *packer) {
  msgpack_sbuffer_clear(sbuf);
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

void pack_event(rbkit_event_header *event_header, msgpack_sbuffer *sbuf, msgpack_packer *packer) {
  msgpack_sbuffer_clear(sbuf);
  switch (event_header->event_type) {
    case obj_created:
      pack_obj_created_event(event_header, sbuf, packer);
      break;
    case obj_destroyed:
      //TODO
      break;
    case gc_start:
      //TODO
      break;
    case gc_end_m:
      //TODO
      break;
    case gc_end_s:
      //TODO
      break;
    case object_space_dump:
      //TODO
      break;
    case gc_stats:
      //TODO
      break;
  }
}
