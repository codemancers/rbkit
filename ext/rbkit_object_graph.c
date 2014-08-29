#include <ruby.h>
#include "rbkit_object_graph.h"
#include "rbkit_utils.h"

struct ObjectData * initialize_object_data() {
  struct ObjectData *data = (struct ObjectData *) rbkit_malloc(sizeof(struct ObjectData));
  data->next = NULL;
  data->references = NULL;
  data->class_name = NULL;
  data->reference_count = 0;
  data->file = NULL;
  data->line = 0;
  data->size = 0;
  return data;
}

static void set_size(VALUE obj, struct ObjectData * data) {
  size_t size;
  if ((size = rb_obj_memsize_of(obj)) > 0)
    data->size = size;
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

  //Set file path and line no where object is defined
  struct allocation_info *info;
  if (st_lookup(dump->object_table, obj, (st_data_t *)&info)) {
    if(info) {
      data->file = info->path;
      data->line = info->line;
    }
  }

  set_size(obj, data);

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
    data->references = rbkit_malloc(sizeof(void *));
  } else {
    data->references = rbkit_realloc(data->references, data->reference_count * sizeof(void *) );
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

  //Set file path and line no where object is defined
  struct allocation_info *info;
  if (st_lookup(dump->object_table, obj, (st_data_t *)&info)) {
    if(info) {
      data->file = info->path;
      data->line = info->line;
    }
  }

  set_size(obj, data);

  //Put it in the linked list
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

struct ObjectDump * get_object_dump(st_table * object_table) {
  struct ObjectDump * dump = (struct ObjectDump *) rbkit_malloc(sizeof(struct ObjectDump));
  dump->object_table = object_table;
  dump->first = NULL;
  dump->last = NULL;
  collect_root_objects(dump);
  collect_heap_objects(dump);
  return dump;
}
