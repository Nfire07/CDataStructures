#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <stdatomic.h>
#include <sched.h>

#if defined(__linux__) && defined(XALLOC_NUMA)
#include <numa.h>
#include <numaif.h>
#endif

#ifdef __linux__

#endif

#define ALIGNMENT              16
#define CACHE_LINE             64
#define MAX_SMALL_SIZE         1024
#define BLOCK_REFILL_COUNT     16
#define MAX_LOCAL_CACHE_SIZE   64
#define NUM_SIZE_CLASSES       7
#define MAX_ARENAS             64
#define TRANSFER_CACHE_SLOTS   32
#define HUGE_PAGE_THRESHOLD    (2u * 1024 * 1024)
#define QUARANTINE_SIZE        128

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define PAD_TO(sz) \
    char _pad[((sz) % CACHE_LINE) ? (CACHE_LINE - ((sz) % CACHE_LINE)) : 0]


static size_t PAGE_SIZE      = 4096;
static size_t HUGE_PAGE_SIZE = 2u * 1024 * 1024;

__attribute__((constructor))
static void init_page_sizes(void) {
    PAGE_SIZE      = (size_t)sysconf(_SC_PAGESIZE);
    HUGE_PAGE_SIZE = 2u * 1024 * 1024;
}


typedef struct __attribute__((aligned(16))) {
    size_t   totalSize;
    uint32_t isLarge;
    uint32_t magic;
} BlockHeader;

_Static_assert(sizeof(BlockHeader) % ALIGNMENT == 0,
               "BlockHeader must be a multiple of ALIGNMENT");


typedef struct FreeNode {
    struct FreeNode* next;
} FreeNode;


typedef struct Slab {
    struct Slab* next;
    size_t       freeCount;
    size_t       totalCount;
    int          classIdx;
    int          numaNode;
    FreeNode*    freeList;
} Slab;


typedef struct LargeBlock {
    size_t             size;
    int                numaNode;
    struct LargeBlock* next;
} LargeBlock;


typedef struct {
    atomic_size_t allocs;
    atomic_size_t frees;
    atomic_size_t currentUsage;
    atomic_size_t slabsCreated;
    atomic_size_t slabsReclaimed;
    PAD_TO(5 * sizeof(atomic_size_t));
} SizeClassStats;


typedef struct {
    atomic_size_t totalAllocated;
    atomic_size_t totalFreed;
    atomic_size_t currentUsage;
    atomic_size_t mmapCalls;
    atomic_size_t munmapCalls;
    atomic_size_t largeReuses;
    atomic_size_t hugePagesUsed;
    SizeClassStats perClass[NUM_SIZE_CLASSES];
} AllocStats;

static AllocStats gStats = {0};

typedef struct {
    _Alignas(CACHE_LINE) atomic_uintptr_t slots[TRANSFER_CACHE_SLOTS];
    _Alignas(CACHE_LINE) atomic_int       head;
    _Alignas(CACHE_LINE) atomic_int       tail;
} TransferCache;

static inline bool tc_push(TransferCache* tc, FreeNode* node) {
    int tail = atomic_load_explicit(&tc->tail, memory_order_relaxed);
    int next = (tail + 1) % TRANSFER_CACHE_SLOTS;
    if (next == atomic_load_explicit(&tc->head, memory_order_acquire))
        return false;
    atomic_store_explicit(&tc->slots[tail], (uintptr_t)node, memory_order_relaxed);
    atomic_store_explicit(&tc->tail, next, memory_order_release);
    return true;
}

static inline FreeNode* tc_pop(TransferCache* tc) {
    int head = atomic_load_explicit(&tc->head, memory_order_relaxed);
    if (head == atomic_load_explicit(&tc->tail, memory_order_acquire))
        return NULL;
    FreeNode* node = (FreeNode*)atomic_load_explicit(&tc->slots[head],
                                                      memory_order_relaxed);
    atomic_store_explicit(&tc->head,
                          (head + 1) % TRANSFER_CACHE_SLOTS,
                          memory_order_release);
    return node;
}

typedef struct {
    _Alignas(CACHE_LINE)
    FreeNode*        freeLists[NUM_SIZE_CLASSES];
    int              listCounts[NUM_SIZE_CLASSES];
    Slab*            slabs[NUM_SIZE_CLASSES];
    TransferCache    tc[NUM_SIZE_CLASSES];
    pthread_mutex_t  lock;
    int              numaNode;
    int              arenaId;
    long             allocCount;
    int              initialized;
    PAD_TO(sizeof(FreeNode*) * NUM_SIZE_CLASSES +
           sizeof(int)       * NUM_SIZE_CLASSES +
           sizeof(Slab*)     * NUM_SIZE_CLASSES +
           sizeof(TransferCache) * NUM_SIZE_CLASSES +
           sizeof(pthread_mutex_t) +
           3 * sizeof(int) + sizeof(long));
} Arena;


#define ARENA_REGISTRY_SIZE (MAX_ARENAS + 1)

static Arena         gArenas[MAX_ARENAS];
static atomic_int    gArenaCount = 0;
static __thread Arena* localArena = NULL;


static LargeBlock*     largeFreeList = NULL;
static pthread_mutex_t largeLock     = PTHREAD_MUTEX_INITIALIZER;


#ifdef XALLOC_DEBUG
typedef struct {
    void*           ptrs[QUARANTINE_SIZE];
    int             head;
    int             count;
    pthread_mutex_t lock;
} Quarantine;

static Quarantine gQuarantine = { .lock = PTHREAD_MUTEX_INITIALIZER };
#endif


static inline int getSizeClassIndex(size_t size) {
    if (likely(size <=   16)) return 0;
    if (likely(size <=   32)) return 1;
    if (likely(size <=   64)) return 2;
    if (likely(size <=  128)) return 3;
    if (likely(size <=  256)) return 4;
    if (likely(size <=  512)) return 5;
    if (likely(size <= 1024)) return 6;
    return -1;
}

static inline size_t getClassSize(int index) {
    return (size_t)16 << index;
}

static inline size_t pageAlign(size_t n) {
    return (n + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

static inline size_t hugePageAlign(size_t n) {
    return (n + HUGE_PAGE_SIZE - 1) & ~(HUGE_PAGE_SIZE - 1);
}


static int getCurrentNumaNode(void) {
#if defined(__linux__) && defined(XALLOC_NUMA)
    if (likely(numa_available() >= 0)) {
        int cpu = sched_getcpu();
        if (likely(cpu >= 0))
            return numa_node_of_cpu(cpu);
    }
#endif
    return 0;
}


static void* sysAlloc(size_t size) {
    void* p = mmap(NULL, size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (unlikely(p == MAP_FAILED)) return NULL;
    atomic_fetch_add_explicit(&gStats.mmapCalls, 1, memory_order_relaxed);
    return p;
}

static void* hugePageAlloc(size_t size) {
    size_t aligned = hugePageAlign(size);
    void* p = mmap(NULL, aligned, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    if (unlikely(p == MAP_FAILED)) {
        p = sysAlloc(size);
        return p;
    }
    atomic_fetch_add_explicit(&gStats.mmapCalls,     1, memory_order_relaxed);
    atomic_fetch_add_explicit(&gStats.hugePagesUsed, 1, memory_order_relaxed);
    return p;
}

static void* numaAlloc(size_t size, int numaNode) {
#if defined(__linux__) && defined(XALLOC_NUMA)
    if (likely(numa_available() >= 0 && numaNode >= 0)) {
        void* p = numa_alloc_onnode(size, numaNode);
        if (likely(p != NULL)) {
            atomic_fetch_add_explicit(&gStats.mmapCalls, 1, memory_order_relaxed);
            return p;
        }
    }
#endif
    (void)numaNode;
    return sysAlloc(size);
}


static Arena* acquireArena(void) {
    int id = atomic_fetch_add(&gArenaCount, 1);
    if (unlikely(id >= MAX_ARENAS)) {
        atomic_fetch_sub(&gArenaCount, 1);
        id = 0;
    }
    Arena* a = &gArenas[id];
    if (!a->initialized) {
        pthread_mutex_init(&a->lock, NULL);
        a->numaNode   = getCurrentNumaNode();
        a->arenaId    = id;
        a->initialized = 1;
        for (int i = 0; i < NUM_SIZE_CLASSES; i++) {
            atomic_store(&a->tc[i].head, 0);
            atomic_store(&a->tc[i].tail, 0);
        }
    }
    return a;
}

static inline Arena* getArena(void) {
    if (unlikely(localArena == NULL))
        localArena = acquireArena();
    return localArena;
}


static void tryReclaimSlabs(Arena* arena, int classIdx) {
    Slab** pp   = &arena->slabs[classIdx];
    Slab*  cur  = *pp;
    while (cur) {
        Slab* next = cur->next;
        if (cur->freeCount == cur->totalCount) {
            *pp = next;
            size_t objSize  = getClassSize(classIdx) + sizeof(BlockHeader);
            size_t slabSize = pageAlign(sizeof(Slab) + objSize * BLOCK_REFILL_COUNT);
            munmap(cur, slabSize);
            atomic_fetch_add_explicit(&gStats.munmapCalls,          1, memory_order_relaxed);
            atomic_fetch_add_explicit(&gStats.perClass[classIdx].slabsReclaimed,
                                      1, memory_order_relaxed);
        } else {
            pp = &cur->next;
        }
        cur = next;
    }
}


static void newSlab(Arena* arena, int classIdx) {
    size_t objSize  = getClassSize(classIdx) + sizeof(BlockHeader);
    size_t slabSize = pageAlign(sizeof(Slab) + objSize * BLOCK_REFILL_COUNT);

    uint8_t* raw = (uint8_t*)numaAlloc(slabSize, arena->numaNode);
    if (unlikely(!raw)) return;

    Slab* slab       = (Slab*)raw;
    slab->next       = arena->slabs[classIdx];
    slab->freeCount  = 0;
    slab->totalCount = 0;
    slab->classIdx   = classIdx;
    slab->numaNode   = arena->numaNode;
    slab->freeList   = NULL;
    arena->slabs[classIdx] = slab;

    uint8_t* cursor = raw + sizeof(Slab);
    uint8_t* end    = raw + slabSize;

    while (cursor + objSize <= end) {
        BlockHeader* hdr = (BlockHeader*)cursor;
        hdr->totalSize   = (size_t)classIdx;
        hdr->isLarge     = 0;
        hdr->magic       = 0xDEADBEEF;

        FreeNode* node = (FreeNode*)(hdr + 1);
        node->next     = arena->freeLists[classIdx];
        arena->freeLists[classIdx] = node;
        arena->listCounts[classIdx]++;
        slab->freeCount++;
        slab->totalCount++;
        cursor += objSize;
    }

    atomic_fetch_add_explicit(&gStats.perClass[classIdx].slabsCreated,
                              1, memory_order_relaxed);
}


static void refillArena(Arena* arena, int classIdx) {
    int pulled = 0;
    FreeNode* node;
    while (pulled < BLOCK_REFILL_COUNT &&
           (node = tc_pop(&arena->tc[classIdx])) != NULL) {
        node->next = arena->freeLists[classIdx];
        arena->freeLists[classIdx] = node;
        arena->listCounts[classIdx]++;
        pulled++;
    }
    if (pulled == 0)
        newSlab(arena, classIdx);
}


static void* largeAlloc(size_t userSize) {
    bool useHuge = userSize >= HUGE_PAGE_THRESHOLD;
    size_t needed = useHuge
        ? hugePageAlign(userSize + sizeof(BlockHeader) + sizeof(LargeBlock))
        : pageAlign    (userSize + sizeof(BlockHeader) + sizeof(LargeBlock));

    pthread_mutex_lock(&largeLock);
    LargeBlock* prev = NULL;
    LargeBlock* cur  = largeFreeList;
    while (cur) {
        if (cur->size >= needed) {
            if (prev) prev->next = cur->next;
            else      largeFreeList = cur->next;
            pthread_mutex_unlock(&largeLock);

            atomic_fetch_add_explicit(&gStats.largeReuses, 1, memory_order_relaxed);
            madvise(cur, cur->size, MADV_WILLNEED);

            BlockHeader* hdr = (BlockHeader*)(cur + 1);
            hdr->totalSize   = cur->size;
            hdr->isLarge     = 1;
            hdr->magic       = 0xDEADBEEF;
            return (void*)(hdr + 1);
        }
        prev = cur;
        cur  = cur->next;
    }
    pthread_mutex_unlock(&largeLock);

    int numaNode = getCurrentNumaNode();
    void* raw = useHuge ? hugePageAlloc(needed) : numaAlloc(needed, numaNode);
    if (unlikely(!raw)) return NULL;

    LargeBlock*  lb  = (LargeBlock*)raw;
    lb->size         = needed;
    lb->numaNode     = numaNode;
    BlockHeader* hdr = (BlockHeader*)(lb + 1);
    hdr->totalSize   = needed;
    hdr->isLarge     = 1;
    hdr->magic       = 0xDEADBEEF;
    return (void*)(hdr + 1);
}

static void largeFree(BlockHeader* hdr) {
    LargeBlock* lb = (LargeBlock*)((uint8_t*)hdr - sizeof(LargeBlock));
    madvise(lb, lb->size, MADV_DONTNEED);

    pthread_mutex_lock(&largeLock);
    lb->next      = largeFreeList;
    largeFreeList = lb;
    pthread_mutex_unlock(&largeLock);
}


static void _xFreeImmediate(void* ptr) {
    if (unlikely(!ptr)) return;

    BlockHeader* hdr = ((BlockHeader*)ptr) - 1;
    if (unlikely(hdr->magic != 0xDEADBEEF)) abort();

    if (likely(hdr->isLarge == 0)) {
        int    classIdx = (int)hdr->totalSize;
        size_t sz       = getClassSize(classIdx);

        atomic_fetch_add_explicit(&gStats.totalFreed,   sz, memory_order_relaxed);
        atomic_fetch_sub_explicit(&gStats.currentUsage, sz, memory_order_relaxed);
        atomic_fetch_add_explicit(&gStats.perClass[classIdx].frees,
                                  sz, memory_order_relaxed);
        atomic_fetch_sub_explicit(&gStats.perClass[classIdx].currentUsage,
                                  sz, memory_order_relaxed);

        Arena* arena = getArena();
        FreeNode* node = (FreeNode*)ptr;

        if (!tc_push(&arena->tc[classIdx], node)) {
            node->next = arena->freeLists[classIdx];
            arena->freeLists[classIdx] = node;
            arena->listCounts[classIdx]++;

            if (unlikely(arena->listCounts[classIdx] > MAX_LOCAL_CACHE_SIZE)) {
                pthread_mutex_lock(&arena->lock);
                int moved = 0;
                while (moved < BLOCK_REFILL_COUNT) {
                    FreeNode* toMove = arena->freeLists[classIdx];
                    if (!toMove) break;
                    arena->freeLists[classIdx] = toMove->next;
                    arena->listCounts[classIdx]--;
                    if (!tc_push(&arena->tc[classIdx], toMove)) {
                        toMove->next = arena->freeLists[classIdx];
                        arena->freeLists[classIdx] = toMove;
                        arena->listCounts[classIdx]++;
                        break;
                    }
                    moved++;
                }
                tryReclaimSlabs(arena, classIdx);
                pthread_mutex_unlock(&arena->lock);
            }
        }
        return;
    }

    size_t sz = hdr->totalSize - sizeof(BlockHeader) - sizeof(LargeBlock);
    atomic_fetch_add_explicit(&gStats.totalFreed,   sz, memory_order_relaxed);
    atomic_fetch_sub_explicit(&gStats.currentUsage, sz, memory_order_relaxed);
    largeFree(hdr);
}


#ifdef XALLOC_DEBUG
static void* quarantine_add(void* ptr) {
    pthread_mutex_lock(&gQuarantine.lock);
    void* evicted = NULL;
    if (gQuarantine.count == QUARANTINE_SIZE) {
        evicted = gQuarantine.ptrs[gQuarantine.head];
        gQuarantine.ptrs[gQuarantine.head] = ptr;
        gQuarantine.head = (gQuarantine.head + 1) % QUARANTINE_SIZE;
    } else {
        int slot = (gQuarantine.head + gQuarantine.count) % QUARANTINE_SIZE;
        gQuarantine.ptrs[slot] = ptr;
        gQuarantine.count++;
    }
    pthread_mutex_unlock(&gQuarantine.lock);
    return evicted;
}
#endif


bool null(void* ptr) {
    return (ptr == NULL);
}

void xAllocatorStats(void) {
    fprintf(stderr,
        "=== xAllocator Stats ===\n"
        "  Allocated   : %zu bytes\n"
        "  Freed       : %zu bytes\n"
        "  In use      : %zu bytes\n"
        "  mmap calls  : %zu\n"
        "  munmap calls: %zu\n"
        "  Large reuses: %zu\n"
        "  Huge pages  : %zu\n",
        atomic_load(&gStats.totalAllocated),
        atomic_load(&gStats.totalFreed),
        atomic_load(&gStats.currentUsage),
        atomic_load(&gStats.mmapCalls),
        atomic_load(&gStats.munmapCalls),
        atomic_load(&gStats.largeReuses),
        atomic_load(&gStats.hugePagesUsed));

    for (int i = 0; i < NUM_SIZE_CLASSES; i++) {
        fprintf(stderr,
            "  [class %d  %4zu B] allocs=%zu  frees=%zu  inUse=%zu"
            "  slabsCreated=%zu  slabsReclaimed=%zu\n",
            i, getClassSize(i),
            atomic_load(&gStats.perClass[i].allocs),
            atomic_load(&gStats.perClass[i].frees),
            atomic_load(&gStats.perClass[i].currentUsage),
            atomic_load(&gStats.perClass[i].slabsCreated),
            atomic_load(&gStats.perClass[i].slabsReclaimed));
    }
}

void xFree(void* ptr) {
    if (unlikely(!ptr)) return;
#ifdef XALLOC_DEBUG
    void* evicted = quarantine_add(ptr);
    if (evicted) _xFreeImmediate(evicted);
#else
    _xFreeImmediate(ptr);
#endif
}

void* xMalloc(size_t size) {
    if (unlikely(size == 0)) return NULL;

    int classIdx = getSizeClassIndex(size);

    if (likely(classIdx >= 0)) {
        Arena* arena = getArena();

        if (unlikely(!arena->freeLists[classIdx]))
            refillArena(arena, classIdx);
        if (unlikely(!arena->freeLists[classIdx])) return NULL;

        FreeNode* node = arena->freeLists[classIdx];
        arena->freeLists[classIdx] = node->next;
        arena->listCounts[classIdx]--;

        size_t actualSize = getClassSize(classIdx);
        atomic_fetch_add_explicit(&gStats.totalAllocated, actualSize, memory_order_relaxed);
        atomic_fetch_add_explicit(&gStats.currentUsage,   actualSize, memory_order_relaxed);
        atomic_fetch_add_explicit(&gStats.perClass[classIdx].allocs,
                                  actualSize, memory_order_relaxed);
        atomic_fetch_add_explicit(&gStats.perClass[classIdx].currentUsage,
                                  actualSize, memory_order_relaxed);
        return (void*)node;
    }

    void* ptr = largeAlloc(size);
    if (likely(ptr != NULL)) {
        atomic_fetch_add_explicit(&gStats.totalAllocated, size, memory_order_relaxed);
        atomic_fetch_add_explicit(&gStats.currentUsage,   size, memory_order_relaxed);
    }
    return ptr;
}

void* xCalloc(size_t nmemb, size_t size) {
    if (unlikely(nmemb == 0 || size == 0)) return NULL;
    size_t total = nmemb * size;
    if (unlikely(size != 0 && total / size != nmemb)) return NULL;
    void* ptr = xMalloc(total);
    if (likely(ptr != NULL)) memset(ptr, 0, total);
    return ptr;
}

void* xRealloc(void* ptr, size_t size) {
    if (unlikely(ptr == NULL)) return xMalloc(size);
    if (unlikely(size == 0))   { xFree(ptr); return NULL; }

    BlockHeader* hdr = ((BlockHeader*)ptr) - 1;
    if (unlikely(hdr->magic != 0xDEADBEEF)) abort();

    size_t currentSize = hdr->isLarge
        ? hdr->totalSize - sizeof(BlockHeader) - sizeof(LargeBlock)
        : getClassSize((int)hdr->totalSize);

    if (likely(size <= currentSize)) return ptr;

    void* newPtr = xMalloc(size);
    if (likely(newPtr != NULL)) {
        memcpy(newPtr, ptr, currentSize);
        xFree(ptr);
    }
    return newPtr;
}

void* xShrinkRealloc(void* ptr, size_t size) {
    if (unlikely(ptr == NULL)) return xMalloc(size);
    if (unlikely(size == 0))   { xFree(ptr); return NULL; }

    BlockHeader* hdr = ((BlockHeader*)ptr) - 1;
    if (unlikely(hdr->magic != 0xDEADBEEF)) abort();

    size_t currentSize = hdr->isLarge
        ? hdr->totalSize - sizeof(BlockHeader) - sizeof(LargeBlock)
        : getClassSize((int)hdr->totalSize);

    if (size <= currentSize && size > currentSize / 2) return ptr;

    void* newPtr = xMalloc(size);
    if (likely(newPtr != NULL)) {
        size_t copySize = currentSize < size ? currentSize : size;
        memcpy(newPtr, ptr, copySize);
        xFree(ptr);
    }
    return newPtr;
}