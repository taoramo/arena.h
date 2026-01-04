#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define STB_SPRINTF_IMPLEMENTATION
#define ARENA_IMPLEMENTATION
#define ARENA_ABORT_ON_OOM 0
#include "../arena.h"

typedef struct {
    uint64_t original_size;
    uint64_t new_size;
    uint8_t alignment;
    uint8_t operation;
} realloc_input_t;

static Arena *g_arena = NULL;

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

#ifndef LLVMFuzzerTestOneInput
int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        FILE *f = fopen(argv[i], "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            fseek(f, 0, SEEK_SET);
            uint8_t *data = malloc(size);
            fread(data, 1, size, f);
            fclose(f);
            LLVMFuzzerTestOneInput(data, size);
            free(data);
        }
    }
    return 0;
}
#endif

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size < sizeof(realloc_input_t)) return 0;

    realloc_input_t input;
    memcpy(&input, data, sizeof(realloc_input_t));

    if (input.original_size > (1ULL << 30)) input.original_size = 0;
    if (input.new_size > (1ULL << 30)) input.new_size = 0;
    if (input.alignment > 128 || input.alignment == 0) input.alignment = 16;
    if (input.operation > 7) input.operation = 0;

    if (g_arena != NULL) {
        arena_release(g_arena);
    }
    g_arena = arena_create_scratch_default();

    if (g_arena == NULL) return 0;

    void *ptr = NULL;
    if (input.original_size > 0 && input.original_size < (1ULL << 28)) {
        ptr = arena_push(g_arena, input.original_size, input.alignment, 0);
        if (ptr && input.original_size > 0 && input.original_size < 1024) {
            memset(ptr, 0xAA, input.original_size);
        }
    }

    void *newptr = ptr;
    switch (input.operation % 8) {
    case 0:
        if (input.new_size > 0 && input.new_size < (1ULL << 28) && ptr != NULL) {
            newptr = arena_realloc(g_arena, ptr, input.original_size, input.new_size);
        }
        break;
    case 1:
        if (input.new_size > 0 && input.new_size < (1ULL << 28)) {
            newptr = arena_realloc(g_arena, NULL, 0, input.new_size);
        }
        break;
    case 2:
        if (ptr != NULL && input.original_size < (1ULL << 28)) {
            newptr = arena_realloc(g_arena, ptr, input.original_size, input.original_size);
        }
        break;
    case 3: {
        void *ptr2 = arena_push(g_arena, 16, 8, 0);
        if (ptr2 && input.new_size > 0 && input.new_size < (1ULL << 28)) {
            newptr = arena_realloc(g_arena, ptr, input.original_size, input.new_size);
        }
        break;
    }
    case 4:
        if (input.new_size > input.original_size && input.new_size < (1ULL << 28) && ptr != NULL) {
            newptr = arena_realloc(g_arena, ptr, input.original_size, input.new_size);
        }
        break;
    case 5:
        if (input.new_size < input.original_size && input.new_size > 0 && input.new_size < (1ULL << 28) && ptr != NULL) {
            newptr = arena_realloc(g_arena, ptr, input.original_size, input.new_size);
        }
        break;
    case 6:
        if (input.new_size == 0 && ptr != NULL && input.original_size < (1ULL << 28)) {
            newptr = arena_realloc(g_arena, ptr, input.original_size, 0);
        }
        break;
    case 7: {
        uint64_t doubled = input.original_size * 2;
        if (input.new_size > 0 && input.new_size < (1ULL << 28) && ptr != NULL && doubled < (1ULL << 28)) {
            newptr = arena_realloc(g_arena, ptr, input.original_size, doubled);
        }
        break;
    }
    }

    size_t memset_size = (input.new_size < (1ULL << 20)) ? input.new_size : (1ULL << 20);
    if (newptr != NULL && memset_size > 0) {
        memset(newptr, 0x55, memset_size);
    }

    return 0;
}
