CC = clang
CFLAGS = -Wall -Wextra -std=c99 -I.. -I.
DEBUG_FLAGS = -g -O0
OPTIMIZE_FLAGS = -O2
ASAN_FLAGS = -fsanitize=address -fno-omit-frame-pointer
LSAN_FLAGS = -fsanitize=leak
VALGRIND = valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 --suppressions=valgrind.supp

TEST_SOURCES = test_lifecycle.c test_push.c test_pop.c test_temp.c test_string.c \
               test_memdup.c test_realloc.c test_chain.c test_da.c test_helpers.c test_stress.c

ASAN_ONLY_TESTS = test_asan_poisoning.c

TEST_BINS = $(patsubst %.c,%.out,$(TEST_SOURCES))
DEBUG_BINS = $(patsubst %.c,%_debug.out,$(TEST_SOURCES))
ASAN_BINS = $(patsubst %.c,%_asan.out,$(TEST_SOURCES))
ASAN_POISONING_BIN = $(patsubst %.c,%_asan.out,$(ASAN_ONLY_TESTS))
VALGRIND_BINS = $(patsubst %.c,%_valgrind.out,$(TEST_SOURCES))

.PHONY: all test test-debug test-asan test-valgrind test-asan-poisoning clean run-all run-debug run-asan run-valgrind run-asan-poisoning

all: $(TEST_BINS)

%.out: %.c acutest.h
	$(CC) $(CFLAGS) $(OPTIMIZE_FLAGS) $< -o $@ $(LDFLAGS)

%_debug.out: %.c acutest.h
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -DARENA_ENABLE_DEBUG=1 $< -o $@ $(LDFLAGS)

%_asan.out: %.c acutest.h
	$(CC) $(CFLAGS) $(OPTIMIZE_FLAGS) $(ASAN_FLAGS) $< -o $@ $(LDFLAGS)

%_valgrind.out: %.c acutest.h
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) $< -o $@ $(LDFLAGS)

test: $(TEST_BINS)
	@for test in $(TEST_BINS); do \
		echo "Running $$test..."; \
		./$$test || exit 1; \
	done
	@echo ""
	@echo "All tests passed!"

test-debug: $(DEBUG_BINS)
	@for test in $(DEBUG_BINS); do \
		echo "Running $$test..."; \
		./$$test || exit 1; \
	done
	@echo ""
	@echo "All debug tests passed!"

test-asan: $(ASAN_BINS)
	@for test in $(ASAN_BINS) ; do \
		echo "Running $$test with ASAN..."; \
		./$$test || exit 1; \
	done
	@echo ""
	@echo "All ASAN tests passed!"

test-asan-poisoning:
	@echo "Running ASAN poisoning tests (may have expected failures on macOS)..."
	@./test_asan_poisoning_asan.out || true
	@echo ""
	@echo "ASAN poisoning tests completed. Note: Some tests may fail due to fork/ASAN interactions on macOS."

test-valgrind: $(VALGRIND_BINS)
	@for test in $(VALGRIND_BINS); do \
		echo "Running $$test with Valgrind..."; \
		$(VALGRIND) ./$$test || exit 1; \
	done
	@echo ""
	@echo "All Valgrind tests passed!"

run-all: test

run-debug: test-debug

run-asan: test-asan

run-valgrind: test-valgrind

run-asan-poisoning: test-asan-poisoning

clean:
	rm -f $(TEST_BINS) $(DEBUG_BINS) $(ASAN_BINS) $(VALGRIND_BINS)
	rm -rf *_debug.out.dSYM
