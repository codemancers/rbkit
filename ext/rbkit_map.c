#include "rbkit_map.h"

rbkit_map_t * new_str_map() {
  rbkit_map_t * map = malloc(sizeof(rbkit_map_t));
  map->table = st_init_strtable();
  map->count = 0;
  return map;
}

rbkit_map_t * new_num_map() {
  rbkit_map_t * map = malloc(sizeof(rbkit_map_t));
  map->table = st_init_numtable();
  map->count = 0;
  return map;
}

void free_map(rbkit_map_t *map) {
  map->count = 0;
  st_free_table(map->table);
  free(map);
}
