#include "rbkit_object_tracer.h"
#include "rbkit_unique_string.h"

static struct _object_tracer_statics {
  // Needed
  rbkit_map_t *files_signatures_map;
  /*rbkit_map_t *stacktraces_counts_map;*/
  st_table *object_ids_signatures_table;
  /*st_table *signatures_counts_table;*/
} statics = {
  .files_signatures_map = NULL,
  .object_ids_signatures_table = NULL
  /*.stacktraces_counts_map = NULL,*/
  /*.stacktrace_strings_stacktrace_ids_table = NULL,*/
  /*.object_ids_details_map = NULL,*/
  /*.watched_object_sources = NULL*/
};

static rbkit_object_signature *create_object_signature(const char *file, const unsigned long line, const char *klass, const size_t size) {
  rbkit_object_signature *signature = malloc(sizeof(rbkit_object_signature));
  signature->file = get_unique_string(file);
  signature->line = line;
  signature->klass = get_unique_string(klass);
  signature->size = size;
  return signature;
}

static void free_object_signature(rbkit_object_signature *signature) {
  free_unique_string(signature->file);
  free_unique_string(signature->klass);
  free(signature);
}

typedef struct _find_signature_arg {
  rbkit_object_signature *existing_signature;
  rbkit_object_signature *new_signature;
} find_signature_arg;

static int compare_strings(char *s1, char *s2) {
  if(s1 && s2) {
    return (strcmp(s1, s2) == 0);
  } else {
    // True if both are NULL
    return (s1 == s2);
  }
}

static int same_signatures(rbkit_object_signature *signature1, rbkit_object_signature *signature2) {
  if(
      (signature1->line != signature2->line) ||
      (signature1->size != signature2->size) ||
      !compare_strings(signature1->file, signature2->file) ||
      !compare_strings(signature1->klass, signature2->klass)
    ) {
    return 0;
  }
  return 1;
}

static int get_similar_signature_i(st_data_t key, st_data_t value, st_data_t arg) {
  find_signature_arg *find_arg = (find_signature_arg *)arg;
  rbkit_object_signature *existing_signature = (rbkit_object_signature *)key;
  if(same_signatures(existing_signature, find_arg->new_signature)) {
    find_arg->existing_signature = existing_signature;
    return ST_STOP;
  }
  return ST_CONTINUE;
}

static void decrement_signature_count(rbkit_map_t *signatures_counts_map, rbkit_object_signature *signature) {
  size_t n;
  st_table *table = signatures_counts_map->table;
  if(signatures_counts_map->count == 0) {
    fprintf(stderr, "BUG! Object not traced properly %u\n", __LINE__);
  }

  if(st_lookup(table, (st_data_t)signature, (st_data_t *)&n)) {
    // TODO: Use signed long and allow n to become negative
    if(n == 1) {
      //Remove signature from table
      st_delete(table, (st_data_t *)&signature, 0);
      free_object_signature(signature);
      if(signatures_counts_map->count == 1) {
        st_delete(statics.files_signatures_map->table, (st_data_t *)&signatures_counts_map, 0);
        free_map(signatures_counts_map);
      } else {
        signatures_counts_map->count--;
      }
    } else {
      st_insert(table, (st_data_t)signature, n-1);
    }
  } else {
    fprintf(stderr, "BUG! Object not traced properly %u\n", __LINE__);
  }
}

rbkit_object_signature *find_or_create_object_signature(const unsigned long object_id,
    const char *file, const unsigned long line, const char *klass, const size_t size) {
  size_t n;
  rbkit_object_signature *signature = create_object_signature(file, line, klass, size);
  rbkit_map_t *signatures_counts_map = NULL;
  st_table *table;
  if(st_lookup(statics.files_signatures_map->table, (st_data_t)file, (st_data_t *)&signatures_counts_map)) {
    //go through keys, if similar, increment count of signatures_counts_map
    find_signature_arg *arg = malloc(sizeof(find_signature_arg));
    arg->new_signature = signature;
    arg->existing_signature = NULL;
    st_foreach(signatures_counts_map->table, get_similar_signature_i, (st_data_t)arg);
    if(arg->existing_signature) {
      free_object_signature(signature);
      signature = arg->existing_signature;
    }
    free(arg);
  } else {
    signatures_counts_map = new_num_map();
    st_add_direct(statics.files_signatures_map->table, (st_data_t)signature->file, (st_data_t)signatures_counts_map);
  }

  table = signatures_counts_map->table;
  if (st_lookup(table, (st_data_t)signature, &n)) {
    st_insert(table, (st_data_t)signature, n+1);
  } else {
    st_add_direct(table, (st_data_t)signature, 1);
    signatures_counts_map->count++;
  }

  // Insert into {object_id => object_signature} table
  st_add_direct(statics.object_ids_signatures_table, (st_data_t)object_id, (st_data_t)signature);
  return signature;
}

void delete_object(unsigned long object_id) {
  rbkit_object_signature *signature;
  rbkit_map_t *signatures_counts_map;
  if(st_lookup(statics.object_ids_signatures_table, (st_data_t)object_id, (st_data_t *)&signature)) {
    if(st_lookup(statics.files_signatures_map->table, (st_data_t)signature->file, (st_data_t *)&signatures_counts_map)) {
      decrement_signature_count(signatures_counts_map, signature);
    } else {
      fprintf(stderr, "BUG! Object not traced properly %u\n", __LINE__);
    }
    st_delete(statics.object_ids_signatures_table, (st_data_t *)&object_id, 0);
  }
}

void init_object_tracer() {
  statics.files_signatures_map = new_str_map();
  statics.object_ids_signatures_table = st_init_numtable();
}
