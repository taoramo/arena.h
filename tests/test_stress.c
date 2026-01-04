#define ARENA_IMPLEMENTATION
#include "arena.h"
#include "base.h"
#define STB_SPRINTF_IMPLEMENTATION
#include "third_party/stb_sprintf.h"
#include "acutest.h"
#include <string.h>

void test_stress_small_allocs(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        const int count = 100000;
        for (int i = 0; i < count; i++) {
            void* ptr = arena_push(arena, 1, 1, 0);
            TEST_CHECK(ptr != NULL);
        }
        arena_release(arena);
    }
}

void test_stress_large_allocs(void)
{
    Arena* arena = arena_create(10ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    TEST_CHECK(arena != NULL);
    if (arena) {
        void* ptr = arena_push(arena, 5ULL * 1024 * 1024, 8, 0);
        TEST_CHECK(ptr != NULL);

        U8* data = (U8*)ptr;
        for (U64 i = 0; i < 5ULL * 1024 * 1024; i++) {
            data[i] = (U8)(i % 256);
        }

        for (U64 i = 0; i < 5ULL * 1024 * 1024; i++) {
            TEST_CHECK(data[i] == (U8)(i % 256));
        }

        arena_release(arena);
    }
}

void test_stress_1gb_allocation(void)
{
    Arena* arena = arena_create(2ULL * 1024 * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    TEST_CHECK(arena != NULL);
    if (arena) {
        void* ptr = arena_push(arena, 1ULL * 1024 * 1024 * 1024, 8, 0);
        TEST_CHECK(ptr != NULL);
        
        U8* data = (U8*)ptr;
        
        data[0] = 0;
        data[1024*1024*1024/2] = 127;
        data[1024*1024*1024 - 1] = 255;
        
        TEST_CHECK(data[0] == 0);
        TEST_CHECK(data[1024*1024*1024/2] == 127);
        TEST_CHECK(data[1024*1024*1024 - 1] == 255);
        
        arena_release(arena);
    }
}

void test_stress_mixed(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        for (int i = 0; i < 10000; i++) {
            switch (i % 5) {
                case 0: arena_push(arena, 1, 1, 0); break;
                case 1: arena_push(arena, 8, 8, 0); break;
                case 2: arena_push(arena, 64, 16, 0); break;
                case 3: arena_push(arena, 512, 32, 0); break;
                case 4: arena_push(arena, 4096, 64, 0); break;
            }
        }
        arena_release(arena);
    }
}

void test_stress_temp_scopes(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        for (int i = 0; i < 1000; i++) {
            TempArena temp = arena_temp_begin(arena);
            for (int j = 0; j < 100; j++) {
                arena_push(arena, sizeof(int) * (j % 10 + 1), 8, 0);
            }
            arena_temp_end(temp);
        }

        TEST_CHECK(arena_pos(arena) == ARENA_HEADER_SIZE);
        arena_release(arena);
    }
}

void test_stress_realloc(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        void* ptr = arena_push(arena, 10, 8, 0);
        for (int i = 0; i < 1000; i++) {
            size_t old_size = 10 + i * 10;
            size_t new_size = 10 + (i + 1) * 10;
            ptr = arena_realloc(arena, ptr, old_size, new_size);
            TEST_CHECK(ptr != NULL);
        }
        arena_release(arena);
    }
}

void test_stress_da(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        typedef struct {
            int* items;
            size_t count;
            size_t capacity;
        } IntArray;

        IntArray array = {0};

        for (int i = 0; i < 100000; i++) {
            arena_da_append(arena, &array, i);
        }

        TEST_CHECK(array.count == 100000);

        for (int i = 0; i < 100000; i++) {
            TEST_CHECK(array.items[i] == i);
        }

        arena_release(arena);
    }
}

void test_push_pop_pattern(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        U64 positions[100];

        for (int i = 0; i < 100; i++) {
            positions[i] = arena_pos(arena);
            arena_push(arena, sizeof(int) * (i + 1), 8, 0);
        }

        for (int i = 99; i >= 0; i--) {
            arena_pop_to(arena, positions[i]);
            TEST_CHECK(arena_pos(arena) == positions[i]);
        }

        TEST_CHECK(arena_pos(arena) == ARENA_HEADER_SIZE);
        arena_release(arena);
    }
}

void test_many_temp_scopes_nested(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        const int depth = 10;
        TempArena temps[depth];

        for (int i = 0; i < depth; i++) {
            temps[i] = arena_temp_begin(arena);
            arena_push(arena, sizeof(int) * 10, 8, 0);
        }

        for (int i = depth - 1; i >= 0; i--) {
            arena_temp_end(temps[i]);
        }

        TEST_CHECK(arena_pos(arena) == ARENA_HEADER_SIZE);
        arena_release(arena);
    }
}

void test_alignment_stress(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        U64 alignments[] = {1, 2, 4, 8, 16, 32, 64, 128, 256};
        for (int i = 0; i < 1000; i++) {
            for (size_t j = 0; j < sizeof(alignments) / sizeof(alignments[0]); j++) {
                void* ptr = arena_push(arena, 1, alignments[j], 0);
                TEST_CHECK(ptr != NULL);
                TEST_CHECK((U64)ptr % alignments[j] == 0);
            }
        }
        arena_release(arena);
    }
}

void test_zero_fill_stress(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        for (size_t i = 0; i < 1000; i++) {
            size_t size = sizeof(int) * (i + 1);
            int* ptr = (int*)arena_push(arena, size, 8, 1);
            TEST_CHECK(ptr != NULL);
            for (size_t j = 0; j < i + 1; j++) {
                TEST_CHECK(ptr[j] == 0);
            }
        }
        arena_release(arena);
    }
}

void test_string_concatenation(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        char* result = arena_strdup(arena, "");
        TEST_CHECK(result != NULL);

        for (int i = 0; i < 100; i++) {
            char* temp = arena_sprintf(arena, "%s%d", result, i);
            TEST_CHECK(temp != NULL);
            result = temp;
        }

        TEST_CHECK(strlen(result) > 0);
        arena_release(arena);
    }
}

void test_complex_workflow(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        typedef struct {
            int* items;
            size_t count;
            size_t capacity;
        } IntArray;

        IntArray array = {0};

        for (int i = 0; i < 100; i++) {
            arena_da_append(arena, &array, i);
        }

        TempArena temp = arena_temp_begin(arena);

        char* str1 = arena_strdup(arena, "Hello");
        char* str2 = arena_strdup(arena, "World");

        char* combined = arena_sprintf(arena, "%s %s", str1, str2);

        TEST_CHECK(strcmp(combined, "Hello World") == 0);

        arena_temp_end(temp);

        TEST_CHECK(array.count == 100);
        for (int i = 0; i < 100; i++) {
            TEST_CHECK(array.items[i] == i);
        }

        arena_release(arena);
    }
}

TEST_LIST = {
    { "stress_small_allocs", test_stress_small_allocs },
    { "stress_large_allocs", test_stress_large_allocs },
    { "stress_1gb_allocation", test_stress_1gb_allocation },
    { "stress_mixed", test_stress_mixed },
    { "stress_temp_scopes", test_stress_temp_scopes },
    { "stress_realloc", test_stress_realloc },
    { "stress_da", test_stress_da },
    { "push_pop_pattern", test_push_pop_pattern },
    { "many_temp_scopes_nested", test_many_temp_scopes_nested },
    { "alignment_stress", test_alignment_stress },
    { "zero_fill_stress", test_zero_fill_stress },
    { "string_concatenation", test_string_concatenation },
    { "complex_workflow", test_complex_workflow },
    { NULL, NULL }
};
