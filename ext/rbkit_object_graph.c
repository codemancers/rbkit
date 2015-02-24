#include <ruby.h>
#include "rbkit_object_graph.h"

rbkit_object_dump_page * rbkit_object_dump_page_new() {
  rbkit_object_dump_page *page = malloc(sizeof(rbkit_object_dump_page));
  page->count = 0;
  page->next = NULL;
  return page;
}

rbkit_object_data * initialize_object_data(rbkit_object_dump *dump) {
  if(dump->first == NULL) {
    rbkit_object_dump_page *page = rbkit_object_dump_page_new();
    dump->first = page;
    dump->last = page;
    dump->page_count = 1;
    dump->object_count = 0;
  } else if (dump->last->count == RBKIT_OBJECT_DUMP_PAGE_SIZE) {
    rbkit_object_dump_page *page = rbkit_object_dump_page_new();
    dump->last->next = page;
    dump->last = page;
    dump->page_count++;
  }
  rbkit_object_data *data = &(dump->last->data[dump->last->count]);
  data->references = NULL;
  data->class_name = NULL;
  data->reference_count = 0;
  data->file = NULL;
  data->line = 0;
  data->size = 0;
  return(data);
}

static void set_size(VALUE obj, rbkit_object_data * data) {
  size_t size;
  if ((size = rb_obj_memsize_of(obj)) > 0)
    data->size = size;
}

static void reachable_object_i(VALUE ref, void *arg)
{
  rbkit_object_data *data = (rbkit_object_data *)arg;
  if(RBASIC_CLASS(ref) == ref)
    return;

  data->reference_count ++;
  if (data->reference_count == 1) {
    data->references = malloc(sizeof(unsigned long long));
  } else {
    data->references = realloc(data->references, data->reference_count * sizeof(unsigned long long) );
  }
  data->references[data->reference_count - 1] = FIX2ULONG(rb_obj_id(ref));
}

static void dump_heap_object(VALUE obj, rbkit_object_dump *dump) {

  // Get next available slot from page
  rbkit_object_data *data = initialize_object_data(dump);
  //Set object id
  data->object_id = FIX2ULONG(rb_obj_id(obj));

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
  dump->object_count++;
  dump->last->count++;
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
      dump_heap_object(obj, (rbkit_object_dump *)dump_data);
    }
  }
  return 0;
}

static void collect_heap_objects(rbkit_object_dump * dump) {
  rb_objspace_each_objects(heap_obj_i, (void *)dump);
}

rbkit_object_dump * get_object_dump(st_table * object_table) {
  rbkit_object_dump * dump = (rbkit_object_dump *) malloc(sizeof(rbkit_object_dump));
  dump->object_table = object_table;
  dump->first = NULL;
  dump->last = NULL;
  collect_heap_objects(dump);
  return dump;
}
