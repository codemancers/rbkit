#include "rbkit_unique_string.h"

// Create a static struct
static struct _string_table {
  // Needed
  st_table *string_table;
  /*st_table *signatures_counts_table;*/
} statics = {
  .string_table = NULL
};

static int free_string_i(st_data_t key, st_data_t value, st_data_t arg){
  char *str = (char *)key;
  free(str);
  return ST_DELETE;
}

void init_unique_string_table() {
  statics.string_table = st_init_strtable();
}

void free_unique_string_table() {
  st_table *tbl = statics.string_table;
  st_foreach(tbl, free_string_i, 0);
  st_free_table(tbl);
}

char *get_unique_string(const char * str) {
  size_t len = strlen(str);
  st_table *tbl = statics.string_table;
  st_data_t n;
  char *result;

  if (!str) {
    return NULL;
  }
  else {
    if (st_lookup(tbl, (st_data_t)str, &n)) {
      st_insert(tbl, (st_data_t)str, n+1);
      st_get_key(tbl, (st_data_t)str, (st_data_t *)&result);
    }
    else {
      result = (char *)malloc(len+1);
      memcpy(result, str, len);
      result[len] = 0;
      st_add_direct(tbl, (st_data_t)result, 1);
    }
    return result;
  }
}
void free_unique_string(const char *str) {
  // Decrement count/delete from table
  st_table *tbl = statics.string_table;
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
