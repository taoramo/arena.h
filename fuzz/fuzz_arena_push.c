#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define STB_SPRINTF_IMPLEMENTATION
#define ARENA_IMPLEMENTATION
#include "../arena.h"

typedef struct {
    uint64_t size;
    uint8_t alignment;
    uint8_t zero_fill;
} push_input_t;

static Arena *g_arena;

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
    if (size < sizeof(push_input_t)) return 0;

    push_input_t input;
    memcpy(&input, data, sizeof(push_input_t));

    if (input.size > (1ULL << 30)) return 0;
    if (input.alignment > 128 || input.alignment == 0) input.alignment = 16;
    if (input.zero_fill > 1) input.zero_fill = 0;

    arena_release(g_arena);
    g_arena = arena_create_scratch_default();

    void *ptr = arena_push(g_arena, input.size, input.alignment, input.zero_fill);
    if (ptr && input.size > 0) {
        memset(ptr, 0xAA, input.size);
    }

    void *ptr2 = arena_push(g_arena, input.size, input.alignment ^ 0xFF, input.zero_fill ^ 0x01);
    if (ptr2 && input.size > 0) {
        memset(ptr2, 0x55, input.size);
    }

    void *ptr3 = arena_push(g_arena, input.size, input.alignment & 0x0F, 1);
    (void)ptr3;

    if (input.alignment == 0) {
        ptr = arena_push(g_arena, input.size, 1, input.zero_fill);
    }

    return 0;
}
