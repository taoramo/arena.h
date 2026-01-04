#define ARENA_IMPLEMENTATION
#include "arena.h"
#include "base.h"
#define STB_SPRINTF_IMPLEMENTATION
#include "third_party/stb_sprintf.h"
#include "acutest.h"
#include <string.h>

void test_temp_basic(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        TempArena temp = arena_temp_begin(arena);
        int* a = (int*)arena_push(arena, sizeof(int) * 100, 8, 0);
        TEST_CHECK(a != NULL);
        for (int i = 0; i < 100; i++) a[i] = i;
        arena_temp_end(temp);

        U64 pos = arena_pos(arena);
        TEST_CHECK(pos == ARENA_HEADER_SIZE);

        int* b = (int*)arena_push(arena, sizeof(int), 8, 0);
        TEST_CHECK(b != NULL);
        *b = 42;
        TEST_CHECK(*b == 42);

        arena_release(arena);
    }
}

void test_temp_nested(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        TempArena temp1 = arena_temp_begin(arena);
        int* a = (int*)arena_push(arena, sizeof(int), 8, 0);
        *a = 100;

        TempArena temp2 = arena_temp_begin(arena);
        int* b = (int*)arena_push(arena, sizeof(int), 8, 0);
        *b = 200;

        arena_temp_end(temp2);
        TEST_CHECK(*a == 100);

        arena_temp_end(temp1);
        TEST_CHECK(arena_pos(arena) == ARENA_HEADER_SIZE);

        arena_release(arena);
    }
}

void test_temp_large_allocation(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        TempArena temp = arena_temp_begin(arena);
        void* big = arena_push(arena, 1024ULL * 1024, 8, 0);
        TEST_CHECK(big != NULL);
        U8* data = (U8*)big;
        for (U64 i = 0; i < 1024 * 1024; i++) {
            data[i] = (U8)(i % 256);
        }
        arena_temp_end(temp);

        U64 pos = arena_pos(arena);
        TEST_CHECK(pos == ARENA_HEADER_SIZE);

        arena_release(arena);
    }
}

void test_temp_multiple_allocations(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        U64 pos_before = arena_pos(arena);

        TempArena temp = arena_temp_begin(arena);
        for (int i = 0; i < 100; i++) {
            int* p = (int*)arena_push(arena, sizeof(int), 8, 0);
            TEST_CHECK(p != NULL);
            *p = i;
        }
        arena_temp_end(temp);

        U64 pos_after = arena_pos(arena);
        TEST_CHECK(pos_before == pos_after);

        arena_release(arena);
    }
}

void test_temp_preserves_data_outside(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        int* outside = (int*)arena_push(arena, sizeof(int), 8, 0);
        *outside = 12345;

        TempArena temp = arena_temp_begin(arena);
        int* inside = (int*)arena_push(arena, sizeof(int), 8, 0);
        *inside = 67890;
        arena_temp_end(temp);

        TEST_CHECK(*outside == 12345);

        arena_release(arena);
    }
}

void test_temp_reuse(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        for (int i = 0; i < 10; i++) {
            TempArena temp = arena_temp_begin(arena);
            int* a = (int*)arena_push(arena, sizeof(int) * (i + 1), 8, 0);
            for (int j = 0; j <= i; j++) a[j] = i * 100 + j;
            arena_temp_end(temp);
        }

        TEST_CHECK(arena_pos(arena) == ARENA_HEADER_SIZE);

        arena_release(arena);
    }
}

void test_temp_with_alignment(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        int* a = (int*)arena_push(arena, sizeof(int), 8, 0);
        *a = 111;

        TempArena temp = arena_temp_begin(arena);
        void* b = arena_push(arena, 1, 256, 0);
        TEST_CHECK((U64)b % 256 == 0);
        void* c = arena_push(arena, 1, 512, 0);
        TEST_CHECK((U64)c % 512 == 0);
        arena_temp_end(temp);

        TEST_CHECK(*a == 111);

        arena_release(arena);
    }
}

TEST_LIST = {
    { "temp_basic", test_temp_basic },
    { "temp_nested", test_temp_nested },
    { "temp_large_allocation", test_temp_large_allocation },
    { "temp_multiple_allocations", test_temp_multiple_allocations },
    { "temp_preserves_data_outside", test_temp_preserves_data_outside },
    { "temp_reuse", test_temp_reuse },
    { "temp_with_alignment", test_temp_with_alignment },
    { NULL, NULL }
};
