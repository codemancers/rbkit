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
  OBJ_FREEZE(events);
  return events;
}

rbkit_obj_created_event *new_rbkit_obj_created_event(void *object_id,
    char *klass, rbkit_allocation_info *info) {
  rbkit_obj_created_event *event = malloc(sizeof(rbkit_obj_created_event));

  rbkit_event_header *header = event;
  header->event_type = obj_created;

  event->object_id = object_id;
  event->klass = klass;
  event->allocation_info = info;
  return event;
}

rbkit_obj_destroyed_event *new_rbkit_obj_destroyed_event(void *object_id) {
  rbkit_obj_destroyed_event *event = malloc(sizeof(rbkit_obj_destroyed_event));

  rbkit_event_header *header = event;
  header->event_type = obj_destroyed;

  event->object_id = object_id;
  return event;
}

rbkit_hash_event *new_rbkit_hash_event(rbkit_event_type event_type, VALUE hash) {
  rbkit_hash_event *event = malloc(sizeof(rbkit_hash_event));

  rbkit_event_header *header = event;
  header->event_type = event_type;

  event->hash = hash;
  return event;
}
