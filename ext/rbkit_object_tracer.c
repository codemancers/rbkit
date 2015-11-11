#include "rbkit_object_tracer.h"



// Struct named `statics` contain the static variables used in this file.
// The values of this struct are initialized here.
struct _object_tracer_statics {
  rbkit_object_allocation_infos infos;
} statics = {
  .infos = { .count = 0 }
};

void push_new_object_allocation_info(rbkit_new_object_info *info) {
  /*if(object_allocation_info_full())*/
    /*return;*/

  statics.infos.info_list[statics.infos.count++] = info;
}

rbkit_object_allocation_infos *get_object_allocation_infos() {
  return &statics.infos;
}

int object_allocation_info_full() {
  return(statics.infos.count == MAX_NEW_OBJ_INFOS);
}
