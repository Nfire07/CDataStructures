#include "../include/arrays.h"
#include "../include/pointers.h"
#define MIN_CAPACITY 4

Array array(size_t esize){
    Array arr = xMalloc(sizeof(ArrayStruct));
    arr->esize = esize;
    arr->len = 0;
    arr->capacity = MIN_CAPACITY;
    arr->data = xMalloc(arr->capacity * arr->esize);
    return arr;
}
