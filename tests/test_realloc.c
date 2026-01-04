#define ARENA_IMPLEMENTATION
#include "arena.h"
#include "base.h"
#define STB_SPRINTF_IMPLEMENTATION
#include "third_party/stb_sprintf.h"
#include "acutest.h"
#include <string.h>
#include <stdlib.h>

void test_realloc_null(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        void* ptr = arena_realloc(arena, NULL, 0, 100);
        TEST_CHECK(ptr != NULL);
        TEST_CHECK(arena_pos(arena) > ARENA_HEADER_SIZE);
        arena_release(arena);
    }
}

void test_realloc_zero_old(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        void* ptr = arena_realloc(arena, (void*)0x12345678, 0, 100);
        TEST_CHECK(ptr != NULL);
        arena_release(arena);
    }
}

void test_realloc_in_place_grow(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        int* ptr = (int*)arena_push(arena, sizeof(int) * 10, 8, 0);
        for (int i = 0; i < 10; i++) ptr[i] = i;

        void* new_ptr = arena_realloc(arena, ptr, sizeof(int) * 10, sizeof(int) * 20);
        TEST_CHECK(new_ptr == ptr);

        for (int i = 0; i < 10; i++) {
            TEST_CHECK(ptr[i] == i);
        }
        for (int i = 10; i < 20; i++) {
            ptr[i] = i;
        }
        for (int i = 0; i < 20; i++) {
            TEST_CHECK(ptr[i] == i);
        }

        arena_release(arena);
    }
}

void test_realloc_in_place_commit_boundary(void)
{
    Arena* arena = arena_create(1ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    TEST_CHECK(arena != NULL);
    if (arena) {
        U64 initial_pos = arena_pos(arena);
        void* ptr = arena_push(arena, 32ULL * 1024, 8, 0);
        U64 pos_after_push = arena_pos(arena);

        void* new_ptr = arena_realloc(arena, ptr, 32ULL * 1024, 128ULL * 1024);
        TEST_CHECK(new_ptr == ptr);

        for (int i = 0; i < 32768; i++) {
            ((U8*)ptr)[i] = (U8)i;
        }
        for (int i = 32768; i < 131072; i++) {
            ((U8*)ptr)[i] = (U8)i;
        }

        arena_release(arena);
    }
}

void test_realloc_shrink(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        int* ptr = (int*)arena_push(arena, sizeof(int) * 20, 8, 0);
        for (int i = 0; i < 20; i++) ptr[i] = i;

        U64 pos_before = arena_pos(arena);

        void* new_ptr = arena_realloc(arena, ptr, sizeof(int) * 20, sizeof(int) * 10);
        TEST_CHECK(new_ptr == ptr);
        TEST_CHECK(arena_pos(arena) < pos_before);

        for (int i = 0; i < 10; i++) {
            TEST_CHECK(ptr[i] == i);
        }

        arena_release(arena);
    }
}

void test_realloc_not_last_allocation(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        int* a = (int*)arena_push(arena, sizeof(int) * 5, 8, 0);
        for (int i = 0; i < 5; i++) a[i] = i;

        int* b = (int*)arena_push(arena, sizeof(int) * 5, 8, 0);
        for (int i = 0; i < 5; i++) b[i] = i + 100;

        int* c = (int*)arena_push(arena, sizeof(int) * 5, 8, 0);
        for (int i = 0; i < 5; i++) c[i] = i + 200;

        int* new_b = (int*)arena_realloc(arena, b, sizeof(int) * 5, sizeof(int) * 10);
        TEST_CHECK(new_b != b);

        for (int i = 0; i < 5; i++) {
            TEST_CHECK(a[i] == i);
        }
        for (int i = 0; i < 5; i++) {
            TEST_CHECK(new_b[i] == i + 100);
        }

        arena_release(arena);
    }
}

void test_realloc_boxed_in(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        int* a = (int*)arena_push(arena, sizeof(int) * 10, 8, 0);
        int* b = (int*)arena_push(arena, sizeof(int) * 10, 8, 0);
        int* c = (int*)arena_push(arena, sizeof(int) * 10, 8, 0);

        for (int i = 0; i < 10; i++) {
            a[i] = i;
            b[i] = i + 100;
            c[i] = i + 200;
        }

        int* new_b = (int*)arena_realloc(arena, b, sizeof(int) * 10, sizeof(int) * 20);
        TEST_CHECK(new_b != b);

        for (int i = 0; i < 10; i++) {
            TEST_CHECK(a[i] == i);
            TEST_CHECK(new_b[i] == i + 100);
            TEST_CHECK(c[i] == i + 200);
        }

        arena_release(arena);
    }
}

void test_realloc_same_size(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        int* ptr = (int*)arena_push(arena, sizeof(int) * 10, 8, 0);
        for (int i = 0; i < 10; i++) ptr[i] = i;

        U64 pos_before = arena_pos(arena);

        void* new_ptr = arena_realloc(arena, ptr, sizeof(int) * 10, sizeof(int) * 10);
        TEST_CHECK(new_ptr == ptr);
        TEST_CHECK(arena_pos(arena) == pos_before);

        for (int i = 0; i < 10; i++) {
            TEST_CHECK(ptr[i] == i);
        }

        arena_release(arena);
    }
}

void test_realloc_large_grow(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        int* ptr = (int*)arena_push(arena, sizeof(int) * 10, 8, 0);
        for (int i = 0; i < 10; i++) ptr[i] = i;

        int* new_ptr = (int*)arena_realloc(arena, ptr, sizeof(int) * 10, sizeof(int) * 10000);
        TEST_CHECK(new_ptr != NULL);

        for (int i = 0; i < 10; i++) {
            TEST_CHECK(new_ptr[i] == i);
        }
        for (int i = 10; i < 10000; i++) {
            new_ptr[i] = i;
        }
        for (int i = 0; i < 10000; i++) {
            TEST_CHECK(new_ptr[i] == i);
        }

        arena_release(arena);
    }
}

void test_realloc_pattern(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        int* ptr = (int*)arena_push(arena, sizeof(int), 8, 0);
        *ptr = 42;

        for (int i = 1; i <= 100; i++) {
            ptr = (int*)arena_realloc(arena, ptr, sizeof(int) * i, sizeof(int) * (i + 1));
            TEST_CHECK(ptr != NULL);
            ptr[i] = i * 42;
        }

        for (int i = 0; i <= 100; i++) {
            if (i == 0) {
                TEST_CHECK(ptr[i] == 42);
            } else {
                TEST_CHECK(ptr[i] == i * 42);
            }
        }

        arena_release(arena);
    }
}

void test_realloc_with_alignment(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        int* a = (int*)arena_push(arena, sizeof(int), 8, 0);
        *a = 111;

        int* b = (int*)arena_push(arena, sizeof(int) * 10, 8, 0);
        for (int i = 0; i < 10; i++) b[i] = i;

        int* new_b = (int*)arena_realloc(arena, b, sizeof(int) * 10, sizeof(int) * 20);
        TEST_CHECK(new_b != NULL);

        TEST_CHECK(*a == 111);
        for (int i = 0; i < 10; i++) {
            TEST_CHECK(new_b[i] == i);
        }

        arena_release(arena);
    }
}

void test_realloc_multiple(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        void* ptrs[10];
        size_t sizes[10] = {10, 20, 15, 30, 25, 40, 35, 50, 45, 60};

        for (int i = 0; i < 10; i++) {
            ptrs[i] = arena_realloc(arena, NULL, 0, sizes[i]);
            TEST_CHECK(ptrs[i] != NULL);
        }

        for (int i = 0; i < 10; i++) {
            ptrs[i] = arena_realloc(arena, ptrs[i], sizes[i], sizes[i] * 2);
            TEST_CHECK(ptrs[i] != NULL);
        }

        arena_release(arena);
    }
}

TEST_LIST = {
    { "realloc_null", test_realloc_null },
    { "realloc_zero_old", test_realloc_zero_old },
    { "realloc_in_place_grow", test_realloc_in_place_grow },
    { "realloc_in_place_commit_boundary", test_realloc_in_place_commit_boundary },
    { "realloc_shrink", test_realloc_shrink },
    { "realloc_not_last_allocation", test_realloc_not_last_allocation },
    { "realloc_boxed_in", test_realloc_boxed_in },
    { "realloc_same_size", test_realloc_same_size },
    { "realloc_large_grow", test_realloc_large_grow },
    { "realloc_pattern", test_realloc_pattern },
    { "realloc_with_alignment", test_realloc_with_alignment },
    { "realloc_multiple", test_realloc_multiple },
    { NULL, NULL }
};
