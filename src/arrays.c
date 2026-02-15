#include "../include/arrays.h"
#include "../include/pointers.h"
#include "../include/strings.h" 
#include <limits.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

_Static_assert(CHAR_BIT == 8, "Unsupported Platform");
#define MIN_CAPACITY 4

Array array(size_t esize){
    Array arr = xMalloc(sizeof(ArrayStruct));
    arr->esize = esize;
    arr->len = 0;
    arr->capacity = MIN_CAPACITY;
    arr->data = xMalloc(arr->capacity * arr->esize);
    return arr;
}

Array arrayFromPtr(void* ptr, size_t len, size_t esize) {
    if (null(ptr) && len > 0) return NULL;
    
    Array arr = xMalloc(sizeof(ArrayStruct));
    arr->esize = esize;
    arr->len = len;
    
    arr->capacity = (len < MIN_CAPACITY) ? MIN_CAPACITY : len;
    
    arr->data = xMalloc(arr->capacity * arr->esize);
    
    if (len > 0) {
        memcpy(arr->data, ptr, len * esize);
    }
    
    return arr;
}

void arrayGrow(Array arr) {
    if (null(arr)) return;
    if (arr->capacity >= INT_MAX) return;
    
    size_t newCapacity = (arr->capacity == 0) ? MIN_CAPACITY : arr->capacity * 2;
    if (newCapacity > INT_MAX) {
        newCapacity = INT_MAX;
    }
    
    size_t newSize = newCapacity * arr->esize;
    void* newData = xRealloc(arr->data, newSize);
    if (!null(newData)) {
        arr->data = newData;
        arr->capacity = newCapacity;
    }
}

void arrayShrink(Array arr) {
    if (null(arr) || null(arr->data)) return;
    size_t newCapacity = arr->len;
    
    if (newCapacity < MIN_CAPACITY) {
        newCapacity = MIN_CAPACITY;
    }
    if (newCapacity >= arr->capacity) return;
    
    size_t newByteSize = newCapacity * arr->esize;
    void* newData = xShrinkRealloc(arr->data, newByteSize);
    if (!null(newData)) {
        arr->data = newData;
        arr->capacity = newCapacity;
    }
}

void* arrayGetRef(Array arr, int index) {
    if (null(arr) || null(arr->data)) return NULL;
    if (index < 0) {
        if ((size_t)(-index) > arr->len) return NULL;
    } 
    else {
        if ((size_t)index >= arr->len) return NULL;
    }
    size_t realIndex = (index >= 0) ? (size_t)index : arr->len + index;
    return (unsigned char*)arr->data + realIndex * arr->esize;
}

void* arrayGetCpy(Array arr, int index){
    void* ref = arrayGetRef(arr, index);
    if(null(ref)) return NULL;
    void* ret = xMalloc(arr->esize);
    memcpy(ret, ref, arr->esize);
    return ret;
}

void arrayAdd(Array arr, void* e){
    if (null(arr) || null(arr->data) || null(e)) return;
    if (arr->len >= arr->capacity) {
        arrayGrow(arr);
        if (arr->len >= arr->capacity) return;
    }
    unsigned char* target = (unsigned char*)arr->data + (arr->len * arr->esize);
    memcpy(target, e, arr->esize);
    arr->len++;
}

void arrayRemoveAt(Array arr, int index) {
    if (null(arr) || null(arr->data)) return;
    if (index < 0) {
        if ((size_t)(-index) > arr->len) return;
    } else {
        if ((size_t)index >= arr->len) return;
    }
    size_t realIndex = (index >= 0) ? (size_t)index : arr->len + index;
    if (realIndex < arr->len - 1) {
        unsigned char* target = (unsigned char*)arr->data + realIndex * arr->esize;
        unsigned char* next = target + arr->esize;
        size_t bytesToMove = (arr->len - 1 - realIndex) * arr->esize;
        memmove(target, next, bytesToMove);
    }
    arr->len--;
}

void arrayInsert(Array arr, int index, void* e) {
    if (null(arr) || null(arr->data) || null(e)) return;
    if (index < 0) {
        if ((size_t)(-index) > arr->len) return;
    } else {
        if ((size_t)index > arr->len) return;
    }
    if (arr->len >= arr->capacity) {
        arrayGrow(arr);
        if (arr->len >= arr->capacity) return;
    }
    size_t realIndex = (index >= 0) ? (size_t)index : arr->len + index;
    unsigned char* target = (unsigned char*)arr->data + realIndex * arr->esize;

    if (realIndex < arr->len) {
        size_t bytesToMove = (arr->len - realIndex) * arr->esize;
        memmove(target + arr->esize, target, bytesToMove);
    }
    memcpy(target, e, arr->esize);
    arr->len++;
}

void arraySet(Array arr, int index, void* e) {
    if (null(arr) || null(arr->data) || null(e)) return;
    void* target = arrayGetRef(arr, index);
    if (!null(target)) {
        memcpy(target, e, arr->esize);
    }
}

void arrayClear(Array arr) {
    if (null(arr)) return;
    arr->len = 0;
}

void arrayFree(Array arr, void (*freeFunc)(void*)) {
    if (null(arr)) return;

    if (!null(freeFunc) && !null(arr->data)) {
        for (size_t i = 0; i < arr->len; i++) {
            void* itemAddr = (char*)arr->data + (i * arr->esize);
            freeFunc(itemAddr);
        }
    }

    xFree(arr->data);
    xFree(arr);
}

void arraySort(Array arr, SortComparator cmp) {
    if (null(arr) || null(arr->data) || null(cmp)) return;
    qsort(arr->data, arr->len, arr->esize, cmp);
}

int SORT_INT_ASC(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

int SORT_INT_DESC(const void *a, const void *b) {
    return (*(int*)b - *(int*)a);
}

int SORT_CHAR_ASC(const void *a, const void *b) {
    return (*(char*)a - *(char*)b);
}

int SORT_CHAR_DESC(const void *a, const void *b) {
    return (*(char*)b - *(char*)a);
}

int SORT_FLOAT_ASC(const void *a, const void *b) {
    return (*(float*)a > *(float*)b) - (*(float*)a < *(float*)b);
}

int SORT_FLOAT_DESC(const void *a, const void *b) {
    return (*(float*)b > *(float*)a) - (*(float*)b < *(float*)a);
}

int SORT_DOUBLE_ASC(const void *a, const void *b) {
    return (*(double*)a > *(double*)b) - (*(double*)a < *(double*)b);
}

int SORT_DOUBLE_DESC(const void *a, const void *b) {
    return (*(double*)b > *(double*)a) - (*(double*)b < *(double*)a);
}

int SORT_CSTRING_ASC(const void *a, const void *b) {
    return strcmp(*(char**)a, *(char**)b);
}

int SORT_CSTRING_DESC(const void *a, const void *b) {
    return strcmp(*(char**)b, *(char**)a);
}

int SORT_STRING_ASC(const void *a, const void *b) {
    String s1 = *(const String*)a;
    String s2 = *(const String*)b;
    return stringCompare(s1, s2);
}

int SORT_STRING_DESC(const void *a, const void *b) {
    String s1 = *(const String*)a;
    String s2 = *(const String*)b;
    return stringCompare(s2, s1);
}