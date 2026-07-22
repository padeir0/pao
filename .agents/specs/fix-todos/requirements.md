# Fix All In-Code TODOs — Requirements

**Goal**: Resolve every in-code `TODO`, `UNTESTED` marker, and known bug
cataloged in the source files.

---

## Items

### Bug Fixes

1. **Remove trailing `;` in debug macros** — `lib/debug.h` lines 17, 24, 32, 40.
   All four macros end with `} while (0);` instead of `} while (0)`, breaking
   `if/else` usage. *(known-issues #3)*

2. **Add `static inline` to `i_flAlloc_within`** — `lib/alloc/flAlloc.h`
   line 229. Missing qualifier causes linker errors in multi-TU builds.
   *(known-issues #1)*

3. **Fix `util_absI32(INT32_MIN)` / `util_absI64(INT64_MIN)` UB** —
   `lib/util.h` lines 29–44. Negating the minimum signed value is undefined.
   Cast to unsigned before negation; return types change to `u32` / `u64`.
   *(known-issues #2)*

4. **Fix `%ld` format for `size_t`** — `lib/alloc/flAlloc.h` line 272,
   `lib/alloc/pool.h` line 128. Use `%zu` (C99). *(known-issues #7)*

5. **Add `const` to `buffer_writeString` parameter** — `lib/buffer.h` line 122.
   Change `char* s` → `const char* s`. *(known-issues #5)*

6. **Add `static inline` to `rational_equal`** — `lib/numbers/rational.h`
   line 96. Same class of bug as item 2.

7. **Replace `bzero` with `memset`** — `tests/common.h` line 38. Remove
   `#include <strings.h>`. *(known-issues #8)*

8. **Fix typo `sucesses` → `successes`** — `tests/common.h` line 34.
   *(known-issues #15)*

### Alignment

9. **Align `linearAlloc_alloc` to word boundary** — `lib/alloc/linearAlloc.h`
   line 40. On ARM64, unaligned access is a bus fault. *(known-issues #6)*

### Pool Optimization

10. **Make `pool_available` O(1)** — `lib/alloc/pool.h` line 165. Add a counter
    to the `Pool` struct; keep the O(n) walk as a debug-mode integrity check.

### Buffer Functions

11. **Implement `buffer_copy(in, out)`** — `lib/buffer.h` line 10.
12. **Implement `buffer_copySome(in, out, len)`** — `lib/buffer.h` line 11.
13. **Test `buffer_toHex`** and verify write-offset correctness —
    `lib/buffer.h` line 135. *(known-issues #4)*

### Rational Arithmetic

14. **Implement `rational_simplify`** — `lib/numbers/rational.h` line 115.
    Uses `natural_gcd`.
15. **Implement `rational_neg`** — `lib/numbers/rational.h` line 121.
    Flip sign.
16. **Implement `rational_normalize`** — `lib/numbers/rational.h` line 112.
    Set two rationals to same denominator.
17. **Implement `rational_compare`** — `lib/numbers/rational.h` line 113.
    Cross-multiply.
18. **Implement `rational_add`** — `lib/numbers/rational.h` line 117.
19. **Implement `rational_sub`** — `lib/numbers/rational.h` line 118.
20. **Implement `rational_mul`** — `lib/numbers/rational.h` line 120.
21. **Implement `rational_div`** — `lib/numbers/rational.h` line 119.

### Refactors

22. **Refactor `rational_snprint` to use `buffer.h`** —
    `lib/numbers/rational.h` line 88.

### Natural Optimizations

23. **Optimize `natural_multBase`** — `lib/numbers/natural.h` line 115.
    Shift during copy when allocating.
24. **Optimize `natural_copy`** — `lib/numbers/natural.h` line 205.
    Allocate `len+pad` not `cap`.
25. **Optimize `natural_divDigit`** — `lib/numbers/natural.h` line 824.
    Use pushDigit + reverse.

---

## Out of Scope

- Roadmap features from `plans.md`: stack allocator, decimals, UTF-8 encoder,
  CLI arg parsing, Ovo language.
- Build system (known-issues #9).
- Sanitizer integration (known-issues #10).
- CI (known-issues #17).
- Leading underscore rename in struct tags (known-issues #18).
- `PAO_UNUSED` portability macro (known-issues #16).
- README (known-issues #13).
- `status_toString()` (known-issues #12).
- `status_CHECK` variable name coupling (known-issues #14).
- Buffer serialization functions (`buffer.h:194–221` — author explicitly said
  "will not implement until needed").
- OOM failure-path tests for `natural.h` (known-issues #11 — separate project).

---

## Affected Modules

| Module | Changes |
|---|---|
| `lib/debug.h` | Macro fix |
| `lib/util.h` | Return type fix + UB fix |
| `lib/buffer.h` | `const` fix, new functions, remove UNTESTED |
| `lib/iallocator.h` | Possibly remove/resolve TODO comment |
| `lib/alloc/linearAlloc.h` | Alignment |
| `lib/alloc/flAlloc.h` | `static inline` fix, format string |
| `lib/alloc/pool.h` | Format string, O(1) counter |
| `lib/numbers/natural.h` | 3 optimizations |
| `lib/numbers/rational.h` | `static inline` fix, 8 new functions, snprint refactor |
| `tests/common.h` | `bzero`, typo |
| `tests/buffer_test.c` | New tests for copy, copySome, toHex |
| `tests/allocators/linearAlloc_test.c` | Alignment tests |
| `tests/numbers/rational_test.c` | Tests for all new rational functions |

---

## Dependencies

- Item 3 (util_abs fix) must happen before any tests that exercise `INT32_MIN`.
- Items 14–21 (rational arithmetic) depend on item 6 (`static inline` fix on
  `rational_equal`) being done first.
- Item 14 (`rational_simplify`) must be implemented before items 18–21
  (add/sub/mul/div), since those should simplify their results.
- Item 16 (`rational_normalize`) must be implemented before items 18–19
  (add/sub), since those need common denominators.
- Item 22 (snprint refactor) depends on items 11–12 (buffer_copy) if we want
  to use them.

---

## Risks

- **Item 3**: Changing `util_absI32` return type from `i32` → `u32` is a
  breaking API change. Only known caller is `integer.h` line 50 (debug check:
  `natural_BASE <= util_absI32(num)`) — this stays valid since both sides are
  unsigned-compatible.
- **Items 14–21**: Rational arithmetic is the largest new feature. The
  scratch-parameter pattern (caller provides temporaries) is established by
  `natural_gcd` and `rational_equal`, but the number of scratch parameters
  grows quickly for compound operations.
- **Item 9**: Alignment change in `linearAlloc` subtly changes allocation sizes
  (rounds up), which could break code that depends on exact allocation counts.

---

## Open Questions

1. **`iallocator.h` TODO (line 70)** — "Should allocators provide free+bzero?"
   — This is a design question, not a code bug. Should I implement a
   `secureFree` vtable entry, or just remove/leave the comment?

2. **Natural optimizations (items 23–25)** — These are pure performance
   improvements marked `OPT` with no API change. Should I do these now, or
   defer them?

3. **Rational `snprint` refactor (item 22)** — The current implementation works
   correctly. The TODO asks to rewrite it using `buffer.h`. Worth doing, or is
   the raw pointer math acceptable?
