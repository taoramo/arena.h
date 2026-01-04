# Arena.h Unit Tests

Comprehensive unit tests for the arena allocator using the [Acutest](https://github.com/mity/acutest) framework.

## Structure

Each test file focuses on specific functionality:
- `test_lifecycle.c` - Arena creation, release, and configuration
- `test_push.c` - Memory allocation with various alignments and sizes (including 1GB allocation and 16GB address reservation)
- `test_pop.c` - Position management, pop, and clear operations
- `test_temp.c` - Temporary scopes (arena_temp_begin/end)
- `test_string.c` - String helpers (strdup, sprintf)
- `test_memdup.c` - Memory duplication
- `test_realloc.c` - Arena reallocation optimization
- `test_chain.c` - Chaining behavior and flags
- `test_da.c` - Dynamic array macros
- `test_helpers.c` - Helper functions (split, list_filenames)
- `test_stress.c` - Stress tests and integration tests (including 1GB allocation stress test)
- `test_asan_poisoning.c` - ASAN poisoning verification tests (only works with AddressSanitizer)

## Building

```bash
cd tests
make
```

## Running Tests

### Run all tests (optimized)
```bash
make test
# or
make run-all
```

### Run with debug mode (ARENA_ENABLE_DEBUG=1)
```bash
make test-debug
# or
make run-debug
```

### Run with AddressSanitizer (detects memory errors)
```bash
make test-asan
# or
make run-asan
```

**Note:** The `test_asan_poisoning.c` file contains tests specifically for verifying ASAN poisoning behavior. These tests use fork() to create child processes that intentionally access poisoned memory. Run with:

```bash
make test-asan-poisoning
# or
make run-asan-poisoning
```

These tests verify that:
- `arena_pop_to()` properly poisons freed memory
- `arena_clear()` poisons all committed memory
- `arena_temp_end()` poisons temporary allocations
- Buffer overflows/underflows are caught
- Realloc copies poison old pointer
- Various allocation patterns are properly poisoned

**Important:** The ASAN poisoning tests use fork() and may have platform-specific issues. On macOS, some tests may fail due to fork/ASAN interactions. These tests are intentionally accessing poisoned memory and should trigger ASAN errors. Tests that show "OK" successfully detected poisoned memory access.

### Run with Valgrind (detects memory leaks and errors)

### Run with Valgrind (detects memory leaks and errors)
```bash
make test-valgrind
# or
make run-valgrind
```

**Note:** Valgrind requires debug symbols for accurate reporting. The `test-valgrind` target builds tests with `-g -O0` flags automatically. Valgrind must be installed on your system (`brew install valgrind` on macOS).

### Run individual test files
```bash
./test_lifecycle.out
./test_push.out
./test_realloc.out
```

### Run specific tests
```bash
./test_push.out push_normal
./test_realloc.out realloc_in_place_grow
```

### Run tests matching a pattern
```bash
./test_realloc.out realloc
./test_da.out da_growth
```

### Run all except a pattern
```bash
./test_push.out --exclude alignment
```

### Run individual test with Valgrind
```bash
valgrind --leak-check=full --show-leak-kinds=all ./test_lifecycle_valgrind.out
```

### List all tests
```bash
./test_lifecycle.out --list
```

### Verbose output
```bash
./test_push.out --verbose
```

## Makefile Targets

- `make` or `make all` - Build all test binaries
- `make test` - Run all tests
- `make test-debug` - Run tests with debug logging enabled
- `make test-asan` - Run tests with AddressSanitizer (excludes ASAN poisoning tests)
- `make test-valgrind` - Run tests with Valgrind
- `make test-asan-poisoning` - Run ASAN poisoning verification tests (fork-based negative tests)
- `make clean` - Remove all build artifacts
- `make run-all` - Alias for `make test`
- `make run-debug` - Alias for `make test-debug`
- `make run-asan` - Alias for `make test-asan`
- `make run-valgrind` - Alias for `make test-valgrind`
- `make run-asan-poisoning` - Alias for `make test-asan-poisoning`

## Test Coverage

### Lifecycle Tests
- Basic arena creation with various sizes
- Page alignment verification
- Release single and chained arenas
- Flag handling (ARENA_FLAG_NO_CHAIN)
- Zero initial commit edge case

### Allocation Tests
- Normal allocations with zero-fill
- Various alignment requirements (1-256 bytes)
- Large alignment (> allocation size)
- Zero-byte allocations
- Multi-page allocations
- Alignment preservation
- Struct alignment

### Position Management
- Basic pop operations
- Multiple position checkpoints
- Pop to header
- Clear functionality
- Position tracking with alignment
- Multiple clear cycles

### Temporary Scopes
- Basic temp scope usage
- Nested temp scopes
- Large allocations in temp scopes
- Multiple temp scope reuse
- Data preservation outside temp scopes

### String Tests
- String duplication (normal, empty, large, special chars)
- Formatted strings (basic, multiple args, pointers, width/precision)
- Multiple sprintf calls
- va_list variant

### Memory Duplication
- Basic memory copy
- Zero-size copy
- Large memory blocks
- Various data types
- Alignment preservation
- Original data preservation
- Multiple copies
- Data with null bytes

### Reallocation Tests
- NULL handling (like malloc)
- Zero old size
- In-place growth optimization
- Commit boundary crossing
- Shrinking
- Copy when not last allocation
- Copy when boxed in
- Same size no-op
- Large growth
- Sequential realloc pattern
- Multiple reallocs

### Chaining Tests
- Chain creation on overflow
- Multiple chained blocks
- Allocations across chains
- Pop with chains
- No-chain flag behavior
- Position tracking across chains
- Clear with chains
- Multiple independent arenas

### Dynamic Array Tests
- Single item append
- Batch append
- Growth from zero capacity
- Multiple growth cycles
- Various element types (int, char, double, struct, pointer)
- Alignment verification
- Large arrays

### Helper Tests
- String splitting (various delimiters, edge cases, whitespace)
- Directory listing (valid dir, nonexistent, empty, filtering)

### Stress Tests
- Many small allocations (100,000)
- Large allocations (5MB)
- Mixed allocation sizes
- Many temp scopes (1,000)
- Many reallocs (1,000)
- Large dynamic arrays (100,000 elements)
- Push/pop patterns
- Deep nested temp scopes
- Alignment stress
- Zero-fill stress
- String concatenation
- Complex workflow integration

## Breaking Scenarios Targeted

These tests specifically try to break the arena allocator:

1. **Alignment Issues** - Tests various alignment requirements including large alignments exceeding allocation size
2. **Off-by-one Errors** - Position tracking with precise checkpoints
3. **Overflow** - Large allocations pushing boundaries
4. **Chaining Bugs** - Multiple chained blocks with cross-chain operations
5. **Realloc Optimization** - Tests all realloc paths (in-place vs copy)
6. **Zero-size Edge Cases** - Zero-byte allocations and memdup
7. **NULL Handling** - All functions tested with NULL inputs where applicable
8. **Memory Poisoning** - ASan builds to detect use-after-free and invalid access
9. **Commit Boundary** - Allocations crossing commit granularity boundaries
10. **Nested Temp Scopes** - Deep nesting to test temp scope restoration

## CI/CD Integration

For continuous integration:

```bash
# Quick test run
make test

# Full test with ASan
make test-asan

# ASAN poisoning verification (negative tests)
make test-asan-poisoning

# Full test with Valgrind
make test-valgrind

# With debug logging
make test-debug
```

Exit codes:
- 0: All tests pass
- 1: Some test failed
- Other: Internal error
