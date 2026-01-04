#define ARENA_IMPLEMENTATION
#include "arena.h"
#include "base.h"
#define STB_SPRINTF_IMPLEMENTATION
#include "third_party/stb_sprintf.h"
#include "acutest.h"
#include <string.h>

void test_memdup_basic(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        int original[] = {1, 2, 3, 4, 5};
        int* copy = (int*)arena_memdup(arena, original, sizeof(original));
        TEST_CHECK(copy != NULL);
        TEST_CHECK(copy != original);
        TEST_CHECK(memcmp(copy, original, sizeof(original)) == 0);
        arena_release(arena);
    }
}

void test_memdup_zero(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        void* copy = arena_memdup(arena, "test", 0);
        TEST_CHECK(copy != NULL);
        arena_release(arena);
    }
}

void test_memdup_large(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        char data[1024 * 100];
        for (int i = 0; i < sizeof(data); i++) {
            data[i] = (char)(i % 256);
        }

        char* copy = (char*)arena_memdup(arena, data, sizeof(data));
        TEST_CHECK(copy != NULL);
        TEST_CHECK(memcmp(copy, data, sizeof(data)) == 0);

        for (int i = 0; i < sizeof(data); i++) {
            TEST_CHECK(copy[i] == (char)(i % 256));
        }
        arena_release(arena);
    }
}

void test_memdup_various_types(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        struct TestStruct {
            char c;
            int i;
            double d;
            long l;
        } original = {'A', 42, 3.14159, 99999L};

        struct TestStruct* copy = (struct TestStruct*)arena_memdup(arena, &original, sizeof(original));
        TEST_CHECK(copy != NULL);
        TEST_CHECK(copy->c == original.c);
        TEST_CHECK(copy->i == original.i);
        TEST_CHECK(copy->d == original.d);
        TEST_CHECK(copy->l == original.l);

        arena_release(arena);
    }
}

void test_memdup_alignment(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        double data[] = {1.1, 2.2, 3.3, 4.4, 5.5};
        double* copy = (double*)arena_memdup(arena, data, sizeof(data));
        TEST_CHECK(copy != NULL);
        TEST_CHECK((U64)copy % 8 == 0);
        TEST_CHECK(memcmp(copy, data, sizeof(data)) == 0);
        arena_release(arena);
    }
}

void test_memdup_preserves_original(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        char original[] = "Original data";
        char* copy = (char*)arena_memdup(arena, original, strlen(original));

        strcpy(original, "Modified");
        TEST_CHECK(strcmp(copy, "Original data") == 0);
        TEST_CHECK(strcmp(original, "Modified") == 0);

        arena_release(arena);
    }
}

void test_memdup_multiple(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        char* copies[100];
        char data[256];
        for (int i = 0; i < 256; i++) data[i] = (char)i;

        for (int i = 0; i < 100; i++) {
            copies[i] = (char*)arena_memdup(arena, data, sizeof(data));
            TEST_CHECK(copies[i] != NULL);
            TEST_CHECK(memcmp(copies[i], data, sizeof(data)) == 0);
        }

        for (int i = 0; i < 100; i++) {
            for (int j = 0; j < 256; j++) {
                TEST_CHECK(copies[i][j] == (char)j);
            }
        }

        arena_release(arena);
    }
}

void test_memdup_with_null_bytes(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        char data[] = {0, 1, 2, 0, 3, 4, 0, 5};
        char* copy = (char*)arena_memdup(arena, data, sizeof(data));
        TEST_CHECK(copy != NULL);
        TEST_CHECK(memcmp(copy, data, sizeof(data)) == 0);
        arena_release(arena);
    }
}

TEST_LIST = {
    { "memdup_basic", test_memdup_basic },
    { "memdup_zero", test_memdup_zero },
    { "memdup_large", test_memdup_large },
    { "memdup_various_types", test_memdup_various_types },
    { "memdup_alignment", test_memdup_alignment },
    { "memdup_preserves_original", test_memdup_preserves_original },
    { "memdup_multiple", test_memdup_multiple },
    { "memdup_with_null_bytes", test_memdup_with_null_bytes },
    { NULL, NULL }
};
