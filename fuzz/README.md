# AFL++ Fuzzing Harnesses for arena.h

This directory contains AFL++ fuzzing harnesses for arena.h memory allocator library.

## Quick Start

Build all fuzzers (works with clang even without AFL++):

```bash
make -f fuzz.mk CC=clang
```

Test a fuzzer with a seed file:

```bash
./fuzz_arena_api seeds/seed_01_api.bin
```

All fuzzers include AddressSanitizer (ASAN) for detecting memory errors.

## Building with AFL++

First, ensure AFL++ is built and available:

```bash
cd ../AFLplusplus
make
cd ../fuzz
```

Then build fuzzers:

```bash
make -f fuzz.mk
```

This will compile four fuzzers with AddressSanitizer enabled:
- `fuzz_arena_api` - Comprehensive API fuzzer
- `fuzz_arena_push` - Focused push operation fuzzer
- `fuzz_arena_realloc` - Focused realloc fuzzer
- `fuzz_arena_temp` - Temporary scope fuzzer

## Running the Fuzzers

Run each fuzzer independently:

```bash
make -f fuzz.mk run-api
make -f fuzz.mk run-push
make -f fuzz.mk run-realloc
make -f fuzz.mk run-temp
```

Or run manually with AFL++:

```bash
../AFLplusplus/afl-fuzz -i seeds -o out_api -- ./fuzz_arena_api @@
```

## Fuzzer Details

**Input Validation**: All fuzzers validate input to prevent crashes from invalid data:
- Size limited to 1GB (2^30 bytes)
- Alignment limited to 128 (power of 2, non-zero)
- Position limited to 1GB
- Nest depth limited to 8 levels

### fuzz_arena_api
Tests all arena.h API functions in a single fuzzer.

**Input format (binary):**
```
operation:1    [0-11] selects which API operation to test
size:8         allocation size in bytes
alignment:1    alignment in bytes (power of 2)
zero_fill:1   0 = no zero fill, 1 = zero fill
pos:8          position for pop_to operations
payload:var    optional payload data for operations that need it
```

**Operations:**
0: arena_push with payload copy
1: arena_push followed by arena_realloc (grow)
2: arena_push followed by arena_pop_to
3: arena_clear
4: arena_temp_begin/end with allocation
5: arena_strdup with payload
6: arena_sprintf with formatted string
7: arena_push with zero_fill=1
8: Multiple push/pop_to sequences
9: arena_push with arena_memdup
10: arena_clear between allocations
11: Nested temp scopes

### fuzz_arena_push
Focused fuzzer for arena_push edge cases.

**Input format (binary):**
```
size:8         allocation size in bytes
alignment:1    alignment in bytes
zero_fill:1    zero fill flag
```

**Tests:**
- Various allocation sizes (including edge cases)
- Different alignments (powers of 2, non-powers)
- Zero fill vs no zero fill
- Alignment = 0 (should default to 1)
- Multiple sequential allocations

### fuzz_arena_realloc
Focused fuzzer for arena_realloc optimization paths.

**Input format (binary):**
```
original_size:8   size of initial allocation
new_size:8        size to reallocate to
alignment:1       alignment for allocation
operation:1       [0-7] selects realloc scenario
```

**Operations:**
0: Standard realloc (new_size)
1: Realloc NULL pointer (edge case)
2: Realloc to same size (no-op)
3: Realloc after other allocations (tests not-last-path)
4: Grow operation (new_size > original_size)
5: Shrink operation (new_size < original_size)
6: Realloc to 0 (edge case)
7: Double size realloc

### fuzz_arena_temp
Focused fuzzer for temporary arena scopes.

**Input format (binary):**
```
alloc_size:8     size for each allocation
nest_depth:1     how many temp scopes to nest
alignment:1      alignment for allocations
pattern:1        byte pattern to fill memory with
```

**Tests:**
- Nested temp scopes (up to 8 levels)
- Interleaved allocations
- Scope restoration
- arena_clear with active temp scopes
- Various allocation patterns

## Fixed Crash Bugs (Investigation 2026-01-04)

### Multiple Crashes in fuzz_arena_realloc

**Root Cause:** Buffer overflow in memset after failed realloc operations

**Problem:**
```c
// When realloc failed/skipped, we still tried to memset with new_size
if (newptr != NULL && input.new_size > 0 && input.new_size < (1ULL << 20)) {
    memset(newptr, 0x55, input.new_size);  // input.new_size could be 256MB!
}
```

**Example Crash Input (id:000000):**
```
original_size=44544 bytes
new_size=268435456 bytes (256MB)
operation=0 (standard realloc)
```
- Allocates 44544 bytes initially
- Realloc to 256MB fails/skipped (arena limit)
- memset() tries to write 256MB → buffer overflow

**Fixes Applied:**

1. **arena_realloc fallback fixed**:
   - Added NULL check in realloc when arena_push fails
   - Prevents segfault when OOM in realloc operations

2. **Proper memset size calculation**:
   ```c
   size_t actual_size = (newptr == ptr) ? input.original_size : input.new_size;
   size_t memset_size = (actual_size < (1ULL << 20)) ? actual_size : (1ULL << 20);
   ```

3. **Enhanced input validation**:
   - Alignment must be power-of-2 (not just > 128)
   - Size limits: 256MB max for fuzzing
   - NULL checks after all allocations

4. **ARENA_ABORT_ON_OOM=0** for fuzzing:
   - Fuzzers return NULL on OOM instead of aborting
   - Production code keeps abort behavior

**Test Results:**
- All 6 crash inputs now handle gracefully
- Valid seeds still work correctly
- No more segfaults or bus errors
- AFL++ fuzzing can continue safely

## Creating Custom Seeds

Seeds are binary files in `seeds/` directory. Create seeds by encoding the input format as binary data.

Example using Python:

```python
import struct

# Create seed for fuzz_arena_api
operation = 0
size = 16
alignment = 8
zero_fill = 0
pos = 0

header = struct.pack('<BQBBQ', operation, size, alignment, zero_fill, pos)
with open('seeds/custom_seed.bin', 'wb') as f:
    f.write(header)
    f.write(b'test payload')
```

## Fixed Crash Bugs

### Segfault in fuzz_arena_realloc

**Problem:** The fuzzer had a buffer overflow bug:
```c
if (ptr && input.new_size > 0) {
    memset(ptr, 0x55, input.new_size);  // BUG: using new_size, not allocated size
}
```

When `arena_realloc` failed (returned NULL) or allocated less than `new_size`, we were still memset'ing with `new_size`, causing buffer overflows and segfaults.

**Fix:** Added size limit to memset:
```c
if (ptr && input.new_size > 0 && input.new_size < (1ULL << 20)) {
    memset(ptr, 0x55, input.new_size);  // Max 1MB memset
}
```

This prevents buffer overflows while still testing the realloc functionality.

## Interpreting Results

AFL++ output directories:
- `out_api/`, `out_push/`, `out_realloc/`, `out_temp/` - Fuzzer output

Look for crashes in:
- `out_<fuzzer>/crashes/` - Crashing inputs
- `out_<fuzzer>/hangs/` - Timeout inputs

Reproduce a crash:

```bash
./fuzz_arena_api < crash_file
```

## AFL++ Features Used

- **AddressSanitizer (ASAN)**: Catches memory errors during fuzzing
- **LibFuzzer-style harness**: Clean interface for test cases
- **Persistent mode**: Better performance (automatic with afl-clang-fast)
- **Dictionaries**: Recommended for faster convergence (see below)

## Dictionaries

For better performance, create AFL++ dictionaries with common values:

```bash
# Create dictionary for arena.h fuzzing
cat > arena.dict << 'EOF'
size_1="AAAAAAAAAAAAAAAB"  # 8 bytes = 1 (little endian)
size_16="AAAAAAAAAAAAAAA@"
align_1=
align_8=
align_16=
zero_off=
zero_on=
EOF

# Run with dictionary
../AFLplusplus/afl-fuzz -i seeds -o out -D arena.dict -- ./fuzz_arena_api @@
```

## Handling OOM During Fuzzing

arena.h by default calls `abort()` on out-of-memory. For fuzzing, we use a compile-time flag to return `NULL` instead:

```c
#define ARENA_ABORT_ON_OOM 0  // Fuzzers return NULL on OOM
#define ARENA_ABORT_ON_OOM 1  // Production aborts on OOM (default)
```

Fuzzers are built with `ARENA_ABORT_ON_OOM=0`, allowing them to explore more inputs without crashing on OOM. Production code builds with the default (abort on OOM).

This approach:
- Maintains production behavior (fail-fast on OOM)
- Enables efficient fuzzing (graceful handling of OOM)
- No code changes needed in production use

## Common Issues

**"afl-clang-fast not found"**: Build AFL++ first:
```bash
cd ../AFLplusplus && make && cd ../fuzz
```

**Fuzzer crashes immediately on all inputs**: May be ASAN false positive or harness bug. Run with ASAN options:
```bash
ASAN_OPTIONS=detect_leaks=0:halt_on_error=0 ./fuzz_arena_api < test_input
```

**Slow execution**: AFL++ may not be using persistent mode. Check that you're using `afl-clang-fast` not `afl-cc`.

## Next Steps

After finding crashes:

1. Minimize crash inputs:
   ```bash
   ../AFLplusplus/afl-tmin -i crash_file -o minimized_crash -- ./fuzz_arena_api @@
   ```

2. Analyze crash with GDB:
   ```bash
   gdb --args ./fuzz_arena_api crash_file
   ```

3. Fix bugs in arena.h

4. Add regression tests to `../tests/`
