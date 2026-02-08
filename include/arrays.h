#ifndef ARRAYS_H
#define ARRAYS_H
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    void* data;
    size_t esize;
    size_t len;
    size_t capacity;
} ArrayStruct;

typedef ArrayStruct* Array;

typedef int (*SortComparator)(const void*, const void*);

Array array(size_t esize);


#endif