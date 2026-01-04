#define ARENA_IMPLEMENTATION
#include "arena.h"
#include "base.h"
#define STB_SPRINTF_IMPLEMENTATION
#include "third_party/stb_sprintf.h"
#include "acutest.h"
#include <string.h>

void test_chain_creation(void)
{
    U64 page_size = getpagesize();
    Arena* arena = arena_create(page_size, page_size, page_size, ARENA_FLAG_NONE);
    TEST_CHECK(arena != NULL);
    if (arena) {
        void* ptr1 = arena_push(arena, page_size / 2, 8, 0);
        TEST_CHECK(ptr1 != NULL);

        void* ptr2 = arena_push(arena, page_size / 2 + 100, 8, 0);
        TEST_CHECK(ptr2 != NULL);

        TEST_CHECK(arena->current != arena);
        TEST_CHECK(arena->current->prev == arena);

        arena_release(arena);
    }
}

void test_chain_multiple_blocks(void)
{
    U64 page_size = getpagesize();
    Arena* arena = arena_create(page_size, page_size, page_size, ARENA_FLAG_NONE);
    TEST_CHECK(arena != NULL);
    if (arena) {
        void* ptrs[10];

        for (int i = 0; i < 10; i++) {
            ptrs[i] = arena_push(arena, page_size / 2, 8, 0);
            TEST_CHECK(ptrs[i] != NULL);
        }

        int count = 0;
        Arena* current = arena->current;
        while (current) {
            count++;
            current = current->prev;
        }
        TEST_CHECK(count > 1);

        arena_release(arena);
    }
}

void test_chain_allocations(void)
{
    U64 page_size = getpagesize();
    Arena* arena = arena_create(page_size, page_size, page_size, ARENA_FLAG_NONE);
    TEST_CHECK(arena != NULL);
    if (arena) {
        void* ptr1 = arena_push(arena, page_size / 2, 8, 0);
        TEST_CHECK(ptr1 != NULL);
        *(int*)ptr1 = 111;

        void* ptr2 = arena_push(arena, page_size / 2 + 100, 8, 0);
        TEST_CHECK(ptr2 != NULL);
        *(int*)ptr2 = 222;

        void* ptr3 = arena_push(arena, page_size / 2 + 200, 8, 0);
        TEST_CHECK(ptr3 != NULL);
        *(int*)ptr3 = 333;

        TEST_CHECK(*(int*)ptr1 == 111);
        TEST_CHECK(*(int*)ptr2 == 222);
        TEST_CHECK(*(int*)ptr3 == 333);

        arena_release(arena);
    }
}

void test_chain_pop(void)
{
    U64 page_size = getpagesize();
    Arena* arena = arena_create(page_size, page_size, page_size, ARENA_FLAG_NONE);
    TEST_CHECK(arena != NULL);
    if (arena) {
        U64 pos1 = arena_pos(arena);
        void* ptr1 = arena_push(arena, page_size / 2, 8, 0);
        *(int*)ptr1 = 111;

        U64 pos2 = arena_pos(arena);
        void* ptr2 = arena_push(arena, page_size / 2 + 100, 8, 0);

        U64 pos3 = arena_pos(arena);
        void* ptr3 = arena_push(arena, page_size / 2, 8, 0);

        arena_pop_to(arena, pos2);
        TEST_CHECK(arena_pos(arena) <= pos2);
        TEST_CHECK(*(int*)ptr1 == 111);

        arena_pop_to(arena, pos1);
        TEST_CHECK(arena_pos(arena) == pos1);

        arena_release(arena);
    }
}

void test_chain_no_chain_flag(void)
{
    U64 page_size = getpagesize();
    Arena* arena = arena_create(page_size * 2, page_size, page_size, ARENA_FLAG_NO_CHAIN);
    TEST_CHECK(arena != NULL);
    if (arena) {
        void* ptr1 = arena_push(arena, page_size / 2, 8, 0);
        TEST_CHECK(ptr1 != NULL);

        void* ptr2 = arena_push(arena, page_size / 2, 8, 0);
        TEST_CHECK(ptr2 != NULL);

        TEST_CHECK(arena->current == arena);

        arena_release(arena);
    }
}

void test_chain_pos_tracking(void)
{
    U64 page_size = getpagesize();
    Arena* arena = arena_create(page_size, page_size, page_size, ARENA_FLAG_NONE);
    TEST_CHECK(arena != NULL);
    if (arena) {
        U64 pos1 = arena_pos(arena);
        void* ptr1 = arena_push(arena, page_size / 2, 8, 0);

        U64 pos2 = arena_pos(arena);
        TEST_CHECK(pos2 > pos1);

        void* ptr2 = arena_push(arena, page_size / 2 + 100, 8, 0);

        U64 pos3 = arena_pos(arena);
        TEST_CHECK(pos3 > pos2);

        TEST_CHECK(arena->current != arena);

        arena_release(arena);
    }
}

void test_chain_clear(void)
{
    U64 page_size = getpagesize();
    Arena* arena = arena_create(page_size, page_size, page_size, ARENA_FLAG_NONE);
    TEST_CHECK(arena != NULL);
    if (arena) {
        for (int i = 0; i < 5; i++) {
            arena_push(arena, page_size / 2 + 100, 8, 0);
        }

        TEST_CHECK(arena->current != arena);

        arena_clear(arena);
        TEST_CHECK(arena_pos(arena) == ARENA_HEADER_SIZE);

        arena_release(arena);
    }
}

void test_chain_multiple_arenas(void)
{
    U64 page_size = getpagesize();
    Arena* arena1 = arena_create(page_size, page_size, page_size, ARENA_FLAG_NONE);
    Arena* arena2 = arena_create_scratch_default();
    TEST_CHECK(arena1 != NULL && arena2 != NULL);
    if (arena1 && arena2) {
        void* ptr1 = arena_push(arena1, page_size / 2, 8, 0);
        void* ptr2 = arena_push(arena2, 1024, 8, 0);

        TEST_CHECK(ptr1 != NULL);
        TEST_CHECK(ptr2 != NULL);

        arena_release(arena1);
        arena_release(arena2);
    }
}

TEST_LIST = {
    { "chain_creation", test_chain_creation },
    { "chain_multiple_blocks", test_chain_multiple_blocks },
    { "chain_allocations", test_chain_allocations },
    { "chain_pop", test_chain_pop },
    { "chain_no_chain_flag", test_chain_no_chain_flag },
    { "chain_pos_tracking", test_chain_pos_tracking },
    { "chain_clear", test_chain_clear },
    { "chain_multiple_arenas", test_chain_multiple_arenas },
    { NULL, NULL }
};
