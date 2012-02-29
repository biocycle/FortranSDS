#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>

void *xmalloc(size_t bytes);
void *xcalloc(size_t bytes);
void *xrealloc(void *ptr, size_t bytes);

#define NEW(type) (type *)xmalloc(sizeof(type))
#define NEW0(type) (type *)xcalloc(sizeof(type))
#define NEWA(type,n) (type *)xmalloc(sizeof(type) * (n))

void *grow_ary(void *ary, size_t element_size, size_t *capacity);

#define GROW_IF_FULL(ary,num,cap,type) \
    if ((num) == (cap)) \
        ary = (type *)grow_ary(ary, sizeof(type), &(cap))

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#endif // UTIL_H
