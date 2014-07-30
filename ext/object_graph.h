#ifndef OBJECT_GRAPH
#define OBJECT_GRAPH

struct ObjectData {
  const void * object_id;
  const char * class_name;
  void ** references;
  size_t reference_count;
  char * file;
  unsigned long line;
  struct ObjectData *next;
};

struct ObjectDump {
  size_t size;
  struct ObjectData *first;
  struct ObjectData *last;
};

struct ObjectDump * get_object_dump();

struct allocation_info {
  const char *path;
  unsigned long line;
  const char *class_path;
  VALUE method_id;
  size_t generation;
};
#endif
