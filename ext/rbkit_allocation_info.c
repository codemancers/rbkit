#include "rbkit_allocation_info.h"

/*
 * make_unique_str helps to reuse memory by allocating memory for a string
 * only once and keeping track of how many times that string is referenced.
 * It does so by creating a map of strings to their no of references.
 * A new map is created for a string on its first use, and for further usages
 * the reference count is incremented.
 */
static const char * make_unique_str(st_table *tbl, const char *str, long len) {
  if (!str) {
    return NULL;
  }
  else {
    st_data_t n;
    char *result;

    if (st_lookup(tbl, (st_data_t)str, &n)) {
      st_insert(tbl, (st_data_t)str, n+1);
      st_get_key(tbl, (st_data_t)str, (st_data_t *)&result);
    }
    else {
      result = (char *)malloc(len+1);
      strncpy(result, str, len);
      result[len] = 0;
      st_add_direct(tbl, (st_data_t)result, 1);
    }
    return result;
  }
}

/*
 * Used to free allocation of string when it's not referenced anymore.
 * Decrements the reference count of a string if it's still used, else
 * the map is removed completely.
 */
static void delete_unique_str(st_table *tbl, const char *str) {
  if (str) {
    st_data_t n;

    st_lookup(tbl, (st_data_t)str, &n);
    if (n == 1) {
      st_delete(tbl, (st_data_t *)&str, 0);
      free((char *)str);
    }
    else {
      st_insert(tbl, (st_data_t)str, n-1);
    }
  }
}

rbkit_allocation_info * new_rbkit_allocation_info(rb_trace_arg_t *tparg, st_table *str_table, st_table *object_table) {
  VALUE obj = rb_tracearg_object(tparg);
  VALUE klass = RBASIC_CLASS(obj);
  VALUE path = rb_tracearg_path(tparg);
  VALUE line = rb_tracearg_lineno(tparg);
  VALUE method_id = rb_tracearg_method_id(tparg);
  VALUE defined_klass = rb_tracearg_defined_class(tparg);

  rbkit_allocation_info *info;
  const char *path_cstr = RTEST(path) ? make_unique_str(str_table, RSTRING_PTR(path), RSTRING_LEN(path)) : 0;
  VALUE class_path = (RTEST(defined_klass) && !OBJ_FROZEN(defined_klass)) ? rb_class_path_cached(defined_klass) : Qnil;
  const char *class_path_cstr = RTEST(class_path) ? make_unique_str(str_table, RSTRING_PTR(class_path), RSTRING_LEN(class_path)) : 0;

  if (st_lookup(object_table, (st_data_t)obj, (st_data_t *)&info)) {
    /* reuse info */
    delete_unique_str(str_table, info->path);
    delete_unique_str(str_table, info->class_path);
  }
  else {
    info = (rbkit_allocation_info *)malloc(sizeof(rbkit_allocation_info));
  }

  info->path = path_cstr;
  info->line = NUM2INT(line);
  info->method_id = method_id;
  info->class_path = class_path_cstr;
  info->generation = rb_gc_count();
  st_insert(object_table, (st_data_t)obj, (st_data_t)info);
  return info;
}

void delete_rbkit_allocation_info(rb_trace_arg_t *tparg, VALUE obj, st_table *str_table, st_table *object_table) {
  rbkit_allocation_info *info;
  if (st_lookup(object_table, (st_data_t)obj, (st_data_t *)&info)) {
    st_delete(object_table, (st_data_t *)&obj, (st_data_t *)&info);
    delete_unique_str(str_table, info->path);
    delete_unique_str(str_table, info->class_path);
    free(info);
  }
}
