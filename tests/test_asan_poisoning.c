#define ARENA_IMPLEMENTATION
#include "arena.h"
#include "base.h"
#define STB_SPRINTF_IMPLEMENTATION
#include "third_party/stb_sprintf.h"
#include "acutest.h"
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define ASAN_EXIT_CODE 1

static int run_in_child_process(void (*child_func)(void)) {
    pid_t pid = fork();
    
    if (pid == 0) {
        child_func();
        _exit(0);
    } else if (pid > 0) {
        int status;
        pid_t result = waitpid(pid, &status, 0);
        if (result == -1) {
            return 0;
        }
        
        if (WIFSIGNALED(status)) {
            int signal_num = WTERMSIG(status);
            if (signal_num == SIGABRT || signal_num == SIGSEGV || signal_num == SIGILL) {
                return 1;
            }
        }
        
        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            if (exit_code != 0) {
                return 1;
            }
        }
        
        return 0;
    }
    return 0;
}

static void child_access_after_pop(void) {
    Arena* arena = arena_create(1ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    if (!arena) _exit(1);
    
    U64 pos = arena_pos(arena);
    int* ptr = (int*)arena_push(arena, sizeof(int) * 10, 8, 0);
    if (!ptr) _exit(1);
    
    ptr[0] = 42;
    ptr[9] = 99;
    
    arena_pop_to(arena, pos);
    
    volatile int x = ptr[5];
    (void)x;
    
    arena_release(arena);
    _exit(0);
}

static void child_access_after_clear(void) {
    Arena* arena = arena_create(1ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    if (!arena) _exit(1);
    
    int* ptr = (int*)arena_push(arena, sizeof(int) * 100, 8, 0);
    if (!ptr) _exit(1);
    
    ptr[50] = 123;
    
    arena_clear(arena);
    
    volatile int x = ptr[50];
    (void)x;
    
    arena_release(arena);
    _exit(0);
}

static void child_access_after_temp_end(void) {
    Arena* arena = arena_create(1ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    if (!arena) _exit(1);
    
    TempArena temp = arena_temp_begin(arena);
    int* ptr = (int*)arena_push(arena, sizeof(int) * 50, 8, 0);
    if (!ptr) _exit(1);
    
    ptr[25] = 456;
    arena_temp_end(temp);
    
    volatile int x = ptr[25];
    (void)x;
    
    arena_release(arena);
    _exit(0);
}

static void child_one_byte_overflow(void) {
    Arena* arena = arena_create(1ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    if (!arena) _exit(1);
    
    U8* ptr = (U8*)arena_push(arena, 100, 1, 0);
    if (!ptr) _exit(1);
    
    ptr[0] = 1;
    ptr[99] = 100;
    
    volatile U8 x = ptr[100];
    (void)x;
    
    arena_release(arena);
    _exit(0);
}

static void child_one_byte_underflow(void) {
    Arena* arena = arena_create(1ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    if (!arena) _exit(1);
    
    U8* ptr = (U8*)arena_push(arena, 100, 1, 0);
    if (!ptr) _exit(1);
    
    ptr[0] = 1;
    ptr[99] = 100;
    
    volatile U8 x = ptr[-1];
    (void)x;
    
    arena_release(arena);
    _exit(0);
}

static void child_access_first_byte_of_poisoned(void) {
    Arena* arena = arena_create(1ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    if (!arena) _exit(1);
    
    U64 pos = arena_pos(arena);
    char* ptr = (char*)arena_push(arena, 1000, 1, 0);
    if (!ptr) _exit(1);
    
    ptr[0] = 'A';
    ptr[999] = 'Z';
    
    arena_pop_to(arena, pos);
    
    volatile char x = ptr[0];
    (void)x;
    
    arena_release(arena);
    _exit(0);
}

static void child_access_last_byte_of_poisoned(void) {
    Arena* arena = arena_create(1ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    if (!arena) _exit(1);
    
    U64 pos = arena_pos(arena);
    char* ptr = (char*)arena_push(arena, 1000, 1, 0);
    if (!ptr) _exit(1);
    
    ptr[0] = 'A';
    ptr[999] = 'Z';
    
    arena_pop_to(arena, pos);
    
    volatile char x = ptr[999];
    (void)x;
    
    arena_release(arena);
    _exit(0);
}

static void child_access_middle_of_poisoned(void) {
    Arena* arena = arena_create(1ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    if (!arena) _exit(1);
    
    U64 pos = arena_pos(arena);
    int* ptr = (int*)arena_push(arena, sizeof(int) * 100, 8, 0);
    if (!ptr) _exit(1);
    
    for (int i = 0; i < 100; i++) ptr[i] = i * 10;
    
    arena_pop_to(arena, pos);
    
    volatile int x = ptr[50];
    (void)x;
    
    arena_release(arena);
    _exit(0);
}

static void child_write_to_poisoned(void) {
    Arena* arena = arena_create(1ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    if (!arena) _exit(1);
    
    U64 pos = arena_pos(arena);
    int* ptr = (int*)arena_push(arena, sizeof(int) * 10, 8, 0);
    if (!ptr) _exit(1);
    
    arena_pop_to(arena, pos);
    
    ptr[5] = 999;
    
    arena_release(arena);
    _exit(0);
}

static void child_multiple_pops_access_old(void) {
    Arena* arena = arena_create(1ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    if (!arena) _exit(1);
    
    U64 positions[5];
    int* arrays[5];
    
    for (int i = 0; i < 5; i++) {
        positions[i] = arena_pos(arena);
        arrays[i] = (int*)arena_push(arena, sizeof(int) * 10, 8, 0);
        if (!arrays[i]) _exit(1);
    }
    
    arena_pop_to(arena, positions[2]);
    
    volatile int x = arrays[2][5];
    (void)x;
    
    arena_release(arena);
    _exit(0);
}

static void child_access_after_multiple_pops(void) {
    Arena* arena = arena_create(1ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    if (!arena) _exit(1);
    
    U64 pos1 = arena_pos(arena);
    int* ptr1 = (int*)arena_push(arena, sizeof(int) * 10, 8, 0);
    if (!ptr1) _exit(1);
    ptr1[0] = 555;
    
    U64 pos2 = arena_pos(arena);
    int* ptr2 = (int*)arena_push(arena, sizeof(int) * 10, 8, 0);
    if (!ptr2) _exit(1);
    ptr2[0] = 666;
    
    arena_pop_to(arena, pos2);
    
    volatile int x = ptr1[0];
    (void)x;
    
    arena_release(arena);
    _exit(0);
}

static void child_access_after_realloc_copy(void) {
    Arena* arena = arena_create(1ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    if (!arena) _exit(1);
    
    int* ptr1 = (int*)arena_push(arena, sizeof(int) * 10, 8, 0);
    if (!ptr1) _exit(1);
    for (int i = 0; i < 10; i++) ptr1[i] = i * 100;
    
    int* ptr2 = (int*)arena_push(arena, sizeof(int) * 10, 8, 0);
    if (!ptr2) _exit(1);
    for (int i = 0; i < 10; i++) ptr2[i] = i * 200;
    
    int* new_ptr1 = (int*)arena_realloc(arena, ptr1, sizeof(int) * 10, sizeof(int) * 20);
    if (!new_ptr1) _exit(1);
    
    volatile int x = ptr1[5];
    (void)x;
    
    arena_release(arena);
    _exit(0);
}

static void child_four_byte_overflow(void) {
    Arena* arena = arena_create(1ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    if (!arena) _exit(1);
    
    int* ptr = (int*)arena_push(arena, sizeof(int) * 10, 8, 0);
    if (!ptr) _exit(1);
    
    ptr[0] = 1;
    ptr[9] = 10;
    
    volatile int x = ptr[10];
    (void)x;
    
    arena_release(arena);
    _exit(0);
}

static void child_access_large_poisoned_region(void) {
    Arena* arena = arena_create(10ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    if (!arena) _exit(1);
    
    U64 pos = arena_pos(arena);
    U8* ptr = (U8*)arena_push(arena, 1024 * 1024, 8, 0);
    if (!ptr) _exit(1);
    
    for (U64 i = 0; i < 1024 * 1024; i++) ptr[i] = (U8)(i % 256);
    
    arena_pop_to(arena, pos);
    
    volatile U8 x = ptr[512 * 1024];
    (void)x;
    
    arena_release(arena);
    _exit(0);
}

static void child_string_access_after_poison(void) {
    Arena* arena = arena_create(1ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    if (!arena) _exit(1);
    
    U64 pos = arena_pos(arena);
    char* str = arena_strdup(arena, "Hello, World!");
    if (!str) _exit(1);
    
    arena_pop_to(arena, pos);
    
    volatile char x = str[6];
    (void)x;
    
    arena_release(arena);
    _exit(0);
}

static void child_memdup_access_after_poison(void) {
    Arena* arena = arena_create(1ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    if (!arena) _exit(1);
    
    U64 pos = arena_pos(arena);
    int data[] = {1, 2, 3, 4, 5};
    int* copy = (int*)arena_memdup(arena, data, sizeof(data));
    if (!copy) _exit(1);
    
    arena_pop_to(arena, pos);
    
    volatile int x = copy[2];
    (void)x;
    
    arena_release(arena);
    _exit(0);
}

static void child_access_aligned_poisoned(void) {
    Arena* arena = arena_create(1ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    if (!arena) _exit(1);
    
    U64 pos = arena_pos(arena);
    long* ptr = (long*)arena_push(arena, sizeof(long) * 100, 64, 0);
    if (!ptr) _exit(1);
    
    ptr[0] = 111;
    ptr[99] = 999;
    
    arena_pop_to(arena, pos);
    
    volatile long x = ptr[50];
    (void)x;
    
    arena_release(arena);
    _exit(0);
}

void test_asan_poisoned_after_pop(void) {
    int result = run_in_child_process(child_access_after_pop);
    TEST_CHECK(result == 1);
}

void test_asan_poisoned_after_clear(void) {
    int result = run_in_child_process(child_access_after_clear);
    TEST_CHECK(result == 1);
}

void test_asan_poisoned_after_temp_end(void) {
    int result = run_in_child_process(child_access_after_temp_end);
    TEST_CHECK(result == 1);
}

void test_asan_one_byte_overflow(void) {
    int result = run_in_child_process(child_one_byte_overflow);
    TEST_CHECK(result == 1);
}

void test_asan_one_byte_underflow(void) {
    int result = run_in_child_process(child_one_byte_underflow);
    TEST_CHECK(result == 1);
}

void test_asan_access_first_byte_of_poisoned(void) {
    int result = run_in_child_process(child_access_first_byte_of_poisoned);
    TEST_CHECK(result == 1);
}

void test_asan_access_last_byte_of_poisoned(void) {
    int result = run_in_child_process(child_access_last_byte_of_poisoned);
    TEST_CHECK(result == 1);
}

void test_asan_access_middle_of_poisoned(void) {
    int result = run_in_child_process(child_access_middle_of_poisoned);
    TEST_CHECK(result == 1);
}

void test_asan_write_to_poisoned(void) {
    int result = run_in_child_process(child_write_to_poisoned);
    TEST_CHECK(result == 1);
}

void test_asan_multiple_pops_access_old(void) {
    int result = run_in_child_process(child_multiple_pops_access_old);
    TEST_CHECK(result == 1);
}

void test_asan_access_after_multiple_pops(void) {
    int result = run_in_child_process(child_access_after_multiple_pops);
    TEST_CHECK(result == 1);
}

void test_asan_access_after_realloc_copy(void) {
    int result = run_in_child_process(child_access_after_realloc_copy);
    TEST_CHECK(result == 1);
}

void test_asan_four_byte_overflow(void) {
    int result = run_in_child_process(child_four_byte_overflow);
    TEST_CHECK(result == 1);
}

void test_asan_access_large_poisoned_region(void) {
    int result = run_in_child_process(child_access_large_poisoned_region);
    TEST_CHECK(result == 1);
}

void test_asan_string_access_after_poison(void) {
    int result = run_in_child_process(child_string_access_after_poison);
    TEST_CHECK(result == 1);
}

void test_asan_memdup_access_after_poison(void) {
    int result = run_in_child_process(child_memdup_access_after_poison);
    TEST_CHECK(result == 1);
}

void test_asan_access_aligned_poisoned(void) {
    int result = run_in_child_process(child_access_aligned_poisoned);
    TEST_CHECK(result == 1);
}

void test_asan_unpoisoned_access_works(void) {
    Arena* arena = arena_create(1ULL * 1024 * 1024, 64ULL * 1024, 64ULL * 1024, ARENA_FLAG_NONE);
    TEST_CHECK(arena != NULL);
    if (arena) {
        int* ptr = (int*)arena_push(arena, sizeof(int) * 10, 8, 0);
        TEST_CHECK(ptr != NULL);
        
        ptr[0] = 42;
        ptr[5] = 555;
        ptr[9] = 999;
        
        TEST_CHECK(ptr[0] == 42);
        TEST_CHECK(ptr[5] == 555);
        TEST_CHECK(ptr[9] == 999);
        
        arena_release(arena);
    }
}

TEST_LIST = {
    { "asan_poisoned_after_pop", test_asan_poisoned_after_pop },
    { "asan_poisoned_after_clear", test_asan_poisoned_after_clear },
    { "asan_poisoned_after_temp_end", test_asan_poisoned_after_temp_end },
    { "asan_one_byte_overflow", test_asan_one_byte_overflow },
    { "asan_one_byte_underflow", test_asan_one_byte_underflow },
    { "asan_access_first_byte_of_poisoned", test_asan_access_first_byte_of_poisoned },
    { "asan_access_last_byte_of_poisoned", test_asan_access_last_byte_of_poisoned },
    { "asan_access_middle_of_poisoned", test_asan_access_middle_of_poisoned },
    { "asan_write_to_poisoned", test_asan_write_to_poisoned },
    { "asan_multiple_pops_access_old", test_asan_multiple_pops_access_old },
    { "asan_access_after_multiple_pops", test_asan_access_after_multiple_pops },
    { "asan_access_after_realloc_copy", test_asan_access_after_realloc_copy },
    { "asan_four_byte_overflow", test_asan_four_byte_overflow },
    { "asan_access_large_poisoned_region", test_asan_access_large_poisoned_region },
    { "asan_string_access_after_poison", test_asan_string_access_after_poison },
    { "asan_memdup_access_after_poison", test_asan_memdup_access_after_poison },
    { "asan_access_aligned_poisoned", test_asan_access_aligned_poisoned },
    { "asan_unpoisoned_access_works", test_asan_unpoisoned_access_works },
    { NULL, NULL }
};
