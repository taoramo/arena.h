#define ARENA_IMPLEMENTATION
#include "arena.h"
#include "base.h"
#define STB_SPRINTF_IMPLEMENTATION
#include "third_party/stb_sprintf.h"
#include "acutest.h"
#include <string.h>

void test_strdup_normal(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        const char* orig = "Hello, World!";
        char* dup = arena_strdup(arena, orig);
        TEST_CHECK(dup != NULL);
        TEST_CHECK(strcmp(dup, orig) == 0);
        TEST_CHECK(dup != orig);
        arena_release(arena);
    }
}

void test_strdup_empty(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        char* dup = arena_strdup(arena, "");
        TEST_CHECK(dup != NULL);
        TEST_CHECK(strlen(dup) == 0);
        TEST_CHECK(dup[0] == '\0');
        arena_release(arena);
    }
}

void test_strdup_large(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        char large[1024 * 10];
        for (int i = 0; i < sizeof(large) - 1; i++) {
            large[i] = 'A' + (i % 26);
        }
        large[sizeof(large) - 1] = '\0';

        char* dup = arena_strdup(arena, large);
        TEST_CHECK(dup != NULL);
        TEST_CHECK(strcmp(dup, large) == 0);
        arena_release(arena);
    }
}

void test_strdup_special_chars(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        const char* specials = "Hello\x00World\t\n\r\\\"'";
        char* dup = arena_strdup(arena, specials);
        TEST_CHECK(dup != NULL);
        TEST_CHECK(memcmp(dup, specials, strlen(specials) + 1) == 0);
        arena_release(arena);
    }
}

void test_sprintf_basic(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        char* result = arena_sprintf(arena, "Number: %d", 42);
        TEST_CHECK(result != NULL);
        TEST_CHECK(strcmp(result, "Number: 42") == 0);
        arena_release(arena);
    }
}

void test_sprintf_multiple_args(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        char* result = arena_sprintf(arena, "Int: %d, Str: %s, Hex: 0x%x, Float: %.2f", 123, "test", 255, 3.14159f);
        TEST_CHECK(result != NULL);
        TEST_CHECK(strcmp(result, "Int: 123, Str: test, Hex: 0xff, Float: 3.14") == 0);
        arena_release(arena);
    }
}

void test_sprintf_empty(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        char* result = arena_sprintf(arena, "");
        TEST_CHECK(result != NULL);
        TEST_CHECK(strlen(result) == 0);
        arena_release(arena);
    }
}

void test_sprintf_large(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        char* result = arena_sprintf(arena, "%0500d", 12345);
        TEST_CHECK(result != NULL);
        TEST_CHECK(strlen(result) == 500);
        TEST_CHECK(strstr(result, "12345") != NULL);
        arena_release(arena);
    }
}

void test_sprintf_pointers(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        int x = 42;
        char* result = arena_sprintf(arena, "Pointer: %p", &x);
        TEST_CHECK(result != NULL);
        TEST_CHECK(strstr(result, "0x") != NULL || strstr(result, "0X") != NULL);
        arena_release(arena);
    }
}

void test_sprintf_multiple_calls(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        for (int i = 0; i < 100; i++) {
            char* result = arena_sprintf(arena, "Item %d", i);
            TEST_CHECK(result != NULL);
            TEST_CHECK(strcmp(result, "Item 0") == 0 || strcmp(result, "Item 99") == 0 || i > 0);
        }
        arena_release(arena);
    }
}

void test_vsprintf(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        char* result = arena_sprintf(arena, "Test: %d %s", 123, "value");
        TEST_CHECK(result != NULL);
        TEST_CHECK(strstr(result, "123") != NULL);
        TEST_CHECK(strstr(result, "value") != NULL);
        arena_release(arena);
    }
}

void test_sprintf_width_precision(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        char* result = arena_sprintf(arena, "[%10.5s]", "Hello, World!");
        TEST_CHECK(result != NULL);
        TEST_CHECK(strlen(result) == 12);
        arena_release(arena);
    }
}

TEST_LIST = {
    { "strdup_normal", test_strdup_normal },
    { "strdup_empty", test_strdup_empty },
    { "strdup_large", test_strdup_large },
    { "strdup_special_chars", test_strdup_special_chars },
    { "sprintf_basic", test_sprintf_basic },
    { "sprintf_multiple_args", test_sprintf_multiple_args },
    { "sprintf_empty", test_sprintf_empty },
    { "sprintf_large", test_sprintf_large },
    { "sprintf_pointers", test_sprintf_pointers },
    { "sprintf_multiple_calls", test_sprintf_multiple_calls },
    { "vsprintf", test_vsprintf },
    { "sprintf_width_precision", test_sprintf_width_precision },
    { NULL, NULL }
};
