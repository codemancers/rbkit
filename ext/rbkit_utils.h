#ifndef RBKIT_UTILS_H
#define RBKIT_UTILS_H

#include <stdio.h>

static inline void* rbkit_malloc(size_t size)
{
    void* ptr = malloc(size);
    printf("malloc size %x:%d \n", ptr, size);
    return ptr;
}


static inline void* rbkit_realloc(void* oldptr, size_t size)
{
    void* ptr = realloc(oldptr, size);
    printf("realloc size %x:%d\n", ptr, size);
    return ptr;
}


#endif
