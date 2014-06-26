#include "ruby/ruby.h"
#include "ruby/debug.h"

static void
root_obj_i(const char *category, VALUE obj, void *data)
{
  fprintf(stderr, "CATEGORY: %s, OBJ: %p\n", category, (void *)obj);
}

struct some_shit {
  void * something;
};

static VALUE print_root_objects() {
  struct some_shit data;
  rb_objspace_reachable_objects_from_root(root_obj_i, &data);
  return Qnil;
}

void init_object_graph(void) {
  VALUE objectGraphModule = rb_define_module("ObjectGraph");
  rb_define_module_function(objectGraphModule, "print_root_objects", print_root_objects, 0);
}
