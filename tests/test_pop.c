#define ARENA_IMPLEMENTATION
#include "arena.h"
#include "base.h"
#define STB_SPRINTF_IMPLEMENTATION
#include "third_party/stb_sprintf.h"
#include "acutest.h"
#include <string.h>

void test_pop_to_basic(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        U64 pos1 = arena_pos(arena);
        int* a = (int*)arena_push(arena, sizeof(int) * 10, 8, 0);
        TEST_CHECK(a != NULL);

        U64 pos2 = arena_pos(arena);
        TEST_CHECK(pos2 > pos1);

        arena_pop_to(arena, pos1);
        U64 pos3 = arena_pos(arena);
        TEST_CHECK(pos3 == pos1);

        arena_release(arena);
    }
}

void test_pop_to_multiple(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        U64 pos[10];

        for (int i = 0; i < 10; i++) {
            pos[i] = arena_pos(arena);
            arena_push(arena, sizeof(int) * (i + 1), 8, 0);
        }

        arena_pop_to(arena, pos[5]);
        TEST_CHECK(arena_pos(arena) == pos[5]);

        arena_pop_to(arena, pos[2]);
        TEST_CHECK(arena_pos(arena) == pos[2]);

        arena_release(arena);
    }
}

void test_pop_to_header(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        arena_push(arena, 1024, 8, 0);
        TEST_CHECK(arena_pos(arena) > ARENA_HEADER_SIZE);

        arena_pop_to(arena, ARENA_HEADER_SIZE);
        TEST_CHECK(arena_pos(arena) == ARENA_HEADER_SIZE);

        arena_release(arena);
    }
}

void test_clear(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        arena_push(arena, 1024, 8, 0);
        TEST_CHECK(arena_pos(arena) > ARENA_HEADER_SIZE);

        arena_clear(arena);
        TEST_CHECK(arena_pos(arena) == ARENA_HEADER_SIZE);

        int* a = (int*)arena_push(arena, sizeof(int), 8, 0);
        TEST_CHECK(a != NULL);
        *a = 42;
        TEST_CHECK(*a == 42);

        arena_release(arena);
    }
}

void test_pos_tracking(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        U64 pos1 = arena_pos(arena);
        TEST_CHECK(pos1 == ARENA_HEADER_SIZE);

        arena_push(arena, 100, 8, 0);
        U64 pos2 = arena_pos(arena);
        TEST_CHECK(pos2 > pos1);

        arena_push(arena, 200, 16, 0);
        U64 pos3 = arena_pos(arena);
        TEST_CHECK(pos3 > pos2);

        arena_push(arena, 50, 32, 0);
        U64 pos4 = arena_pos(arena);
        TEST_CHECK(pos4 > pos3);

        arena_release(arena);
    }
}

void test_pop_with_alignment(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        U64 pos1 = arena_pos(arena);
        arena_push(arena, 10, 1, 0);

        U64 pos2 = arena_pos(arena);
        arena_push(arena, 10, 64, 0);

        U64 pos3 = arena_pos(arena);
        arena_push(arena, 10, 128, 0);

        arena_pop_to(arena, pos2);
        TEST_CHECK(arena_pos(arena) == pos2);

        arena_pop_to(arena, pos1);
        TEST_CHECK(arena_pos(arena) == pos1);

        arena_release(arena);
    }
}

void test_clear_multiple_times(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        for (int i = 0; i < 10; i++) {
            arena_push(arena, 100, 8, 0);
            TEST_CHECK(arena_pos(arena) > ARENA_HEADER_SIZE);
            arena_clear(arena);
            TEST_CHECK(arena_pos(arena) == ARENA_HEADER_SIZE);
        }
        arena_release(arena);
    }
}

void test_pop_then_allocate(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        U64 pos1 = arena_pos(arena);
        int* a = (int*)arena_push(arena, sizeof(int) * 10, 8, 0);
        TEST_CHECK(a != NULL);
        for (int i = 0; i < 10; i++) a[i] = i;

        arena_pop_to(arena, pos1);

        int* b = (int*)arena_push(arena, sizeof(int) * 5, 8, 0);
        TEST_CHECK(b != NULL);
        for (int i = 0; i < 5; i++) b[i] = i * 2;

        for (int i = 0; i < 5; i++) {
            TEST_CHECK(b[i] == i * 2);
        }

        arena_release(arena);
    }
}

void test_pop_to_intermediate_position(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        U64 positions[10];
        int* arrays[10];

        for (int i = 0; i < 10; i++) {
            positions[i] = arena_pos(arena);
            arrays[i] = (int*)arena_push(arena, sizeof(int) * (i + 1), 8, 0);
            for (int j = 0; j <= i; j++) {
                arrays[i][j] = i * 100 + j;
            }
        }

        arena_pop_to(arena, positions[7]);
        for (int j = 0; j <= 6; j++) {
            TEST_CHECK(arrays[6][j] == 600 + j);
        }

        arena_pop_to(arena, positions[3]);
        for (int j = 0; j <= 2; j++) {
            TEST_CHECK(arrays[2][j] == 200 + j);
        }

        arena_release(arena);
    }
}

TEST_LIST = {
    { "pop_to_basic", test_pop_to_basic },
    { "pop_to_multiple", test_pop_to_multiple },
    { "pop_to_header", test_pop_to_header },
    { "clear", test_clear },
    { "pos_tracking", test_pos_tracking },
    { "pop_with_alignment", test_pop_with_alignment },
    { "clear_multiple_times", test_clear_multiple_times },
    { "pop_then_allocate", test_pop_then_allocate },
    { "pop_to_intermediate_position", test_pop_to_intermediate_position },
    { NULL, NULL }
};
