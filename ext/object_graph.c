#include <ruby.h>
#include "object_graph.h"

struct ObjectData * initialize_object_data()
{
  struct ObjectData *data = (struct ObjectData *) malloc(sizeof(struct ObjectData));
  data->next = NULL;
  data->references = NULL;
  data->class_name = NULL;
  data->reference_count = 0;
  return data;
}

static void dump_root_object(VALUE obj, const char* category, struct ObjectDump * dump) {
  struct ObjectData *data = initialize_object_data();

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

static void reachable_object_i(VALUE ref, struct ObjectData *data)
{
  if(RBASIC_CLASS(ref) == ref)
    return;

  data->reference_count ++;
  if (data->reference_count == 1) {
    data->references = malloc(sizeof(void *));
  } else {
    data->references = realloc(data->references, data->reference_count * sizeof(void *) );
  }
  data->references[data->reference_count - 1] = (void *)ref;
}

static void dump_heap_object(VALUE obj, struct ObjectDump * dump) {

  struct ObjectData *data = initialize_object_data();
  //Set object id
  data->object_id = (void *)obj;

  //Set classname
  VALUE klass = RBASIC_CLASS(obj);
  data->class_name = rb_class2name(klass);

  //Set references
  rb_objspace_reachable_objects_from(obj, reachable_object_i, data);

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

/*
 * The following categories of objects are directly accessible from the root:
 * ["vm", "machine_context", "symbols", "global_list", "end_proc", "global_tbl"]
 */
static void root_object_i(const char *category, VALUE obj, void *dump_data)
{
  dump_root_object(obj, category, (struct ObjectDump *)dump_data);
}

/*
 * Iterator that walks over heap pages
 */
static int heap_obj_i(void *vstart, void *vend, size_t stride, void *dump_data)
{
  VALUE obj = (VALUE)vstart;
  VALUE klass ;

  for (; obj != (VALUE)vend; obj += stride) {
    klass = RBASIC_CLASS(obj);
    if (!NIL_P(klass) && BUILTIN_TYPE(obj) != T_NONE && BUILTIN_TYPE(obj) != T_ZOMBIE && BUILTIN_TYPE(obj) != T_ICLASS) {
      dump_heap_object(obj, (struct ObjectDump *)dump_data);
    }
  }
  return 0;
}


static void collect_root_objects(struct ObjectDump * dump) {
  rb_objspace_reachable_objects_from_root(root_object_i, (void *)dump);
}

static void collect_heap_objects(struct ObjectDump * dump) {
  rb_objspace_each_objects(heap_obj_i, (void *)dump);
}

struct ObjectDump * get_object_dump() {
  struct ObjectDump * dump = (struct ObjectDump *) malloc(sizeof(struct ObjectDump));
  dump->first = NULL;
  dump->last = NULL;
  collect_root_objects(dump);
  collect_heap_objects(dump);
  return dump;
}
