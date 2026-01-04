#define ARENA_IMPLEMENTATION
#include "arena.h"
#include "base.h"
#define STB_SPRINTF_IMPLEMENTATION
#include "third_party/stb_sprintf.h"
#include "acutest.h"
#include <string.h>
#include <limits.h>

void test_arena_create_basic(void)
{
    Arena* arena = arena_create(1ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    TEST_CHECK(arena != NULL);
    if (arena) {
        TEST_CHECK(arena->reserved_size >= 1ULL * 1024 * 1024);
        TEST_CHECK(arena->commit_granularity == 64ULL * 1024);
        TEST_CHECK(arena->committed >= 64ULL * 1024);
        TEST_CHECK(arena->pos == ARENA_HEADER_SIZE);
        TEST_CHECK(arena->prev == NULL);
        TEST_CHECK(arena->current == arena);
        arena_release(arena);
    }
}

void test_arena_create_various_sizes(void)
{
    Arena* small = arena_create(64ULL * 1024, 4ULL * 1024, 4ULL * 1024, ARENA_FLAG_NONE);
    TEST_CHECK(small != NULL);
    if (small) arena_release(small);

    Arena* medium = arena_create(16ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    TEST_CHECK(medium != NULL);
    if (medium) arena_release(medium);

    Arena* large = arena_create(512ULL * 1024 * 1024, 256ULL * 1024, 256ULL * 1024, ARENA_FLAG_NONE);
    TEST_CHECK(large != NULL);
    if (large) arena_release(large);
}

void test_arena_create_alignment(void)
{
    U64 page_size = getpagesize();
    Arena* arena = arena_create(page_size + 123, page_size, page_size, ARENA_FLAG_NONE);
    TEST_CHECK(arena != NULL);
    if (arena) {
        TEST_CHECK(arena->reserved_size % page_size == 0);
        TEST_CHECK(arena->commit_granularity % page_size == 0);
        TEST_CHECK(arena->committed % page_size == 0);
        arena_release(arena);
    }
}

void test_arena_release_single(void)
{
    Arena* arena = arena_create(1ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    TEST_CHECK(arena != NULL);
    arena_release(arena);
}

void test_arena_create_scratch_default(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        TEST_CHECK(arena->reserved_size == 256ULL * 1024 * 1024);
        TEST_CHECK(arena->commit_granularity == 64ULL * 1024);
        TEST_CHECK(arena->committed >= 64ULL * 1024);
        TEST_CHECK(arena->pos == ARENA_HEADER_SIZE);
        TEST_CHECK(arena->flags == ARENA_FLAG_NONE);
        arena_release(arena);
    }
}

void test_arena_create_no_chain_flag(void)
{
    Arena* arena = arena_create(64ULL * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NO_CHAIN);
    TEST_CHECK(arena != NULL);
    if (arena) {
        TEST_CHECK((arena->flags & ARENA_FLAG_NO_CHAIN) != 0);
        arena_release(arena);
    }
}

void test_arena_create_zero_initial_commit(void)
{
    U64 page_size = getpagesize();
    Arena* arena = arena_create(1ULL * 1024 * 1024, 64ULL * 1024, page_size, ARENA_FLAG_NONE);
    TEST_CHECK(arena != NULL);
    if (arena) {
        TEST_CHECK(arena->committed >= ARENA_HEADER_SIZE);
        arena_release(arena);
    }
}

void test_arena_create_very_small_reserve(void)
{
    U64 page_size = getpagesize();
    Arena* arena = arena_create(page_size, page_size, page_size, ARENA_FLAG_NONE);
    TEST_CHECK(arena != NULL);
    if (arena) {
        TEST_CHECK(arena->reserved_size >= page_size);
        arena_release(arena);
    }
}

TEST_LIST = {
    { "arena_create_basic", test_arena_create_basic },
    { "arena_create_various_sizes", test_arena_create_various_sizes },
    { "arena_create_alignment", test_arena_create_alignment },
    { "arena_release_single", test_arena_release_single },
    { "arena_create_scratch_default", test_arena_create_scratch_default },
    { "arena_create_no_chain_flag", test_arena_create_no_chain_flag },
    { "arena_create_zero_initial_commit", test_arena_create_zero_initial_commit },
    { "arena_create_very_small_reserve", test_arena_create_very_small_reserve },
    { NULL, NULL }
};
