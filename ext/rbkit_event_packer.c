#include "rbkit_event_packer.h"
#include "rbkit_object_graph.h"
#include <sys/time.h>

static void pack_string(msgpack_packer *packer, const char *string) {
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
  msgpack_pack_int(packer, rbkit_message_field_event_type);
  msgpack_pack_int(packer, event_type);

  msgpack_pack_int(packer, rbkit_message_field_timestamp);
  pack_timestamp(packer);
}

static unsigned long message_counter = 0;

static unsigned long get_message_counter() {
  return message_counter++;
}

static void pack_obj_created_event(rbkit_obj_created_event *event, msgpack_packer *packer) {
  msgpack_pack_map(packer, 3);
  pack_event_header(packer, event->event_header.event_type);

  msgpack_pack_int(packer, rbkit_message_field_payload);
  msgpack_pack_map(packer, 2);
  msgpack_pack_int(packer, rbkit_message_field_object_id);
  msgpack_pack_unsigned_long_long(packer, event->object_id);
  msgpack_pack_int(packer, rbkit_message_field_class_name);
  pack_string(packer, event->klass);
  //TODO: pack allocation info as well
}

static void pack_obj_destroyed_event(rbkit_obj_destroyed_event *event, msgpack_packer *packer) {
  msgpack_pack_map(packer, 3);
  pack_event_header(packer, event->event_header.event_type);

  msgpack_pack_int(packer, rbkit_message_field_payload);
  msgpack_pack_map(packer, 1);
  msgpack_pack_int(packer, rbkit_message_field_object_id);
  msgpack_pack_unsigned_long_long(packer, event->object_id);
}

static void pack_event_header_only(rbkit_event_header *event_header, msgpack_packer *packer) {
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

static void pack_hash_event(rbkit_hash_event *event, msgpack_packer *packer) {
  msgpack_pack_map(packer, 3);
  pack_event_header(packer, event->event_header.event_type);
  VALUE hash = event->hash;
  int size = RHASH_SIZE(hash);
  msgpack_pack_int(packer, rbkit_message_field_payload);
  msgpack_pack_map(packer, size);
  rb_hash_foreach(hash, hash_pack_iterator, (VALUE)packer);
}

static void pack_object_space_dump_event(rbkit_object_space_dump_event *event, msgpack_packer *packer) {
  rbkit_object_dump *dump = event->dump;
  msgpack_pack_map(packer, 3);
  pack_event_header(packer, event->event_header.event_type);
  msgpack_pack_int(packer, rbkit_message_field_payload);
  // Set size of array to hold all objects
  msgpack_pack_array(packer, dump->object_count);

  // Iterate through all object data
  rbkit_object_dump_page * page = dump->first ;
  while(page != NULL) {
    rbkit_object_data *data;
    size_t i = 0;
    for(;i < page->count; i++) {
      data = &(page->data[i]);
      /* Object dump is a map that looks like this :
       * {
       *   object_id: <OBJECT_ID_IN_HEX>,
       *   class: <CLASS_NAME>,
       *   references: [<OBJECT_ID_IN_HEX>, <OBJECT_ID_IN_HEX>, ...],
       *   file: <FILE_PATH>,
       *   line: <LINE_NO>,
       *   size: <SIZE>
       * }
       */

      msgpack_pack_map(packer, 6);

      // Key1 : rbkit_message_field_object_id
      msgpack_pack_int(packer, rbkit_message_field_object_id);

      // Value1 : pointer address of object
      msgpack_pack_unsigned_long_long(packer, data->object_id);

      // Key2 : rbkit_message_field_class_name
      msgpack_pack_int(packer, rbkit_message_field_class_name);

      // Value2 : Class name of object
      pack_string(packer, data->class_name);

      // Key3 : rbkit_message_field_references
      msgpack_pack_int(packer, rbkit_message_field_references);

      // Value3 : References held by the object
      msgpack_pack_array(packer, data->reference_count);
      if(data->reference_count != 0) {
        size_t count = 0;
        for(; count < data->reference_count; count++ )
          msgpack_pack_unsigned_long_long(packer, data->references[count]);
        free(data->references);
      }

      // Key4 : rbkit_message_field_file
      msgpack_pack_int(packer, rbkit_message_field_file);

      // Value4 : File path where object is defined
      pack_string(packer, data->file);

      // Key5 : rbkit_message_field_line
      msgpack_pack_int(packer, rbkit_message_field_line);

      // Value5 : Line no where object is defined
      if(data->line == 0)
        msgpack_pack_nil(packer);
      else
        msgpack_pack_unsigned_long(packer, data->line);

      // Key6 : rbkit_message_field_size
      msgpack_pack_int(packer, rbkit_message_field_size);

      // Value6 : Size of the object in memory
      if(data->size == 0)
        msgpack_pack_nil(packer);
      else
        msgpack_pack_uint32(packer, data->size);
    }
    rbkit_object_dump_page * prev = page;
    page = page->next;
    free(prev);
  }
}

static void pack_event_collection_event(rbkit_event_collection_event *event, msgpack_packer *packer) {
  msgpack_sbuffer *sbuf = packer->data;
  msgpack_pack_map(packer, 4);
  pack_event_header(packer, event->event_header.event_type);
  msgpack_pack_int(packer, rbkit_message_field_message_counter);
  msgpack_pack_unsigned_long(packer, get_message_counter());
  msgpack_pack_int(packer, rbkit_message_field_payload);
  msgpack_pack_array(packer, event->message_count);
  sbuf->data = realloc(sbuf->data, event->buffer_size + sbuf->size);
  memcpy(sbuf->data + sbuf->size, event->buffer, event->buffer_size);
  sbuf->size += event->buffer_size;
}

void pack_event(rbkit_event_header *event_header, msgpack_packer *packer) {
  msgpack_sbuffer *sbuf = packer->data;
  msgpack_sbuffer_clear(sbuf);

  switch (event_header->event_type) {
    case obj_created:
      pack_obj_created_event((rbkit_obj_created_event *)event_header, packer);
      break;
    case obj_destroyed:
      pack_obj_destroyed_event((rbkit_obj_destroyed_event *)event_header, packer);
      break;
    case gc_start:
      pack_event_header_only(event_header, packer);
      break;
    case gc_end_m:
      pack_event_header_only(event_header, packer);
      break;
    case gc_end_s:
      pack_event_header_only(event_header, packer);
      break;
    case object_space_dump:
      pack_object_space_dump_event((rbkit_object_space_dump_event *)event_header, packer);
      break;
    case gc_stats:
      pack_hash_event((rbkit_hash_event *)event_header, packer);
      break;
    case handshake:
      pack_hash_event((rbkit_hash_event *)event_header, packer);
      break;
    case event_collection:
      pack_event_collection_event((rbkit_event_collection_event *)event_header, packer);
      break;
    default:
      rb_raise(rb_eNotImpError,
          "Rbkit : Unpacking of event type '%u' not implemented",
          event_header->event_type);
  }
}

VALUE rbkit_message_fields_as_hash() {
  VALUE events = rb_hash_new();
  rb_hash_aset(events, ID2SYM(rb_intern("event_type")), INT2FIX(rbkit_message_field_event_type));
  rb_hash_aset(events, ID2SYM(rb_intern("timestamp")), INT2FIX(rbkit_message_field_timestamp));
  rb_hash_aset(events, ID2SYM(rb_intern("payload")), INT2FIX(rbkit_message_field_payload));
  rb_hash_aset(events, ID2SYM(rb_intern("object_id")), INT2FIX(rbkit_message_field_object_id));
  rb_hash_aset(events, ID2SYM(rb_intern("class_name")), INT2FIX(rbkit_message_field_class_name));
  rb_hash_aset(events, ID2SYM(rb_intern("references")), INT2FIX(rbkit_message_field_references));
  rb_hash_aset(events, ID2SYM(rb_intern("file")), INT2FIX(rbkit_message_field_file));
  rb_hash_aset(events, ID2SYM(rb_intern("line")), INT2FIX(rbkit_message_field_line));
  rb_hash_aset(events, ID2SYM(rb_intern("size")), INT2FIX(rbkit_message_field_size));
  rb_hash_aset(events, ID2SYM(rb_intern("message_counter")), INT2FIX(rbkit_message_field_message_counter));
  OBJ_FREEZE(events);
  return events;
}
