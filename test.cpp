#include "arena.h"
#include "base.h"
#define STB_SPRINTF_IMPLEMENTATION
#include "third_party/stb_sprintf.h"

int main(void) {
    Arena *arena = arena_create_scratch_default();
    
    int *arr = (int *)arena_push(arena, sizeof(int) * 10, 8, 0);
    
    for (int i = 0; i < 10; i++) {
        arr[i] = i * i;
    }
    
    char *str = arena_strdup(arena, "test string");
    
    char *formatted = arena_sprintf(arena, "Number: %d", 42);
    
    printf("%s\n%s\n", formatted, str);
    
    arena_release(arena);
    
    return 0;
}
