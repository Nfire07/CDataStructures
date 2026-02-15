#ifndef MAPS_H
#define MAPS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "arrays.h" 

typedef struct MapEntry {
    void* key;
    void* value;
    struct MapEntry* next;
} MapEntry;

typedef struct {
    MapEntry** buckets;     
    size_t capacity;        
    size_t count;           
    size_t keySize;         
    size_t valueSize;       
    uint32_t (*hashFunc)(const void* key, size_t size);
    bool (*keyEquals)(const void* key1, const void* key2, size_t size);
} HashMapStruct;

typedef HashMapStruct* HashMap;

typedef struct {
    HashMap map;
} SetStruct;

typedef SetStruct* Set;

HashMap mapCreate(size_t keySize, size_t valueSize, size_t capacity);
void mapPut(HashMap map, void* key, void* value);
void* mapGet(HashMap map, void* key);
bool mapContains(HashMap map, void* key);
void mapRemove(HashMap map, void* key, void (*keyFree)(void*), void (*valFree)(void*));
void mapClear(HashMap map, void (*keyFree)(void*), void (*valFree)(void*));
void mapFree(HashMap map, void (*keyFree)(void*), void (*valFree)(void*));
void* mapGetKey(HashMap map, void* key);

uint32_t hashInt(const void* key, size_t size);
uint32_t hashString(const void* key, size_t size);
bool keyEqualsInt(const void* k1, const void* k2, size_t size);
bool keyEqualsString(const void* k1, const void* k2, size_t size);

Set setCreate(size_t keySize, size_t capacity);
Set setFromArray(Array arr);
void setAdd(Set set, void* key);
void setRemove(Set set, void* key, void (*freeFn)(void*));
bool setContains(Set set, void* key);
void setFree(Set set, void (*freeFn)(void*));
void* setGet(Set set, void* key);
Array setToArray(Set set);

#endif