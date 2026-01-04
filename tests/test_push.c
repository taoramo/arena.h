#define ARENA_IMPLEMENTATION
#include "arena.h"
#include "base.h"
#define STB_SPRINTF_IMPLEMENTATION
#include "third_party/stb_sprintf.h"
#include "acutest.h"
#include <string.h>

void test_push_normal(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        int* ptr = (int*)arena_push(arena, sizeof(int) * 10, 8, 0);
        TEST_CHECK(ptr != NULL);
        for (int i = 0; i < 10; i++) {
            ptr[i] = i * 2;
            TEST_CHECK(ptr[i] == i * 2);
        }
        arena_release(arena);
    }
}

void test_push_zero_fill(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        int* ptr = (int*)arena_push(arena, sizeof(int) * 100, 8, 1);
        TEST_CHECK(ptr != NULL);
        for (int i = 0; i < 100; i++) {
            TEST_CHECK(ptr[i] == 0);
        }
        arena_release(arena);
    }
}

void test_push_various_alignments(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        void* a1 = arena_push(arena, 1, 1, 0);
        void* a2 = arena_push(arena, 1, 2, 0);
        void* a4 = arena_push(arena, 1, 4, 0);
        void* a8 = arena_push(arena, 1, 8, 0);
        void* a16 = arena_push(arena, 1, 16, 0);
        void* a32 = arena_push(arena, 1, 32, 0);
        void* a64 = arena_push(arena, 1, 64, 0);

        TEST_CHECK(a1 != NULL && (U64)a1 % 1 == 0);
        TEST_CHECK(a2 != NULL && (U64)a2 % 2 == 0);
        TEST_CHECK(a4 != NULL && (U64)a4 % 4 == 0);
        TEST_CHECK(a8 != NULL && (U64)a8 % 8 == 0);
        TEST_CHECK(a16 != NULL && (U64)a16 % 16 == 0);
        TEST_CHECK(a32 != NULL && (U64)a32 % 32 == 0);
        TEST_CHECK(a64 != NULL && (U64)a64 % 64 == 0);

        arena_release(arena);
    }
}

void test_push_large_alignment(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        void* ptr = arena_push(arena, 8, 256, 0);
        TEST_CHECK(ptr != NULL);
        TEST_CHECK((U64)ptr % 256 == 0);
        arena_release(arena);
    }
}

void test_push_size_zero(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        void* ptr = arena_push(arena, 0, 8, 0);
        TEST_CHECK(ptr != NULL);
        U64 pos1 = arena_pos(arena);
        ptr = arena_push(arena, 0, 8, 0);
        TEST_CHECK(ptr != NULL);
        U64 pos2 = arena_pos(arena);
        TEST_CHECK(pos1 == pos2);
        arena_release(arena);
    }
}

void test_push_exceed_commit(void)
{
    Arena* arena = arena_create(1ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    TEST_CHECK(arena != NULL);
    if (arena) {
        U64 initial_committed = arena->committed;
        void* ptr = arena_push(arena, 128ULL * 1024, 8, 0);
        TEST_CHECK(ptr != NULL);
        TEST_CHECK(arena->committed > initial_committed);
        arena_release(arena);
    }
}

void test_push_very_large(void)
{
    Arena* arena = arena_create(2ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    TEST_CHECK(arena != NULL);
    if (arena) {
        void* ptr = arena_push(arena, 1ULL * 1024 * 1024, 8, 0);
        TEST_CHECK(ptr != NULL);
        U8* data = (U8*)ptr;
        data[0] = 42;
        data[1024*1024-1] = 99;
        TEST_CHECK(data[0] == 42);
        TEST_CHECK(data[1024*1024-1] == 99);
        arena_release(arena);
    }
}

void test_push_1gb_allocation(void)
{
    Arena* arena = arena_create(2ULL * 1024 * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    TEST_CHECK(arena != NULL);
    if (arena) {
        void* ptr = arena_push(arena, 1ULL * 1024 * 1024 * 1024, 8, 0);
        TEST_CHECK(ptr != NULL);
        
        U8* data = (U8*)ptr;
        data[0] = 42;
        data[1024*1024*1024 - 1] = 99;
        TEST_CHECK(data[0] == 42);
        TEST_CHECK(data[1024*1024*1024 - 1] == 99);
        
        U64 committed_after = arena->committed;
        TEST_CHECK(committed_after >= 1ULL * 1024 * 1024 * 1024);
        
        arena_release(arena);
    }
}

void test_huge_address_reservation(void)
{
    U64 huge_reserve = 128ULL * 1024 * 1024 * 1024; // 16GB
    Arena* arena = arena_create(huge_reserve, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    TEST_CHECK(arena != NULL);
    if (arena) {
        TEST_CHECK(arena->reserved_size >= huge_reserve);
        
        void* ptr = arena_push(arena, 1ULL * 1024 * 1024, 8, 0);
        TEST_CHECK(ptr != NULL);
        
        U8* data = (U8*)ptr;
        data[0] = 123;
        data[1024*1024 - 1] = 200;
        TEST_CHECK(data[0] == 123);
        TEST_CHECK((U8)(data[1024*1024 - 1]) == (U8)200);
        
        arena_release(arena);
    }
}

void test_push_multiple(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        void* ptrs[1000];
        for (int i = 0; i < 1000; i++) {
            ptrs[i] = arena_push(arena, 64, 8, 0);
            TEST_CHECK(ptrs[i] != NULL);
            *(int*)ptrs[i] = i;
        }
        for (int i = 0; i < 1000; i++) {
            TEST_CHECK(*(int*)ptrs[i] == i);
        }
        arena_release(arena);
    }
}

void test_push_alignment_preserves_data(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        int* a = (int*)arena_push(arena, sizeof(int), 4, 0);
        *a = 12345;

        void* b = arena_push(arena, 1, 256, 0);

        int* c = (int*)arena_push(arena, sizeof(int), 4, 0);
        *c = 67890;

        TEST_CHECK(*a == 12345);
        TEST_CHECK(*c == 67890);
        TEST_CHECK((U64)b % 256 == 0);

        arena_release(arena);
    }
}

void test_push_unaligned_start(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        void* a = arena_push(arena, 1, 1, 0);
        void* b = arena_push(arena, 8, 8, 0);
        (void) a;
        (void) b;
        TEST_CHECK((U64)b % 8 == 0);
        arena_release(arena);
    }
}

void test_push_struct_alignment(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        typedef struct {
            char a;
            int b;
            long c;
        } TestStruct;

        TestStruct* s = (TestStruct*)arena_push(arena, sizeof(TestStruct), 8, 0);
        TEST_CHECK(s != NULL);
        TEST_CHECK((U64)s % 8 == 0);
        s->a = 'X';
        s->b = 42;
        s->c = 123456;
        TEST_CHECK(s->a == 'X');
        TEST_CHECK(s->b == 42);
        TEST_CHECK(s->c == 123456);
        arena_release(arena);
    }
}

TEST_LIST = {
    { "push_normal", test_push_normal },
    { "push_zero_fill", test_push_zero_fill },
    { "push_various_alignments", test_push_various_alignments },
    { "push_large_alignment", test_push_large_alignment },
    { "push_size_zero", test_push_size_zero },
    { "push_exceed_commit", test_push_exceed_commit },
    { "push_very_large", test_push_very_large },
    { "push_1gb_allocation", test_push_1gb_allocation },
    { "push_huge_address_reservation", test_huge_address_reservation },
    { "push_multiple", test_push_multiple },
    { "push_alignment_preserves_data", test_push_alignment_preserves_data },
    { "push_unaligned_start", test_push_unaligned_start },
    { "push_struct_alignment", test_push_struct_alignment },
    { NULL, NULL }
};
