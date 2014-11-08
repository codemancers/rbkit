#ifndef RBKIT_MESSAGE_PACKER
#define RBKIT_MESSAGE_PACKER
#include "msgpack.h"
#include "rbkit_event.h"

typedef enum _rbkit_message_fields {
  rbkit_message_field_event_type,
  rbkit_message_field_timestamp,
  rbkit_message_field_payload,
  rbkit_message_field_object_id,
  rbkit_message_field_class_name,
  rbkit_message_field_references,
  rbkit_message_field_file,
  rbkit_message_field_line,
  rbkit_message_field_size,
  rbkit_message_field_message_counter
} rbkit_message_fields;

VALUE rbkit_message_fields_as_hash();

void pack_event(rbkit_event_header *event_header, msgpack_packer *packer);

#endif
