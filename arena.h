//======================================================================
// Linux Arena Allocator - feature-complete version
//======================================================================
// Features:
//  - reserve/commit allocator using mmap/mprotect
//  - ASan integration
//  - optional debug assertions & logging
//  - arena_push/clear/pop/temp API
//  - high-level helpers: strdup, memdup, sprintf
//  - Tsoding-style dynamic arrays with arena-based growth
//
// Compile with -DARENA_ENABLE_DEBUG=1 to enable logs + assertions
//======================================================================

#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

//======================================================================
// Configuration Flags
//======================================================================
#ifndef ARENA_ENABLE_DEBUG
#define ARENA_ENABLE_DEBUG 0
#endif

//======================================================================
// Basic types and helpers
//======================================================================
typedef uint64_t U64;
typedef uint8_t  U8;
typedef bool     B32;

#define ARENA_HEADER_SIZE (sizeof(struct Arena))
#define AlignUpPow2(x,a) (((x)+((a)-1))&~((a)-1))
#define ClampTop(x,t)    ((x)>(t)?(t):(x))
#define Min(a,b)         ((a)<(b)?(a):(b))

//======================================================================
// Debug System
//======================================================================
#if ARENA_ENABLE_DEBUG
#  include <assert.h>
#  define ARENA_ASSERT(expr) \
    do { \
        if(!(expr)) { \
            fprintf(stderr, "[ARENA ASSERT FAILED] %s:%d: %s\n", \
                    __FILE__, __LINE__, #expr); \
            assert(expr); \
        } \
    } while(0)
#  define ARENA_DEBUG_LOG(fmt, ...) \
    fprintf(stderr, "[arena dbg] " fmt "\n", ##__VA_ARGS__)
#else
#  define ARENA_ASSERT(expr) ((void)0)
#  define ARENA_DEBUG_LOG(fmt, ...) ((void)0)
#endif

//======================================================================
// ASan Integration
//======================================================================
#if defined(__has_feature)
#  if __has_feature(address_sanitizer)
#    define ARENA_USE_ASAN 1
#  else
#    define ARENA_USE_ASAN 0
#  endif
#elif defined(__SANITIZE_ADDRESS__)
#  define ARENA_USE_ASAN 1
#else
#  define ARENA_USE_ASAN 0
#endif

#if ARENA_USE_ASAN
extern void __asan_poison_memory_region(void const volatile *addr, size_t size);
extern void __asan_unpoison_memory_region(void const volatile *addr, size_t size);
#  define AsanPoisonMemoryRegion(addr, size)   __asan_poison_memory_region((addr), (size))
#  define AsanUnpoisonMemoryRegion(addr, size) __asan_unpoison_memory_region((addr), (size))
#else
#  define AsanPoisonMemoryRegion(addr, size)   (void)0
#  define AsanUnpoisonMemoryRegion(addr, size) (void)0
#endif

//======================================================================
// Arena Structure
//======================================================================
enum {
    ARENA_FLAG_NONE     = 0,
    ARENA_FLAG_NO_CHAIN = 1 << 0,
};

typedef struct Arena {
    struct Arena *prev;
    struct Arena *current;
    U64 flags;
    U64 commit_granularity;
    U64 reserved_size;
    U64 committed;
    U64 pos;
} Arena;

//======================================================================
// OS Memory Interface (Linux)
//======================================================================
static inline void* os_reserve(U64 size) {
    void* ptr = mmap(NULL, size, PROT_NONE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return (ptr == MAP_FAILED) ? NULL : ptr;
}

static inline void os_commit(void* ptr, U64 size) {
    size = AlignUpPow2(size, (U64)getpagesize());
    if (mprotect(ptr, size, PROT_READ | PROT_WRITE) != 0) {
        fprintf(stderr, "os_commit failed: %s\n", strerror(errno));
        abort();
    }
}

static inline void os_release(void* ptr, U64 size) {
    munmap(ptr, size);
}

//======================================================================
// Arena Lifecycle
//======================================================================
static inline Arena* arena_create(U64 reserve_size, U64 commit_granularity,
                                  U64 initial_commit, U64 flags) {
    reserve_size       = AlignUpPow2(reserve_size, (U64)getpagesize());
    commit_granularity = AlignUpPow2(commit_granularity, (U64)getpagesize());
    initial_commit     = AlignUpPow2(initial_commit, (U64)getpagesize());

    void* base = os_reserve(reserve_size);
    if (!base) {
        fprintf(stderr, "arena_create: reserve failed\n");
        return NULL;
    }

    os_commit(base, initial_commit);

    Arena* a = (Arena*)base;
    a->prev = NULL;
    a->current = a;
    a->flags = flags;
    a->commit_granularity = commit_granularity;
    a->reserved_size = reserve_size;
    a->committed = initial_commit;
    a->pos = ARENA_HEADER_SIZE;

    AsanPoisonMemoryRegion(base, reserve_size);
    AsanUnpoisonMemoryRegion(base, ARENA_HEADER_SIZE);
    AsanUnpoisonMemoryRegion((U8*)base, initial_commit);

    ARENA_DEBUG_LOG("arena_create: %p reserve=%lluMB commit=%lluKB",
        (void*)a, (unsigned long long)(reserve_size>>20), (unsigned long long)(initial_commit>>10));

    return a;
}

static inline void arena_release(Arena* arena) {
    for (Arena* a = arena->current; a; ) {
        Arena* prev = a->prev;
        ARENA_DEBUG_LOG("arena_release: %p (%.2f MB)", (void*)a, a->reserved_size / (1024.0*1024.0));
        os_release(a, a->reserved_size);
        a = prev;
    }
}

//======================================================================
// Core Allocation
//======================================================================
static inline void* arena_push(Arena* arena, U64 size, U64 align, B32 zero_fill) {
    ARENA_ASSERT(arena != NULL);
    Arena* current = arena->current;
    U64 pos_pre = AlignUpPow2(current->pos, align);
    U64 pos_pst = pos_pre + size;

    // chain new block if needed
    if (pos_pst > current->reserved_size && !(arena->flags & ARENA_FLAG_NO_CHAIN)) {
        U64 rs = current->reserved_size;
        U64 g  = current->commit_granularity;
        U64 ic = current->commit_granularity;
        Arena* nb = arena_create(rs, g, ic, current->flags);
        nb->prev = current;
        arena->current = nb;
        current = nb;
        pos_pre = AlignUpPow2(current->pos, align);
        pos_pst = pos_pre + size;
    }

    // commit new memory if needed
    if (pos_pst > current->committed) {
        U64 aligned = AlignUpPow2(pos_pst, current->commit_granularity);
        U64 clamped = ClampTop(aligned, current->reserved_size);
        U64 csize = clamped - current->committed;
        if (csize > 0) {
            void* ptr = (U8*)current + current->committed;
            ARENA_DEBUG_LOG("arena_commit: %p +%lluKB", ptr, (unsigned long long)(csize >> 10));
            os_commit(ptr, csize);
            current->committed = clamped;
            AsanUnpoisonMemoryRegion(ptr, csize);
        }
    }

    void* result = NULL;
    if (pos_pst <= current->committed) {
        result = (U8*)current + pos_pre;
        current->pos = pos_pst;
        AsanUnpoisonMemoryRegion(result, size);
        if (zero_fill) memset(result, 0, size);
    } else {
        fprintf(stderr, "Arena push failed: insufficient commit.\n");
        abort();
    }
    return result;
}

//======================================================================
// Pop / Clear
//======================================================================
static inline void arena_pop_to(Arena* arena, U64 pos) {
    ARENA_ASSERT(arena);
    Arena* current = arena->current;

    while (current && pos < ARENA_HEADER_SIZE) {
        Arena* prev = current->prev;
        os_release(current, current->reserved_size);
        current = prev;
    }
    if (current) {
        if (pos < ARENA_HEADER_SIZE)
            pos = ARENA_HEADER_SIZE;
        AsanPoisonMemoryRegion((U8*)current + pos, current->pos - pos);
        current->pos = pos;
        arena->current = current;
    }
}

static inline void arena_clear(Arena* arena) { arena_pop_to(arena, ARENA_HEADER_SIZE); }
static inline U64  arena_pos(Arena* arena)   { return arena->current->pos; }

//======================================================================
// Temporary Scopes
//======================================================================
typedef struct TempArena {
    Arena* arena;
    U64 pos;
} TempArena;

static inline TempArena temp_begin(Arena* a) {
    return (TempArena){a, arena_pos(a)};
}
static inline void temp_end(TempArena t) {
    arena_pop_to(t.arena, t.pos);
}

//======================================================================
// Helpers: memcpy
//======================================================================
static inline void *arena_memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = (unsigned char*)dst; const unsigned char *s = (const unsigned char*)src;
    while (n--) *d++ = *s++; return dst;
}

//======================================================================
// String helpers using arena_push()
//======================================================================
static inline char *arena_strdup(Arena *a, const char *cstr) {
    size_t n = strlen(cstr);
    char *dup = (char*)arena_push(a, (U64)(n + 1), 1, false);
    arena_memcpy(dup, cstr, n);
    dup[n] = '\0';
    return dup;
}

static inline void *arena_memdup(Arena *a, const void *data, size_t size) {
    void *copy = arena_push(a, (U64)size, 8, false);
    return arena_memcpy(copy, data, size);
}

static inline char *arena_vsprintf(Arena *a, const char *fmt, va_list args) {
    va_list copy;
    va_copy(copy, args);
    int n = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);

    if (n < 0) {
        fprintf(stderr, "arena_vsprintf: format error\n");
        return NULL;
    }
    char *buf = (char*)arena_push(a, (U64)(n + 1), 1, false);
    vsnprintf(buf, (size_t)(n + 1), fmt, args);
    return buf;
}

static inline char *arena_sprintf(Arena *a, const char *fmt, ...) {
    va_list args; va_start(args, fmt);
    char *res = arena_vsprintf(a, fmt, args);
    va_end(args);
    return res;
}

//======================================================================
// Arena-based Dynamic Arrays
//======================================================================
#ifndef ARENA_DA_INIT_CAP
#define ARENA_DA_INIT_CAP 8
#endif
#define cast_ptr(type) (type)(void*)

static inline void* arena_realloc(Arena* a, void* old_ptr,
                                  size_t old_size, size_t new_size) {
    ARENA_ASSERT(a != NULL);
    ARENA_ASSERT(new_size > 0);
    void* new_ptr = arena_push(a, (U64)new_size, 8, false);
    ARENA_ASSERT(new_ptr != NULL);
    if (old_ptr && old_size > 0)
        arena_memcpy(new_ptr, old_ptr, Min(old_size, new_size));
    ARENA_DEBUG_LOG("arena_realloc: old=%p new=%p old_size=%zu new_size=%zu",
                    old_ptr, new_ptr, old_size, new_size);
    return new_ptr;
}

#define arena_da_append(a, da, item)                                                          \
    do {                                                                                      \
        ARENA_ASSERT((da) != NULL);                                                           \
        if ((da)->count >= (da)->capacity) {                                                  \
            size_t new_cap = (da)->capacity == 0 ? ARENA_DA_INIT_CAP : (da)->capacity*2;     \
            ARENA_ASSERT(new_cap > (da)->capacity);                                           \
            (da)->items = cast_ptr((da)->items)arena_realloc(                                 \
                (a), (da)->items,                                                             \
                (da)->capacity*sizeof(*(da)->items),                                          \
                new_cap*sizeof(*(da)->items));                                                \
            ARENA_ASSERT((da)->items != NULL);                                                \
            (da)->capacity = new_cap;                                                         \
            ARENA_DEBUG_LOG("da_grow: %p new_cap=%zu", (void*)(da)->items, new_cap);          \
        }                                                                                     \
        (da)->items[(da)->count++] = (item);                                                  \
    } while (0)

#define arena_da_append_many(a, da, new_items, new_count)                                      \
    do {                                                                                       \
        ARENA_ASSERT((da) != NULL);                                                            \
        if ((da)->count + (new_count) > (da)->capacity) {                                      \
            size_t cap = (da)->capacity ? (da)->capacity : ARENA_DA_INIT_CAP;                 \
            while ((da)->count + (new_count) > cap) {                                          \
                ARENA_ASSERT(cap < (SIZE_MAX/2));                                              \
                cap *= 2;                                                                      \
            }                                                                                  \
            (da)->items = cast_ptr((da)->items)arena_realloc(                                  \
                (a), (da)->items,                                                              \
                (da)->capacity*sizeof(*(da)->items),                                           \
                cap*sizeof(*(da)->items));                                                     \
            ARENA_ASSERT((da)->items != NULL);                                                 \
            (da)->capacity = cap;                                                              \
            ARENA_DEBUG_LOG("da_grow_many: %p new_cap=%zu", (void*)(da)->items, cap);          \
        }                                                                                      \
        arena_memcpy((da)->items + (da)->count, (new_items), (new_count)*sizeof(*(da)->items));\
        (da)->count += (new_count);                                                            \
    } while (0)
