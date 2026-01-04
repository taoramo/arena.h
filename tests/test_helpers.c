#define ARENA_IMPLEMENTATION
#include "arena.h"
#include "base.h"
#define STB_SPRINTF_IMPLEMENTATION
#include "third_party/stb_sprintf.h"
#include "acutest.h"
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

void test_split_basic(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        StringArray result = arena_split(arena, "a,b,c", ",");
        TEST_CHECK(result.count == 3);
        TEST_CHECK(strcmp(result.items[0], "a") == 0);
        TEST_CHECK(strcmp(result.items[1], "b") == 0);
        TEST_CHECK(strcmp(result.items[2], "c") == 0);
        arena_release(arena);
    }
}

void test_split_multiple_delims(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        StringArray result = arena_split(arena, "a b,c;d", " ,;");
        TEST_CHECK(result.count == 4);
        TEST_CHECK(strcmp(result.items[0], "a") == 0);
        TEST_CHECK(strcmp(result.items[1], "b") == 0);
        TEST_CHECK(strcmp(result.items[2], "c") == 0);
        TEST_CHECK(strcmp(result.items[3], "d") == 0);
        arena_release(arena);
    }
}

void test_split_consecutive_delims(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        StringArray result = arena_split(arena, "a,,b,,c", ",");
        TEST_CHECK(result.count == 3);
        TEST_CHECK(strcmp(result.items[0], "a") == 0);
        TEST_CHECK(strcmp(result.items[1], "b") == 0);
        TEST_CHECK(strcmp(result.items[2], "c") == 0);
        arena_release(arena);
    }
}

void test_split_edge_cases(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        StringArray result1 = arena_split(arena, ",a,b,c,", ",");
        TEST_CHECK(result1.count == 3);

        StringArray result2 = arena_split(arena, "a,b,c", ",");
        TEST_CHECK(result2.count == 3);

        arena_release(arena);
    }
}

void test_split_empty_string(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        StringArray result = arena_split(arena, "", ",");
        TEST_CHECK(result.count == 0);
        arena_release(arena);
    }
}

void test_split_no_delims(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        StringArray result = arena_split(arena, "helloworld", ",");
        TEST_CHECK(result.count == 1);
        TEST_CHECK(strcmp(result.items[0], "helloworld") == 0);
        arena_release(arena);
    }
}

void test_split_whitespace(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        StringArray result = arena_split(arena, "  hello   world  test  ", " \t");
        TEST_CHECK(result.count == 3);
        TEST_CHECK(strcmp(result.items[0], "hello") == 0);
        TEST_CHECK(strcmp(result.items[1], "world") == 0);
        TEST_CHECK(strcmp(result.items[2], "test") == 0);
        arena_release(arena);
    }
}

void test_list_filenames_valid_dir(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        StringArray result;
        B32 success = arena_list_filenames(arena, ".", &result);
        TEST_CHECK(success);
        TEST_CHECK(result.count > 0);

        for (size_t i = 0; i < result.count; i++) {
            TEST_CHECK(result.items[i] != NULL);
            TEST_CHECK(strlen(result.items[i]) > 0);
        }

        arena_release(arena);
    }
}

void test_list_filenames_nonexistent(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        StringArray result;
        B32 success = arena_list_filenames(arena, "/nonexistent/directory/12345", &result);
        TEST_CHECK(!success);
        TEST_CHECK(result.count == 0);
        arena_release(arena);
    }
}

void test_list_filenames_empty_dir(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        char template[] = "/tmp/arena_test_XXXXXX";
        char* tmpdir = mkdtemp(template);
        TEST_CHECK(tmpdir != NULL);

        if (tmpdir) {
            StringArray result;
            B32 success = arena_list_filenames(arena, tmpdir, &result);
            TEST_CHECK(success);
            TEST_CHECK(result.count == 0);

            rmdir(tmpdir);
        }

        arena_release(arena);
    }
}

void test_list_filenames_filtering(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        StringArray result;
        B32 success = arena_list_filenames(arena, ".", &result);
        TEST_CHECK(success);

        for (size_t i = 0; i < result.count; i++) {
            const char* name = result.items[i];
            TEST_CHECK(name != NULL);

            TEST_CHECK(!(name[0] == '.' && (name[1] == '\0' ||
                                           (name[1] == '.' && name[2] == '\0'))));
        }

        arena_release(arena);
    }
}

void test_list_filenames_test_dir(void)
{
    Arena* arena = arena_create_scratch_default();
    TEST_CHECK(arena != NULL);
    if (arena) {
        StringArray result;
        B32 success = arena_list_filenames(arena, ".", &result);
        TEST_CHECK(success);
        TEST_CHECK(result.count > 0);

        B32 found_acutest = 0;
        for (size_t i = 0; i < result.count; i++) {
            if (strstr(result.items[i], "acutest") != NULL) {
                found_acutest = 1;
            }
        }
        TEST_CHECK(found_acutest);

        arena_release(arena);
    }
}

TEST_LIST = {
    { "split_basic", test_split_basic },
    { "split_multiple_delims", test_split_multiple_delims },
    { "split_consecutive_delims", test_split_consecutive_delims },
    { "split_edge_cases", test_split_edge_cases },
    { "split_empty_string", test_split_empty_string },
    { "split_no_delims", test_split_no_delims },
    { "split_whitespace", test_split_whitespace },
    { "list_filenames_valid_dir", test_list_filenames_valid_dir },
    { "list_filenames_nonexistent", test_list_filenames_nonexistent },
    { "list_filenames_empty_dir", test_list_filenames_empty_dir },
    { "list_filenames_filtering", test_list_filenames_filtering },
    { "list_filenames_test_dir", test_list_filenames_test_dir },
    { NULL, NULL }
};
