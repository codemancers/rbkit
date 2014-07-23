#include <ruby.h>
#include "object_graph.h"

/*
 * The following categories of objects are directly accessible from the root:
 * ["vm", "machine_context", "symbols", "global_list", "end_proc", "global_tbl"]
 */
static void root_object_i(const char *category, VALUE obj, void *dump_data)
{
  struct ObjectData *data = (struct ObjectData *) malloc(sizeof(struct ObjectData));
  //Set object id
  data->object_id = (void *)obj;
  //Set classname
  data->class_name = NULL;
  //Set classname of "symbols" category as "Symbol" to match <symbol>.class output
  if(strcmp(category, "symbols") == 0){
    data->class_name = "Symbol" ;
  } else {
    data->class_name = category ;
  }
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
