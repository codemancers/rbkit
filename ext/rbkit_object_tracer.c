#include "rbkit_object_tracer.h"

// Struct named `statics` contain the static variables used in this file.
// The values of this struct are initialized here.
struct _object_tracer_statics {
  rbkit_map_t *location_map;
} statics = {
  .location_map = NULL
};

// This joins line, klass and size as a string delimited by a blank space
// Example: "20 String 38"
// Reason we do this: it's very common that objects of the same kind are usually
// created multiple times at the same location. An object's origin can usually be identified
// uniquely based on the line number, object type and the size of the object.
static char *get_object_detail_string(unsigned long line, const char *klass, size_t size) {
  const int n = snprintf(NULL, 0, "%lu %s %lu", line, klass, size);
  char *str = malloc(n + 1);;
  snprintf(str, n+1, "%lu %s %lu", line, klass, size);
  return str;
}

// Increment the number of times an object was created by the same origin location
static void increment_object_count(rbkit_map_t *object_count_map, unsigned long line, const char *klass, size_t size) {
  st_table *table = object_count_map->table;
  st_data_t n;
  char *key = get_object_detail_string(line, klass, size);

  if (!key) {
    return;
  }
  else {
    if (st_lookup(table, (st_data_t)key, &n)) {
      st_insert(table, (st_data_t)key, n+1);
    }
    else {
      st_add_direct(table, (st_data_t)key, 1);
      object_count_map->count++;
    }
  }
}

static rbkit_map_t *new_object_count_map() {
  rbkit_map_t * object_count_map = malloc(sizeof(rbkit_map_t));
  object_count_map->table = st_init_strtable();
  object_count_map->count = 0;
  return object_count_map;
}

static void add_location_map(const char *file, unsigned long line, const char *klass, size_t size) {
  /*st_table *table = NULL;*/
  rbkit_map_t *obj_count_map;
  char *new_string;
  size_t len = strlen(file);

  if (!file) {
    return;
  }
  else {
    if (st_lookup(statics.location_map->table,
          (st_data_t)file,
          (st_data_t *)&obj_count_map)) {
      // Do nothing
    }
    else {
      new_string = (char *)malloc(len+1);
      strncpy(new_string, file, len);
      new_string[len] = 0;
      obj_count_map = new_object_count_map();//st_init_strtable();
      /*table = obj_count_map;*/
      st_add_direct(statics.location_map->table, (st_data_t)new_string, (st_data_t)obj_count_map);
      statics.location_map->count ++;
    }
    increment_object_count(obj_count_map, line, klass, size);
  }
}

void init_object_allocation_table() {
  statics.location_map = malloc(sizeof(rbkit_map_t));
  statics.location_map->table = st_init_strtable();
  statics.location_map->count = 0;
}


void add_new_object_info(rbkit_new_object_info *info) {
  add_location_map(info->file, info->line, info->klass, info->size);
}

rbkit_map_t *get_allocation_map() {
  return statics.location_map;
}
