---
name: known-issues
description: Catalog of known bugs, TODOs, UNTESTED markers, and review recommendations for the Pão codebase. Check this before fixing or refactoring anything.
---

# Known Issues & TODOs

Full review details: [review.md](../../review.md) (2026-07-13).

---

## Critical Bugs — Fix Before Any Release

### 1. Missing `static inline` on `i_flAlloc_within`

**File**: `lib/alloc/flAlloc.h`, line 229.
**Problem**: `i_flAlloc_within` is defined without `static inline`. This causes multiple-definition linker errors if the header is included from more than one translation unit.
**Fix**: Add `static inline` before the function definition.

### 2. `util_absI32(INT32_MIN)` and `util_absI64(INT64_MIN)` are undefined behavior

**File**: `lib/util.h`, lines 29–44.
**Problem**: Negating `INT32_MIN` overflows signed integers on two's complement. Same for `INT64_MIN`.
**Fix**: Cast to unsigned before negation:
```c
static inline
u32 util_absI32(i32 a) {
  if (a < 0) {
    return -(u32)a;
  }
  return (u32)a;
}
```
Note: return type must change to `u32`/`u64` for this to be fully correct, which may require updating callers.

### 3. Trailing `;` after `while(0)` in debug macros

**File**: `lib/debug.h`, lines 12–41.
**Problem**: All four macros (`debug_PRINT`, `debug_PRINTFMT`, `debug_FATAL`, `debug_FATALFMT`) end with `} while (0);` — the trailing `;` makes `if (cond) debug_PRINT("x"); else foo();` parse incorrectly (the `else` binds to an empty statement).
**Fix**: Remove the trailing `;` from all four macro definitions: `} while (0)` instead of `} while (0);`.

---

## High Priority

### 4. `buffer_toHex` write offset bug

**File**: `lib/buffer.h`, lines ~137–166.
**Problem**: If called on a buffer where some bytes have already been read (`start > 0`), the write position will overlap with already-read data. The function correctly uses `i_buffer_absLen` for `snprintf` offset on line 146 (this was fixed after the review noted it), but verify the `writeStart` variable on line 139 also uses the correct base.

### 5. `buffer_writeString` takes `char*` instead of `const char*`

**File**: `lib/buffer.h`, line 122.
**Problem**: The string is only read, never modified. This prevents passing string literals without casting in strict compilers.
**Fix**: Change `char* s` to `const char* s`.

### 6. `linearAlloc_alloc` has no alignment

**File**: `lib/alloc/linearAlloc.h`, line 40.
**Problem**: Acknowledged with a TODO. On ARM64, unaligned access can cause bus faults or degraded performance.
**Fix**: Align allocation offsets to `WORD` (or `_Alignof(max_align_t)`).

### 7. Format string portability: `%ld` for `size_t`

**Files**: `lib/alloc/flAlloc.h` line 272, `lib/alloc/pool.h` line 128.
**Problem**: `%ld` is used for `usize`/`size_t` values. On 32-bit or Windows, `size_t` is not `long`.
**Fix**: Use `%zu` (C99) or cast to `(unsigned long)` with `%lu`.

### 8. `bzero` usage in tests

**File**: `tests/common.h`, line 38.
**Problem**: `bzero` is deprecated POSIX, not ISO C. Also includes `<strings.h>` which is POSIX-only.
**Fix**: Replace `bzero(print_buff, DEFAULT_SIZE)` with `memset(print_buff, 0, DEFAULT_SIZE)` and remove the `<strings.h>` include.

---

## Medium Priority

### 9. No build system

No Makefile, CMake, or meson. Tests use raw `find | gcc` pipelines in shell scripts.

### 10. No sanitizer integration

No `-fsanitize=address,undefined` in test scripts. ASan would catch alignment issues, UBSan would catch the `absI32` UB.

### 11. No tests for allocator failure paths (OOM)

The `i_FailAllocHeap` in `common.h` exists but allocator failure paths are not exercised in the bignum tests. `natural_test.c` itself notes this TODO.

### 12. No `status_toString()` function

When errors propagate, the only diagnostic is an integer code.

### 13. Sparse README

Only says "All my C code goes here" with a link to the style guide. Should list modules, show usage, state build requirements.

---

## Low Priority (Polish)

### 14. `status_CHECK` macro hardcodes variable name `st`

Consider `STATUS_CHECK(var)` to avoid name coupling.

### 15. Typo: `sucesses` → `successes`

**File**: `tests/common.h`, line 34.

### 16. `__attribute__((unused))` is not standard C99

Consider wrapping in a `PAO_UNUSED` portability macro or using `(void)param;`.

### 17. No CI

No GitHub Actions or equivalent. The `testp` script is manual.

### 18. Leading underscore in struct tags

`_fl_node` in `lib/alloc/flAlloc.h` and `_pool_snode` in `lib/alloc/pool.h`. C reserves identifiers starting with `_` followed by uppercase or `__` at file scope. While `_fl_node` is technically safe (lowercase after `_`), it's a risky pattern. Consider `i_pao_fl_node`.

---

## In-Code TODOs (extracted from source)

| Location | TODO |
|---|---|
| `lib/buffer.h:10–11` | `buffer_copy(in, out)` and `buffer_copySome(in, out, len)` |
| `lib/buffer.h:135` | Test `buffer_toHex` |
| `lib/alloc/linearAlloc.h:40` | Align allocations to proper alignment |
| `lib/iallocator.h:70–72` | Should allocators provide free+bzero? |
| `lib/numbers/natural.h:115` | Optimize `multBase` to shift during copy |
| `lib/numbers/natural.h:205` | Optimize `natural_copy` to allocate `len+pad` not `cap` |
| `lib/numbers/rational.h:71` | Test `rational_snprint` |
| `lib/numbers/rational.h:88` | Refactor `rational_snprint` to use `buffer.h` |
| `lib/numbers/rational.h:95` | Test `rational_equal` |
| `lib/numbers/rational.h:112–122` | Implement: normalize, compare, simplify, add, sub, div, mul, neg |
| `docs/plans.md` | Stack allocator, decimals, UTF-8 encoder, CLI arg parsing |

## UNTESTED Markers

| Location | Function |
|---|---|
| `lib/buffer.h:49` | `buffer_printStr` — debug-only, output to stdout |
| `lib/buffer.h:135` | `buffer_toHex` — has a known bug (see #4 above) |
| `lib/numbers/rational.h:71` | `rational_snprint` |
| `lib/numbers/rational.h:95` | `rational_equal` |

## Modules With No Tests At All

- `lib/order.h` — trivial but `order_invert` could have a test
- `lib/util.h` — `util_absI32(INT32_MIN)` is UB (see #2)
- `lib/colors.h` — just constants, testing not needed
