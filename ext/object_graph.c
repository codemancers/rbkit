#include <ruby.h>

static inline const char *
obj_type(VALUE obj)
{
  switch (BUILTIN_TYPE(obj)) {
#define CASE_TYPE(type) case T_##type: return #type; break
    CASE_TYPE(NONE);
    CASE_TYPE(NIL);
    CASE_TYPE(OBJECT);
    CASE_TYPE(CLASS);
    CASE_TYPE(ICLASS);
    CASE_TYPE(MODULE);
    CASE_TYPE(FLOAT);
    CASE_TYPE(STRING);
    CASE_TYPE(REGEXP);
    CASE_TYPE(ARRAY);
    CASE_TYPE(HASH);
    CASE_TYPE(STRUCT);
    CASE_TYPE(BIGNUM);
    CASE_TYPE(FILE);
    CASE_TYPE(FIXNUM);
    CASE_TYPE(TRUE);
    CASE_TYPE(FALSE);
    CASE_TYPE(DATA);
    CASE_TYPE(MATCH);
    CASE_TYPE(SYMBOL);
    CASE_TYPE(RATIONAL);
    CASE_TYPE(COMPLEX);
    CASE_TYPE(UNDEF);
    CASE_TYPE(NODE);
    CASE_TYPE(ZOMBIE);
#undef CASE_TYPE
  }
  return "UNKNOWN";
}

static void
root_obj_i(const char *category, VALUE obj, void *data)
{
  fprintf(stderr, "CATEGORY: %s, OBJ: %p\n", category, (void *)obj);
}


static void
print_class_using_class2name(VALUE self, VALUE obj)
{
  VALUE klass = RBASIC_CLASS(obj);
  if (!NIL_P(klass)) {
    fprintf(stderr, "Using class2name : %s\n", rb_class2name(klass));
  }
}

static int
heap_obj_i(void *vstart, void *vend, size_t stride, void *data)
{
  VALUE v = (VALUE)vstart;
  VALUE klass ;

  for (; v != (VALUE)vend; v += stride) {
    klass = RBASIC_CLASS(v);
    if (!NIL_P(klass) && BUILTIN_TYPE(v) != T_NONE && BUILTIN_TYPE(v) != T_ZOMBIE && BUILTIN_TYPE(v) != T_ICLASS) {
      fprintf(stderr, "POINTER -- : %p ; ", (void *)v);
      fprintf(stderr, "TYPE -- : %s ; ", obj_type(v));
      fprintf(stderr, "CLASS --- : %s\n", rb_class2name(klass));
    }
  }
  return 0;
}

struct some_shit {
  void * something;
};

static VALUE print_root_objects() {
  struct some_shit data;
  rb_objspace_reachable_objects_from_root(root_obj_i, &data);
  return Qnil;
}

static VALUE print_heap_objects() {
  struct some_shit data;
  rb_objspace_each_objects(heap_obj_i, &data);
  return Qnil;
}


void init_object_graph(void) {
  VALUE objectGraphModule = rb_define_module("ObjectGraph");
  rb_define_module_function(objectGraphModule, "print_root_objects", print_root_objects, 0);
  rb_define_module_function(objectGraphModule, "print_heap_objects", print_heap_objects, 0);
  rb_define_module_function(objectGraphModule, "print_class_using_class2name", print_class_using_class2name, 1);
}
