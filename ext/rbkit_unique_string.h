#ifndef RBKIT_UNIQUE_STRING
#define RBKIT_UNIQUE_STRING

#include <stddef.h>
#include "ruby/st.h"

void init_unique_string_table();
void free_unique_string_table();
char *get_unique_string(const char *);
void free_unique_string(const char *);

#endif
