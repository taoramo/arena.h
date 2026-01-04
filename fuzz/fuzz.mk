AFL_DIR ?= ../AFLplusplus
AFL_CC ?= $(AFL_DIR)/afl-clang-fast
CC ?= clang

CC = $(AFL_CC)
CFLAGS = -O2 -g -Wall -Wextra -DAFL_DRIVER=1 -I.. -DARENA_ABORT_ON_OOM=0
ASAN_FLAGS = -fno-omit-frame-pointer

FUZZERS = fuzz_arena_api fuzz_arena_push fuzz_arena_realloc fuzz_arena_temp

all: $(FUZZERS)

fuzz_arena_api: fuzz_arena_api.c
	$(CC) $(CFLAGS) $(ASAN_FLAGS) -o $@ $<

fuzz_arena_push: fuzz_arena_push.c
	$(CC) $(CFLAGS) $(ASAN_FLAGS) -o $@ $<

fuzz_arena_realloc: fuzz_arena_realloc.c
	$(CC) $(CFLAGS) $(ASAN_FLAGS) -o $@ $<

fuzz_arena_temp: fuzz_arena_temp.c
	$(CC) $(CFLAGS) $(ASAN_FLAGS) -o $@ $<

.PHONY: all clean run-api run-push run-realloc run-temp

clean:
	rm -f $(FUZZERS) *.o

run-api: fuzz_arena_api
	$(AFL_DIR)/afl-fuzz -i seeds -o out_api -- ./fuzz_arena_api @@

run-push: fuzz_arena_push
	$(AFL_DIR)/afl-fuzz -i seeds -o out_push -- ./fuzz_arena_push @@

run-realloc: fuzz_arena_realloc
	$(AFL_DIR)/afl-fuzz -i seeds -o out_realloc -- ./fuzz_arena_realloc @@

run-temp: fuzz_arena_temp
	$(AFL_DIR)/afl-fuzz -i seeds -o out_temp -- ./fuzz_arena_temp @@
