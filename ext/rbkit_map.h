#ifndef RBKIT_MAP
#define RBKIT_MAP
#include <stddef.h>
#include "ruby/st.h"

typedef struct _rbkit_map {
  st_table *table;
  size_t count;
} rbkit_map_t;

rbkit_map_t * new_str_map();
rbkit_map_t * new_num_map();
void free_map(rbkit_map_t *);

#endif

