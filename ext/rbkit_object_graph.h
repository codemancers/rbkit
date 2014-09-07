#ifndef RBKIT_OBJECT_GRAPH
#define RBKIT_OBJECT_GRAPH

#define RBKIT_OBJECT_DUMP_PAGE_SIZE 1000

typedef struct _rbkit_object_data {
  const void * object_id;
  const char * class_name;
  void ** references;
  size_t reference_count;
  char * file;
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
#endif
