#ifndef OBJECT_GRAPH
#define OBJECT_GRAPH

struct ObjectData {
  const void * object_id;
  const char * class_name;
  void ** references;
  char * file;
  int line;
  struct ObjectData *next;
};

struct ObjectDump {
  size_t size;
  struct ObjectData *first;
  struct ObjectData *last;
};

struct ObjectDump * get_object_dump();
#endif
