#ifndef RBKIT_OBJECT_GRAPH
#define RBKIT_OBJECT_GRAPH
#include <ruby.h>

#define RBKIT_OBJECT_DUMP_PAGE_SIZE 1000

typedef struct _rbkit_object_data {
  unsigned long long object_id;
  const char * class_name;
  unsigned long long * references;
  size_t reference_count;
  const char * file;
  unsigned long line;
  size_t size;
} rbkit_object_data;

typedef struct _rbkit_object_dump_page {
  rbkit_object_data data[RBKIT_OBJECT_DUMP_PAGE_SIZE];
  size_t count;
  struct _rbkit_object_dump_page *next;
} rbkit_object_dump_page;

typedef struct _rbkit_object_dump {
  size_t page_count;
  size_t object_count;
  rbkit_object_dump_page *first;
  rbkit_object_dump_page *last;
  st_table * object_table;
} rbkit_object_dump;

rbkit_object_dump * get_object_dump();

struct allocation_info {
  const char *path;
  unsigned long line;
  const char *class_path;
  VALUE method_id;
  size_t generation;
};

// Declarations of ruby internals to silence warnings
void rb_objspace_reachable_objects_from(VALUE obj, void (func)(VALUE, void *), void *data);
void rb_objspace_reachable_objects_from_root(void (func)(const char *category, VALUE, void *), void *data);
size_t rb_obj_memsize_of(VALUE);
void rb_objspace_each_objects(
    int (*callback)(void *start, void *end, size_t stride, void *data),
    void *data);
#endif
