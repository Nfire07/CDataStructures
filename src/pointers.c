#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include "../include/pointers.h"

#define ALIGNMENT 16
#define MAX_SMALL_SIZE 1024
#define PAGE_SIZE 4096
#define BLOCK_REFILL_COUNT 16
#define MAX_LOCAL_CACHE_SIZE 64
#define NUM_SIZE_CLASSES 7

typedef struct FreeNode {
    struct FreeNode* next;
} FreeNode;

typedef struct {
    size_t totalSize;
    uint32_t isLarge;
    uint32_t magic;
} BlockHeader;

typedef struct {
    FreeNode* freeLists[NUM_SIZE_CLASSES];
    int listCounts[NUM_SIZE_CLASSES];
    long allocCount;
    int initialized;
} ThreadCache;

typedef struct {
    FreeNode* freeLists[NUM_SIZE_CLASSES];
    pthread_mutex_t lock;
} GlobalHeap;

static __thread ThreadCache localCache = {0};
static GlobalHeap globalHeap = { .lock = PTHREAD_MUTEX_INITIALIZER };

static inline int getSizeClassIndex(size_t size) {
    if (size <= 16) return 0;
    if (size <= 32) return 1;
    if (size <= 64) return 2;
    if (size <= 128) return 3;
    if (size <= 256) return 4;
    if (size <= 512) return 5;
    if (size <= 1024) return 6;
    return -1;
}

static inline size_t getClassSize(int index) {
    return 16 << index;
}

static void* sysAlloc(size_t size) {
    void* p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return (p == MAP_FAILED) ? NULL : p;
}

static void sysFree(void* ptr, size_t size) {
    munmap(ptr, size);
}

static void refillThreadCache(int classIdx) {
    size_t size = getClassSize(classIdx);
    pthread_mutex_lock(&globalHeap.lock);
    
    FreeNode* current = globalHeap.freeLists[classIdx];
    int count = 0;
    while (current && count < BLOCK_REFILL_COUNT) {
        FreeNode* next = current->next;
        current->next = localCache.freeLists[classIdx];
        localCache.freeLists[classIdx] = current;
        localCache.listCounts[classIdx]++;
        current = next;
        count++;
    }
    globalHeap.freeLists[classIdx] = current;
    pthread_mutex_unlock(&globalHeap.lock);

    if (count == 0) {
        size_t chunkSize = size * BLOCK_REFILL_COUNT;
        if (chunkSize < PAGE_SIZE) chunkSize = PAGE_SIZE;
        uint8_t* rawMem = (uint8_t*)sysAlloc(chunkSize);
        if (!rawMem) return;

        size_t blockTotalSize = size + sizeof(BlockHeader);
        size_t offset = 0;
        while (offset + blockTotalSize <= chunkSize) {
            BlockHeader* header = (BlockHeader*)(rawMem + offset);
            header->totalSize = classIdx;
            header->isLarge = 0;
            header->magic = 0xDEADBEEF;
            FreeNode* node = (FreeNode*)(header + 1);
            node->next = localCache.freeLists[classIdx];
            localCache.freeLists[classIdx] = node;
            localCache.listCounts[classIdx]++;
            offset += blockTotalSize;
        }
    }
}

bool null(void* ptr){
    return (ptr == NULL);
}

void* xMalloc(size_t size) {
    if (size == 0) return NULL;
    int classIdx = getSizeClassIndex(size);
    if (classIdx >= 0) {
        if (!localCache.initialized) localCache.initialized = 1;
        if (null(localCache.freeLists[classIdx])) {
            refillThreadCache(classIdx);
            if (null(localCache.freeLists[classIdx])) return NULL;
        }
        FreeNode* node = localCache.freeLists[classIdx];
        localCache.freeLists[classIdx] = node->next;
        localCache.listCounts[classIdx]--;
        return (void*)node;
    }
    size_t totalNeeded = size + sizeof(BlockHeader);
    BlockHeader* header = (BlockHeader*)sysAlloc(totalNeeded);
    if (!header) return NULL;
    header->totalSize = totalNeeded;
    header->isLarge = 1;
    header->magic = 0xDEADBEEF;
    return (void*)(header + 1);
}

void* xCalloc(size_t nmemb, size_t size) {
    if (nmemb == 0 || size == 0) return NULL;
    size_t totalSize = nmemb * size;
    if (size != 0 && totalSize / size != nmemb) return NULL;
    void* ptr = xMalloc(totalSize);
    if (ptr) memset(ptr, 0, totalSize);
    return ptr;
}

void* xRealloc(void* ptr, size_t size) {
    if (ptr == NULL) return xMalloc(size);
    if (size == 0) {
        xFree(ptr);
        return NULL;
    }

    BlockHeader* header = ((BlockHeader*)ptr) - 1;
    if (header->magic != 0xDEADBEEF) abort();

    size_t currentSize;
    if (header->isLarge) {
        currentSize = header->totalSize - sizeof(BlockHeader);
    } else {
        currentSize = getClassSize((int)header->totalSize);
    }

    if (size <= currentSize) return ptr;

    void* newPtr = xMalloc(size);
    if (newPtr) {
        memcpy(newPtr, ptr, currentSize);
        xFree(ptr);
    }
    
    return newPtr;
}

void* xShrinkRealloc(void* ptr, size_t size) {
    if (ptr == NULL) return xMalloc(size);
    if (size == 0) {
        xFree(ptr);
        return NULL;
    }

    BlockHeader* header = ((BlockHeader*)ptr) - 1;
    if (header->magic != 0xDEADBEEF) abort();

    size_t currentSize;
    if (header->isLarge) {
        currentSize = header->totalSize - sizeof(BlockHeader);
    } else {
        currentSize = getClassSize((int)header->totalSize);
    }

    if (size <= currentSize) {
        size_t threshold = currentSize / 2; 
        if (size > threshold) {
             return ptr;
        }
    }

    void* newPtr = xMalloc(size);
    if (newPtr) {
        size_t copySize = (currentSize < size) ? currentSize : size;
        memcpy(newPtr, ptr, copySize);
        xFree(ptr);
    }
    
    return newPtr;
}

void xFree(void* ptr) {
    if (!ptr) return;
    BlockHeader* header = ((BlockHeader*)ptr) - 1;
    if (header->magic != 0xDEADBEEF) abort();
    
    if (header->isLarge == 0) {
        int classIdx = (int)header->totalSize;
        FreeNode* node = (FreeNode*)ptr;
        
        node->next = localCache.freeLists[classIdx];
        localCache.freeLists[classIdx] = node;
        localCache.listCounts[classIdx]++;

        if (localCache.listCounts[classIdx] > MAX_LOCAL_CACHE_SIZE) {
            pthread_mutex_lock(&globalHeap.lock);
            for (int i = 0; i < BLOCK_REFILL_COUNT; i++) {
                FreeNode* toMove = localCache.freeLists[classIdx];
                if (!toMove) break;
                
                localCache.freeLists[classIdx] = toMove->next;
                toMove->next = globalHeap.freeLists[classIdx];
                globalHeap.freeLists[classIdx] = toMove;
                
                localCache.listCounts[classIdx]--;
            }
            pthread_mutex_unlock(&globalHeap.lock);
        }
        return;
    }
    sysFree(header, header->totalSize);
}
