#include "rbkit_object_tracer.h"

typedef struct _object_source {
  char *file;
  char *object_detail;
} object_source;


typedef struct _watched_object_sources {
  size_t count;
  object_source *sources[10]; // Allow watching only 10 objects for now
} watched_object_sources;

// Struct named `statics` contain the static variables used in this file.
// The values of this struct are initialized here.
static struct _object_tracer_statics {
  rbkit_map_t *location_map;
  rbkit_map_t *stacktrace_map;
  st_table *stacktrace_strings_stacktrace_ids_table;
  watched_object_sources *watched_object_sources;
} statics = {
  .location_map = NULL,
  .stacktrace_map = NULL,
  .stacktrace_strings_stacktrace_ids_table = NULL,
  .watched_object_sources = NULL
};

// This joins line, klass and size as a string delimited by a blank space
// Example: "20 String 38"
// Reason we do this: it's very common that objects of the same kind are usually
// created multiple times at the same location. An object's origin can usually be identified
// uniquely based on the line number, object type and the size of the object.
static char *get_object_signature(unsigned long line, const char *klass, size_t size) {
  const int n = snprintf(NULL, 0, "%lu %s %lu", line, klass, size);
  char *str = malloc(n + 1);;
  snprintf(str, n+1, "%lu %s %lu", line, klass, size);
  return str;
}

static int watching_object_allocation(const char *file, const char *object_detail) {
  size_t i;
  for (i = 0; i < statics.watched_object_sources->count; ++i) {
    if((strcmp(statics.watched_object_sources->sources[i]->file, file) == 0) &&
        (strcmp(statics.watched_object_sources->sources[i]->object_detail, object_detail) == 0)) {
      /*fprintf(stderr, "Watching %s -- %s\n", file, object_detail);*/
      return 1;
    }
  }
  return 0;
}

static void increment_stacktrace_count(rbkit_map_t *stacktraces_counts_map, rbkit_stack_trace *stacktrace) {
  st_table *table = stacktraces_counts_map->table;
  size_t n;

  if (st_lookup(table, (st_data_t)stacktrace, &n)) {
    st_insert(table, (st_data_t)stacktrace, n+1);
  }
  else {
    st_add_direct(table, (st_data_t)stacktrace, 1);
    stacktraces_counts_map->count++;
  }
}

static void collect_object_allocation_info(rbkit_map_t *files_object_signatures_map, const char *file, unsigned long line, const char *klass, size_t size) {
  st_table *table = files_object_signatures_map->table;
  char *key = get_object_signature(line, klass, size);
  int exists = 0;
  rbkit_object_allocation_details *allocation_details = NULL; //malloc(sizeof(object_allocation_details));

  if (!key) {
    return;
  }
  else {
    if (st_lookup(table, (st_data_t)key, (st_data_t *)&allocation_details)) {
      exists = 1;
    }
    else {
      allocation_details = malloc(sizeof(rbkit_object_allocation_details));
      allocation_details->count = 0;
      allocation_details->stacktraces_counts_map = new_num_map();

      st_add_direct(table, (st_data_t)key, (st_data_t)allocation_details);
      files_object_signatures_map->count++;
    }
    if(watching_object_allocation(file, key)) {
      increment_stacktrace_count(allocation_details->stacktraces_counts_map, collect_stack_trace());
    }
    allocation_details->count++;
  }
}

static void add_location_map(const char *file, unsigned long line, const char *klass, size_t size) {
  rbkit_map_t *files_object_signatures_map;
  char *new_string;
  size_t len = strlen(file);

  if (!file) {
    return;
  }
  else {
    if (st_lookup(statics.location_map->table,
          (st_data_t)file,
          (st_data_t *)&files_object_signatures_map)) {
      // Do nothing
    }
    else {
      new_string = (char *)malloc(len+1);
      memcpy(new_string, file, len);
      new_string[len] = 0;
      files_object_signatures_map = new_str_map();
      st_add_direct(statics.location_map->table, (st_data_t)new_string, (st_data_t)files_object_signatures_map);
      statics.location_map->count++;
    }
    collect_object_allocation_info(files_object_signatures_map, file, line, klass, size);
  }
}

static void init_stacktrace_map() {
  statics.stacktrace_map = malloc(sizeof(rbkit_map_t));
  statics.stacktrace_map->table = st_init_numtable();
  statics.stacktrace_strings_stacktrace_ids_table = st_init_strtable();
}

void init_object_tracer() {
  init_object_allocation_table();
  init_stacktrace_map();
  statics.watched_object_sources = malloc(sizeof(watched_object_sources));
  statics.watched_object_sources->count = 0;
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

int watch_object_source(const char *file, const char *object_detail) {
  watched_object_sources *sources = statics.watched_object_sources;
  object_source *source = NULL;
  size_t file_len = strlen(file);
  size_t obj_detail_len = strlen(object_detail);
  char *new_file_str;
  char *new_obj_detail_str;
  if(sources->count == 10) {
    return 0;
  } else {
    source = malloc(sizeof(object_source));

    new_file_str = malloc(file_len + 1);
    strncpy(new_file_str, file, file_len);
    new_file_str[file_len] = 0;

    new_obj_detail_str = malloc(obj_detail_len + 1);
    strncpy(new_obj_detail_str, object_detail, obj_detail_len);
    new_obj_detail_str[obj_detail_len] = 0;

    source->file = new_file_str;
    source->object_detail = new_obj_detail_str;

    sources->sources[sources->count++] = source;
    return 1;
  }
}
