#include <string.h>
#include <stdlib.h>
#include "../include/maps.h"
#include "../include/pointers.h"
#include "../include/strings.h"

static uint32_t _fnv1a(const void* data, size_t size) {
    uint32_t hash = 2166136261u;
    const unsigned char* p = (const unsigned char*)data;
    for (size_t i = 0; i < size; i++) {
        hash ^= p[i];
        hash *= 16777619;
    }
    return hash;
}

uint32_t hashInt(const void* key, size_t size) {
    return _fnv1a(key, size);
}

uint32_t hashString(const void* key, size_t size) {
    (void)size;
    return _fnv1a(key, strlen((const char*)key));
}

bool keyEqualsInt(const void* k1, const void* k2, size_t size) {
    return memcmp(k1, k2, size) == 0;
}

bool keyEqualsString(const void* k1, const void* k2, size_t size) {
    (void)size;
    return strcmp((const char*)k1, (const char*)k2) == 0;
}

HashMap mapCreate(size_t keySize, size_t valueSize, size_t capacity) {
    HashMap map = (HashMap)xMalloc(sizeof(HashMapStruct));
    map->buckets = (MapEntry**)xCalloc(capacity, sizeof(MapEntry*));
    map->capacity = capacity;
    map->count = 0;
    map->keySize = keySize;
    map->valueSize = valueSize;
    map->hashFunc = hashInt; 
    map->keyEquals = keyEqualsInt;
    return map;
}

void mapPut(HashMap map, void* key, void* value) {
    if (!map) return;

    uint32_t hash = map->hashFunc(key, map->keySize);
    size_t index = hash % map->capacity;

    MapEntry* current = map->buckets[index];

    while (current) {
        if (map->keyEquals(key, current->key, map->keySize)) {
            if (value && map->valueSize > 0)
                memcpy(current->value, value, map->valueSize);
            return;
        }
        current = current->next;
    }

    MapEntry* newEntry = (MapEntry*)xMalloc(sizeof(MapEntry));
    newEntry->key = xMalloc(map->keySize);
    memcpy(newEntry->key, key, map->keySize);

    if (map->valueSize > 0 && value) {
        newEntry->value = xMalloc(map->valueSize);
        memcpy(newEntry->value, value, map->valueSize);
    } else {
        newEntry->value = NULL;
    }

    newEntry->next = map->buckets[index];
    map->buckets[index] = newEntry;
    map->count++;
}

void* mapGet(HashMap map, void* key) {
    if (!map) return NULL;
    uint32_t hash = map->hashFunc(key, map->keySize);
    size_t index = hash % map->capacity;

    MapEntry* current = map->buckets[index];
    while (current) {
        if (map->keyEquals(key, current->key, map->keySize)) {
            return current->value;
        }
        current = current->next;
    }
    return NULL;
}

void* mapGetKey(HashMap map, void* key) {
    if (!map) return NULL;
    uint32_t hash = map->hashFunc(key, map->keySize);
    size_t index = hash % map->capacity;

    MapEntry* current = map->buckets[index];
    while (current) {
        if (map->keyEquals(key, current->key, map->keySize)) {
            return current->key;
        }
        current = current->next;
    }
    return NULL;
}

bool mapContains(HashMap map, void* key) {
    return mapGetKey(map, key) != NULL;
}

void mapRemove(HashMap map, void* key, void (*keyFree)(void*), void (*valFree)(void*)) {
    if (!map) return;
    uint32_t hash = map->hashFunc(key, map->keySize);
    size_t index = hash % map->capacity;

    MapEntry* current = map->buckets[index];
    MapEntry* prev = NULL;

    while (current) {
        if (map->keyEquals(key, current->key, map->keySize)) {
            if (prev) {
                prev->next = current->next;
            } else {
                map->buckets[index] = current->next;
            }
            
            if (keyFree) keyFree(current->key);
            xFree(current->key);

            if (current->value) {
                if (valFree) valFree(current->value);
                xFree(current->value);
            }
            
            xFree(current);
            map->count--;
            return;
        }
        prev = current;
        current = current->next;
    }
}

void mapClear(HashMap map, void (*keyFree)(void*), void (*valFree)(void*)) {
    if (!map) return;
    for (size_t i = 0; i < map->capacity; i++) {
        MapEntry* current = map->buckets[i];
        while (current) {
            MapEntry* next = current->next;
            
            if (keyFree) keyFree(current->key);
            xFree(current->key);

            if (current->value) {
                if (valFree) valFree(current->value);
                xFree(current->value);
            }

            xFree(current);
            current = next;
        }
        map->buckets[i] = NULL;
    }
    map->count = 0;
}

void mapFree(HashMap map, void (*keyFree)(void*), void (*valFree)(void*)) {
    if (!map) return;
    mapClear(map, keyFree, valFree);
    xFree(map->buckets);
    xFree(map);
}

Set setCreate(size_t keySize, size_t capacity) {
    Set set = (Set)xMalloc(sizeof(SetStruct));
    set->map = mapCreate(keySize, 0, capacity);
    return set;
}

Set setFromArray(Array arr) {
    if (!arr) return NULL;

    size_t capacity = (arr->len > 0) ? (arr->len * 2) : 4;
    Set s = setCreate(arr->esize, capacity);

    for (size_t i = 0; i < arr->len; i++) {
        void* elem = arrayGetRef(arr, (int)i);
        setAdd(s, elem);
    }
    return s;
}

void setAdd(Set set, void* key) {
    if (set) mapPut(set->map, key, NULL);
}

void setRemove(Set set, void* key, void (*freeFn)(void*)) {
    if (set) mapRemove(set->map, key, freeFn, NULL);
}

bool setContains(Set set, void* key) {
    if (!set) return false;
    return mapContains(set->map, key);
}

void setFree(Set set, void (*freeFn)(void*)) {
    if (set) {
        mapFree(set->map, freeFn, NULL);
        xFree(set);
    }
}

void* setGet(Set set, void* key) {
    if (!set) return NULL;
    return mapGetKey(set->map, key);
}

Array setToArray(Set set) {
    if (!set) return NULL;
    Array arr = array(set->map->keySize);
    HashMap map = set->map;
    
    for (size_t i = 0; i < map->capacity; i++) {
        MapEntry* entry = map->buckets[i];
        while (entry) {
            arrayAdd(arr, entry->key);
            entry = entry->next;
        }
    }
    return arr;
}