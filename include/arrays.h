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
Array arrayFromPtr(void* ptr, size_t len, size_t esize);
void* arrayGetRef(Array arr, int index);
void* arrayGetCpy(Array arr, int index);
void arrayGrow(Array arr);
void arrayShrink(Array arr);
void arrayAdd(Array arr, void* e);
void arrayRemoveAt(Array arr, int index);
void arrayInsert(Array arr, int index, void* e);
void arraySet(Array arr, int index, void* e);
void arrayClear(Array arr);
void arrayFree(Array arr);
void arraySort(Array arr, SortComparator cmp);
int SORT_INT_ASC(const void *a, const void *b);
int SORT_INT_DESC(const void *a, const void *b);
int SORT_CHAR_ASC(const void *a, const void *b);
int SORT_CHAR_DESC(const void *a, const void *b);
int SORT_FLOAT_ASC(const void *a, const void *b);
int SORT_FLOAT_DESC(const void *a, const void *b);
int SORT_DOUBLE_ASC(const void *a, const void *b);
int SORT_DOUBLE_DESC(const void *a, const void *b);
int SORT_CSTRING_ASC(const void *a, const void *b);
int SORT_CSTRING_DESC(const void *a, const void *b);
int SORT_STRING_ASC(const void *a, const void *b);
int SORT_STRING_DESC(const void *a, const void *b);

#endif