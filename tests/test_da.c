#define ARENA_IMPLEMENTATION
#include "arena.h"
#include "base.h"
#define STB_SPRINTF_IMPLEMENTATION
#include "third_party/stb_sprintf.h"
#include "acutest.h"
#include <string.h>
#include <math.h>

void test_da_append_single(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        typedef struct {
            char* items;
            size_t count;
            size_t capacity;
        } CharArray;

        CharArray array = {0};

        for (int i = 0; i < 100; i++) {
            char item = (char)('A' + i % 26);
            arena_da_append(arena, &array, item);
        }

        TEST_CHECK(array.count == 100);
        TEST_CHECK(array.items != NULL);

        for (size_t i = 0; i < array.count; i++) {
            TEST_CHECK(array.items[i] == (char)('A' + i % 26));
        }

        arena_release(arena);
    }
}

void test_da_append_many(void)
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

        int batch1[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        int batch2[10] = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19};

        arena_da_append_many(arena, &array, batch1, 10);
        TEST_CHECK(array.count == 10);

        arena_da_append_many(arena, &array, batch2, 10);
        TEST_CHECK(array.count == 20);

        for (size_t i = 0; i < 20; i++) {
            TEST_CHECK(array.items[i] == (int)i);
        }

        arena_release(arena);
    }
}

void test_da_growth_from_zero(void)
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
        TEST_CHECK(array.capacity == 0);
        TEST_CHECK(array.items == NULL);

        arena_da_append(arena, &array, 42);
        TEST_CHECK(array.capacity >= 1);
        TEST_CHECK(array.count == 1);
        TEST_CHECK(array.items[0] == 42);

        arena_release(arena);
    }
}

void test_da_growth_multiple(void)
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

        for (int i = 0; i < 1000; i++) {
            arena_da_append(arena, &array, i);
        }

        TEST_CHECK(array.count == 1000);
        TEST_CHECK(array.capacity >= 1000);

        for (int i = 0; i < 1000; i++) {
            TEST_CHECK(array.items[i] == i);
        }

        arena_release(arena);
    }
}

void test_da_various_types(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        typedef struct {
            char* items;
            size_t count;
            size_t capacity;
        } CharArray;

        CharArray chars = {0};
        arena_da_append(arena, &chars, 'X');
        TEST_CHECK(chars.count == 1);
        TEST_CHECK(chars.items[0] == 'X');

        typedef struct {
            int* items;
            size_t count;
            size_t capacity;
        } IntArray;

        IntArray ints = {0};
        arena_da_append(arena, &ints, 12345);
        TEST_CHECK(ints.count == 1);
        TEST_CHECK(ints.items[0] == 12345);

        typedef struct {
            double* items;
            size_t count;
            size_t capacity;
        } DoubleArray;

        DoubleArray doubles = {0};
        arena_da_append(arena, &doubles, 3.14159);
        TEST_CHECK(doubles.count == 1);
        TEST_CHECK(fabs(doubles.items[0] - 3.14159) < 0.00001);

        arena_release(arena);
    }
}

void test_da_alignment(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        typedef struct {
            double* items;
            size_t count;
            size_t capacity;
        } DoubleArray;

        DoubleArray array = {0};

        for (int i = 0; i < 100; i++) {
            arena_da_append(arena, &array, (double)i);
        }

        TEST_CHECK(array.count == 100);
        TEST_CHECK((U64)array.items % 8 == 0);

        for (int i = 0; i < 100; i++) {
            TEST_CHECK(array.items[i] == (double)i);
        }

        arena_release(arena);
    }
}

void test_da_large(void)
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

        const size_t count = 100000;
        for (size_t i = 0; i < count; i++) {
            arena_da_append(arena, &array, (int)i);
        }

        TEST_CHECK(array.count == count);

        for (size_t i = 0; i < count; i++) {
            TEST_CHECK(array.items[i] == (int)i);
        }

        arena_release(arena);
    }
}

void test_da_struct(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        typedef struct {
            int a;
            double b;
            char c;
        } TestStruct;

        typedef struct {
            TestStruct* items;
            size_t count;
            size_t capacity;
        } StructArray;

        StructArray array = {0};

        for (int i = 0; i < 50; i++) {
            TestStruct s = {i, (double)i * 1.5, (char)('A' + i % 26)};
            arena_da_append(arena, &array, s);
        }

        TEST_CHECK(array.count == 50);

        for (int i = 0; i < 50; i++) {
            TEST_CHECK(array.items[i].a == i);
            TEST_CHECK(fabs(array.items[i].b - (double)i * 1.5) < 0.00001);
            TEST_CHECK(array.items[i].c == (char)('A' + i % 26));
        }

        arena_release(arena);
    }
}

void test_da_pointer_array(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        typedef struct {
            char** items;
            size_t count;
            size_t capacity;
        } StringArray;

        StringArray array = {0};

        char* str1 = arena_strdup(arena, "Hello");
        char* str2 = arena_strdup(arena, "World");
        char* str3 = arena_strdup(arena, "Test");

        arena_da_append(arena, &array, str1);
        arena_da_append(arena, &array, str2);
        arena_da_append(arena, &array, str3);

        TEST_CHECK(array.count == 3);
        TEST_CHECK(strcmp(array.items[0], "Hello") == 0);
        TEST_CHECK(strcmp(array.items[1], "World") == 0);
        TEST_CHECK(strcmp(array.items[2], "Test") == 0);

        arena_release(arena);
    }
}

TEST_LIST = {
    { "da_append_single", test_da_append_single },
    { "da_append_many", test_da_append_many },
    { "da_growth_from_zero", test_da_growth_from_zero },
    { "da_growth_multiple", test_da_growth_multiple },
    { "da_various_types", test_da_various_types },
    { "da_alignment", test_da_alignment },
    { "da_large", test_da_large },
    { "da_struct", test_da_struct },
    { "da_pointer_array", test_da_pointer_array },
    { NULL, NULL }
};
