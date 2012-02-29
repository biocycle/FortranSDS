/* util.c - Utility functions for SDS converter.
 * Copyright (C) 2011 Matthew Bishop
 */
#include "util.h"
#include <stdio.h>

void *xmalloc(size_t bytes)
{
    void *p = malloc(bytes);
    if (!p) {
        fprintf(stderr, "Failed to malloc(%u)\n", (unsigned)bytes);
        abort();
    }
    return p;
}

void *xcalloc(size_t bytes)
{
    void *p = calloc(1, bytes);
    if (!p) {
        fprintf(stderr, "Failed to calloc(1, %u)\n", (unsigned)bytes);
        abort();
    }
    return p;
}

void *xrealloc(void *ptr, size_t bytes)
{
    void *p = realloc(ptr, bytes);
    if (!p) {
        fprintf(stderr, "Failed to realloc(%p, %u)\n", ptr, (unsigned)bytes);
        abort();
    }
    return p;
}

/* Grows the given array by doubling its capacity.  Returns the
 * pointer to the new memory block.  ary: the array to grow.
 * element_size: the size of each element in the array.  capacity: an
 * in/out parameter containing the current capacity of the array.  It
 * will be doubled before the function returns.
 */
void *grow_ary(void *ary, size_t element_size, size_t *capacity)
{
    if (*capacity == 0)
        *capacity = 1;
    else
        *capacity *= 2;
    return realloc(ary, element_size * *capacity);
}
