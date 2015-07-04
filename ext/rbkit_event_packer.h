#ifndef RBKIT_MESSAGE_PACKER
#define RBKIT_MESSAGE_PACKER
#define RBKIT_PROTOCOL_VERSION "2.0"
#include "msgpack.h"
#include "rbkit_event.h"

// Object dump will be split into multiple
// messages. This macro defines the number
// of object data that should be packed
// as the payload of one message.
#define MAX_OBJECT_DUMPS_IN_MESSAGE 1000

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
  rbkit_message_field_message_counter,
  rbkit_message_field_correlation_id,
  rbkit_message_field_complete_message_count,
  rbkit_message_field_method_name,
  rbkit_message_field_label,
  rbkit_message_field_singleton_method,
  rbkit_message_field_thread_id
} rbkit_message_fields;

VALUE rbkit_message_fields_as_hash();
VALUE rbkit_protocol_version();

void pack_event(rbkit_event_header *event_header, msgpack_packer *packer);

#endif
