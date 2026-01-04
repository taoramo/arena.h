#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define STB_SPRINTF_IMPLEMENTATION
#define ARENA_IMPLEMENTATION
#define ARENA_ABORT_ON_OOM 0
#include "../arena.h"

typedef struct {
    uint64_t alloc_size;
    uint8_t nest_depth;
    uint8_t alignment;
    uint8_t pattern;
} temp_input_t;

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
    if (size < sizeof(temp_input_t)) return 0;

    temp_input_t input;
    memcpy(&input, data, sizeof(temp_input_t));

    if (input.alloc_size > (1ULL << 30)) input.alloc_size = 0;
    // Validate alignment is a power of 2
    if (input.alignment == 0 || (input.alignment & (input.alignment - 1)) != 0) {
      input.alignment = 16;  // Default to 16-byte alignment
    }
    if (input.nest_depth > 8) input.nest_depth = 4;

    if (g_arena != NULL) {
        arena_release(g_arena);
    }
    g_arena = arena_create_scratch_default();

    if (g_arena == NULL || g_arena->current == NULL) return 0;

    TempArena temps[8];
    int depth = 0;

    void *base_ptr = arena_push(g_arena, input.alloc_size, input.alignment, 0);
    if (!base_ptr) {
        return 0;
    }

    for (int i = 0; i < (int)(input.nest_depth % 8) + 1; i++) {
        temps[depth] = arena_temp_begin(g_arena);
        depth++;

        void *ptr = arena_push(g_arena, input.alloc_size, input.alignment, 0);
        if (ptr && input.alloc_size > 0) {
            memset(ptr, input.pattern + i, input.alloc_size);
        }

        if (i % 3 == 0 && depth > 0) {
            depth--;
            arena_temp_end(temps[depth]);
        }
    }

    while (depth > 0) {
        depth--;
        arena_temp_end(temps[depth]);
    }

    (void)base_ptr;

    TempArena temp = arena_temp_begin(g_arena);
    void *ptr = arena_push(g_arena, input.alloc_size, input.alignment, 0);
    if (ptr) {
        arena_clear(g_arena);
        arena_temp_end(temp);
    }

    return 0;
}
