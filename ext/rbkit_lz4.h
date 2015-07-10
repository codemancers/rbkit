#ifndef RBKIT_LZ4_h
#define RBKIT_LZ4_h
#include "stddef.h"

int lz4_compress(char *source, char *dest, size_t size, int max_dest_size);
int lz4_max_compressed_length(int uncompressed_length);

void Init_rbkit_lz4();
#endif
