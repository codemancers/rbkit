#include <ruby.h>
#include "object_graph.h"

static void root_object_i(const char *category, VALUE obj, void *dump_data)
{
  struct ObjectData *data = (struct ObjectData *) malloc(sizeof(struct ObjectData));
  data->object_id = (void *)obj;
  data->next = NULL;

  struct ObjectDump * dump = (struct ObjectDump *) dump_data;
  if(dump->first == NULL) {
    dump->first = data;
    dump->last = data;
    dump->size = 1;
  } else {
    dump->last->next = data;
    dump->last = data;
    dump->size++;
  }
}

static VALUE collect_root_objects(struct ObjectDump * dump) {
  rb_objspace_reachable_objects_from_root(root_object_i, (void *)dump);
  return Qnil;
}

struct ObjectDump * get_object_dump() {
  struct ObjectDump * dump = (struct ObjectDump *) malloc(sizeof(struct ObjectDump));
  dump->first = NULL;
  dump->last = NULL;
  collect_root_objects(dump);
  //TODO: collect_heap_objects(dump);
  return dump;
}
