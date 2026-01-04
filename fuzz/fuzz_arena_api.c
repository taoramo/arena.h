#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_SPRINTF_IMPLEMENTATION
#define ARENA_IMPLEMENTATION
#include "../arena.h"

typedef struct {
  uint8_t operation;
  uint64_t size;
  uint8_t alignment;
  uint8_t zero_fill;
  uint64_t pos;
} fuzz_input_t;

static Arena *g_arena;

void reset_arena(void);
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

void reset_arena(void) {
  arena_release(g_arena);
  g_arena = arena_create_scratch_default();
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  if (size < sizeof(fuzz_input_t))
    return 0;

  fuzz_input_t input;
  memcpy(&input, data, sizeof(fuzz_input_t));

  if (input.size > (1ULL << 30)) return 0;
  if (input.alignment > 128 || input.alignment == 0)
    input.alignment = 16;
  if (input.pos > (1ULL << 30))
    input.pos = 0;
  if (input.zero_fill > 1)
    input.zero_fill = 0;

  const uint8_t *payload = data + sizeof(fuzz_input_t);
  size_t payload_size = size - sizeof(fuzz_input_t);

  reset_arena();

  switch (input.operation % 12) {
  case 0: {
    void *ptr =
        arena_push(g_arena, input.size, input.alignment, input.zero_fill);
    if (ptr && payload_size >= input.size) {
      memcpy(ptr, payload,
             payload_size < input.size ? payload_size : input.size);
    }
    break;
  }
  case 1: {
    void *ptr =
        arena_push(g_arena, input.size, input.alignment, input.zero_fill);
    if (ptr) {
      void *newptr = arena_realloc(g_arena, ptr, input.size, input.size + 1);
      (void)newptr;
    }
    break;
  }
  case 2: {
    U64 pos = arena_pos(g_arena);
    if (arena_push(g_arena, input.size, input.alignment, input.zero_fill) !=
        NULL) {
      arena_pop_to(g_arena, pos);
    }
    break;
  }
  case 3: {
    arena_clear(g_arena);
    break;
  }
  case 4: {
    TempArena temp = arena_temp_begin(g_arena);
    if (arena_push(g_arena, input.size, input.alignment, input.zero_fill) !=
        NULL) {
      arena_temp_end(temp);
    }
    break;
  }
  case 5: {
    if (payload_size > 0) {
      char temp_str[257] = {0};
      memcpy(temp_str, payload, payload_size < 256 ? payload_size : 256);
      char *str = arena_strdup(g_arena, temp_str);
      (void)str;
    }
    break;
  }
  case 6: {
    char *str =
        arena_sprintf(g_arena, "test %llu %u", input.size, input.alignment);
    (void)str;
    break;
  }
  case 7: {
    void *ptr = arena_push(g_arena, input.size, input.alignment, 1);
    (void)ptr;
    break;
  }
  case 8: {
    U64 pos1 = arena_pos(g_arena);
    if (arena_push(g_arena, input.size, input.alignment, input.zero_fill) !=
        NULL) {
      U64 pos2 = arena_pos(g_arena);
      arena_pop_to(g_arena, pos1);
      if (arena_push(g_arena, input.size, input.alignment, input.zero_fill) !=
          NULL) {
        arena_pop_to(g_arena, pos2);
      }
    }
    break;
  }
  case 9: {
    void *ptr =
        arena_push(g_arena, input.size, input.alignment, input.zero_fill);
    if (ptr && input.size > 0) {
      arena_memdup(g_arena, ptr, input.size);
    }
    break;
  }
  case 10: {
    U64 pos = arena_pos(g_arena);
    if (arena_push(g_arena, input.size, input.alignment, input.zero_fill) !=
        NULL) {
      arena_clear(g_arena);
    }
    if (arena_push(g_arena, input.size, input.alignment, input.zero_fill) !=
        NULL) {
      (void)pos;
    }
    break;
  }
  case 11: {
    TempArena temp = arena_temp_begin(g_arena);
    if (arena_push(g_arena, input.size, input.alignment, input.zero_fill) !=
        NULL) {
      TempArena temp2 = arena_temp_begin(g_arena);
      if (arena_push(g_arena, input.size, input.alignment, input.zero_fill) !=
          NULL) {
        arena_temp_end(temp2);
      }
      arena_temp_end(temp);
    }
    break;
  }
  }
  return 0;
}
