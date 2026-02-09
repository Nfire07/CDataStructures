#ifndef POINTERS_H
#define POINTERS_H

#include <stddef.h>
#include <stdbool.h>

bool null(void* ptr);
void* xMalloc(size_t size);
void* xCalloc(size_t nmemb,size_t size);
void* xRealloc(void* ptr, size_t size);
void* xShrinkRealloc(void* ptr, size_t size);
void xFree(void* ptr);

#endif