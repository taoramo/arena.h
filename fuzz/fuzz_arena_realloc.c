#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define STB_SPRINTF_IMPLEMENTATION
#define ARENA_IMPLEMENTATION
#include "../arena.h"

typedef struct {
    uint64_t original_size;
    uint64_t new_size;
    uint8_t alignment;
    uint8_t operation;
} realloc_input_t;

static Arena g_arena;

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

    arena_release(&g_arena);
    g_arena = *arena_create_scratch_default();

    void *ptr = arena_push(&g_arena, input.original_size, input.alignment, 0);
    if (ptr && input.original_size > 0) {
        memset(ptr, 0xAA, input.original_size);
    }

    switch (input.operation % 8) {
    case 0:
        ptr = arena_realloc(&g_arena, ptr, input.original_size, input.new_size);
        break;
    case 1:
        ptr = arena_realloc(&g_arena, NULL, 0, input.new_size);
        break;
    case 2:
        ptr = arena_realloc(&g_arena, ptr, input.original_size, input.original_size);
        break;
    case 3: {
        void *ptr2 = arena_push(&g_arena, 16, 8, 0);
        if (ptr2) {
            ptr = arena_realloc(&g_arena, ptr, input.original_size, input.new_size);
        }
        break;
    }
    case 4:
        if (input.new_size > input.original_size) {
            ptr = arena_realloc(&g_arena, ptr, input.original_size, input.new_size);
        }
        break;
    case 5:
        if (input.new_size < input.original_size) {
            ptr = arena_realloc(&g_arena, ptr, input.original_size, input.new_size);
        }
        break;
    case 6:
        ptr = arena_realloc(&g_arena, ptr, input.original_size, 0);
        break;
    case 7:
        ptr = arena_realloc(&g_arena, ptr, input.original_size, input.original_size * 2);
        break;
    }

    if (ptr && input.new_size > 0 && input.new_size < (1ULL << 20)) {
        memset(ptr, 0x55, input.new_size);
    }

    return 0;
}
