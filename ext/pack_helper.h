#include "msgpack.h"

// To be removed vvvvvvvvvvvvvvvv
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

static void pack_event_header(msgpack_packer* packer, const char *event_type, int map_size)
{
  msgpack_pack_map(packer, map_size);
  pack_string(packer, "event_type");
  pack_string(packer, event_type);

  pack_string(packer, "timestamp");
  pack_timestamp(packer);
}
// To be removed ^^^^^^^^^^^^^^^^^
