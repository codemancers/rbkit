#include "rbkit_event.h"

VALUE rbkit_event_types_as_hash() {
  VALUE events = rb_hash_new();
  rb_hash_aset(events, ID2SYM(rb_intern("obj_created")), INT2FIX(obj_created));
  rb_hash_aset(events, ID2SYM(rb_intern("obj_destroyed")), INT2FIX(obj_destroyed));
  rb_hash_aset(events, ID2SYM(rb_intern("gc_start")), INT2FIX(gc_start));
  rb_hash_aset(events, ID2SYM(rb_intern("gc_end_m")), INT2FIX(gc_end_m));
  rb_hash_aset(events, ID2SYM(rb_intern("gc_end_s")), INT2FIX(gc_end_s));
  rb_hash_aset(events, ID2SYM(rb_intern("object_space_dump")), INT2FIX(object_space_dump));
  rb_hash_aset(events, ID2SYM(rb_intern("gc_stats")), INT2FIX(gc_stats));
  rb_hash_aset(events, ID2SYM(rb_intern("event_collection")), INT2FIX(event_collection));
  rb_hash_aset(events, ID2SYM(rb_intern("handshake")), INT2FIX(handshake));
  OBJ_FREEZE(events);
  return events;
}

rbkit_obj_created_event *new_rbkit_obj_created_event(unsigned long long object_id,
    const char *klass, rbkit_allocation_info *info) {
  rbkit_obj_created_event *event = malloc(sizeof(rbkit_obj_created_event));

  rbkit_event_header *header = (rbkit_event_header *)event;
  header->event_type = obj_created;

  event->object_id = object_id;
  event->klass = klass;
  event->allocation_info = info;
  return event;
}

rbkit_obj_destroyed_event *new_rbkit_obj_destroyed_event(unsigned long long object_id) {
  rbkit_obj_destroyed_event *event = malloc(sizeof(rbkit_obj_destroyed_event));

  rbkit_event_header *header = (rbkit_event_header *)event;
  header->event_type = obj_destroyed;

  event->object_id = object_id;
  return event;
}

rbkit_hash_event *new_rbkit_hash_event(rbkit_event_type event_type, VALUE hash) {
  rbkit_hash_event *event = malloc(sizeof(rbkit_hash_event));

  rbkit_event_header *header = (rbkit_event_header *)event;
  header->event_type = event_type;

  event->hash = hash;
  return event;
}

rbkit_object_space_dump_event *new_rbkit_object_space_dump_event(rbkit_object_dump *dump) {
  rbkit_object_space_dump_event *event = malloc(sizeof(rbkit_object_space_dump_event));

  rbkit_event_header *header = (rbkit_event_header *)event;
  header->event_type = object_space_dump;

  event->dump = dump;
  return event;
}

rbkit_event_collection_event *new_rbkit_event_collection_event(void *buffer, size_t buffer_size, size_t message_count) {
  rbkit_event_collection_event *event = malloc(sizeof(rbkit_event_collection_event));

  rbkit_event_header *header = (rbkit_event_header *)event;
  header->event_type = event_collection;

  event->buffer = buffer;
  event->buffer_size = buffer_size;
  event->message_count = message_count;
  return event;
}
