# Fix All In-Code TODOs — Tasks

Ordered checklist of atomic work items. Dependencies flow top-to-bottom.

---

## Group A — Bug Fixes (no dependencies, no new tests)

- [x] **Task 1**: Fix debug macro trailing semicolons
  - Files: `lib/debug.h`
  - Do: Change `} while (0);` → `} while (0)` in all four macros
    (`debug_PRINT`, `debug_PRINTFMT`, `debug_FATAL`, `debug_FATALFMT`).
  - Test: None — compile under `-Wall -Wextra -Werror -pedantic`.
  - Verify: `cd tests && bash test`

- [x] **Task 2**: Add `static inline` to `i_flAlloc_within`
  - Files: `lib/alloc/flAlloc.h`
  - Do: Prepend `static inline` to function definition at line 229.
  - Test: None — existing `flAlloc_test.c`.
  - Verify: `cd tests && bash test`

- [x] **Task 3**: Fix `util_absI32` / `util_absI64` UB
  - Files: `lib/util.h`
  - Do: Change return types to `u32`/`u64`. Cast to unsigned before negation.
    Grep callers and verify compatibility.
  - Test: None — existing `integer_test.c` exercises the caller.
  - Verify: `cd tests && bash test`

- [x] **Task 4**: Fix `%ld` format strings for `size_t`
  - Files: `lib/alloc/flAlloc.h`, `lib/alloc/pool.h`
  - Do: Replace `%ld` with `%zu` wherever the argument is `usize`/`size_t`.
  - Test: None — `-Wformat` catches mismatches.
  - Verify: `cd tests && bash test`

- [x] **Task 5**: Add `const` to `buffer_writeString`
  - Files: `lib/buffer.h`
  - Do: Change `char* s` → `const char* s` at line 122.
  - Test: None — existing `buffer_test.c`.
  - Verify: `cd tests && bash test`

- [x] **Task 6**: Add `static inline` to `rational_equal`
  - Files: `lib/numbers/rational.h`
  - Do: Prepend `static inline` at line 96.
  - Test: None — existing `rational_test.c`.
  - Verify: `cd tests && bash test`

- [x] **Task 7**: Replace `bzero` with `memset`, fix typo
  - Files: `tests/common.h`
  - Do: Replace `bzero(print_buff, DEFAULT_SIZE)` with
    `memset(print_buff, 0, DEFAULT_SIZE)`. Remove `#include <strings.h>`.
    Rename `sucesses` → `successes`.
  - Test: None — all existing tests use `run_tests()`.
  - Verify: `cd tests && bash test`

- [x] **Task 8**: Remove stale `UNTESTED` from `buffer_toHex`
  - Files: `lib/buffer.h`
  - Do: Remove `UNTESTED: (TODO:)` from the comment at line 135.
    Tests already exist in `buffer_test.c`.
  - Test: None.
  - Verify: Visual inspection.

- [x] **Task 9**: Remove `iallocator.h` TODO comment
  - Files: `lib/iallocator.h`
  - Do: Remove the TODO comment block at lines 69–72.
  - Test: None.
  - Verify: Visual inspection.

---

## Group B — Alignment and Pool (independent of each other)

- [x] **Task 10**: Align `linearAlloc_alloc` to `WORD`
  - Files: `lib/alloc/linearAlloc.h`, `tests/allocators/linearAlloc_test.c`
  - Do: Round up `size` to next multiple of `WORD` using
    `(size + WORD - 1) & ~(WORD - 1)`. Add `SAFE` footnote about buffer
    start alignment.
  - Test: Add `test_alignment_basic`, `test_alignment_used`,
    `test_alignment_exact`. Update `test_usedAndAvailable` to account for
    aligned sizes.
  - Verify: `gcc -Wall -Wextra -Werror -pedantic -Wconversion -Wstrict-prototypes -std=c99 -O2 tests/allocators/linearAlloc_test.c -o /tmp/test && /tmp/test`

- [x] **Task 11**: Make `pool_available` O(1)
  - Files: `lib/alloc/pool.h`, `tests/allocators/pool/pool_test.c`
  - Do: Add `usize freeCount` to `Pool` struct. Initialize in
    `i_pool_setList`. Decrement in `pool_alloc`, increment in `pool_free`.
    Return `freeCount * chunkSize` in `pool_available`. Add debug-mode
    assertion that O(n) walk agrees with counter.
  - Test: Add `test_pool_available_afterAllocs`,
    `test_pool_available_afterFree`, `test_pool_available_afterFreeAll`.
  - Verify: `gcc -Wall -Wextra -Werror -pedantic -Wconversion -Wstrict-prototypes -std=c99 -O2 tests/allocators/pool/pool_test.c -o /tmp/test && /tmp/test`

---

## Group C — Buffer Functions

- [x] **Task 12**: Implement `buffer_copy` and `buffer_copySome`
  - Files: `lib/buffer.h`, `tests/buffer_test.c`
  - Do: Implement both functions. `buffer_copySome` is the core;
    `buffer_copy` calls it with `len = in->len`. Does not consume input
    bytes. Remove the TODO comments at lines 10–11.
  - Test: Add `test_buffer_copy_full`, `test_buffer_copy_outSmaller`,
    `test_buffer_copy_empty`, `test_buffer_copySome_exact`,
    `test_buffer_copySome_overRequest`, `test_buffer_copySome_zero`,
    `test_buffer_copy_afterRead`.
  - Verify: `gcc -Wall -Wextra -Werror -pedantic -Wconversion -Wstrict-prototypes -std=c99 -O2 tests/buffer_test.c -o /tmp/test && /tmp/test`

---

## Group D — Rational Arithmetic (ordered by dependency)

- [x] **Task 13**: Implement `rational_neg`
  - Files: `lib/numbers/rational.h`, `tests/numbers/rational_test.c`
  - Do: Implement `rational_neg(Rational* rat)`. Flip sign; guard zero.
    No allocator needed.
  - Test: Add `test_rational_neg_positive`, `test_rational_neg_negative`,
    `test_rational_neg_zero`, `test_rational_neg_doubleNeg`.
  - Verify: `gcc -Wall -Wextra -Werror -pedantic -Wconversion -Wstrict-prototypes -std=c99 -O2 tests/numbers/rational_test.c -o /tmp/test && /tmp/test`

- [x] **Task 14**: Implement `rational_simplify`
  - Files: `lib/numbers/rational.h`, `tests/numbers/rational_test.c`
  - Do: Implement using `natural_gcd` + `natural_div`. Takes 5 scratch
    Naturals. Zero numerator → denominator becomes 1.
  - Test: Add `test_rational_simplify_basic`,
    `test_rational_simplify_alreadySimplified`,
    `test_rational_simplify_zeroNumerator`,
    `test_rational_simplify_one`,
    `test_rational_simplify_negative`,
    `test_rational_simplify_large`.
  - Verify: `gcc -Wall -Wextra -Werror -pedantic -Wconversion -Wstrict-prototypes -std=c99 -O2 tests/numbers/rational_test.c -o /tmp/test && /tmp/test`

- [x] **Task 15**: Implement `rational_normalize`
  - Files: `lib/numbers/rational.h`, `tests/numbers/rational_test.c`
  - Do: Compute LCM via `gcd`, scale both numerators and denominators.
    Modifies A and B in-place.
  - Test: Add `test_rational_normalize_sameDenom`,
    `test_rational_normalize_different`,
    `test_rational_normalize_oneMultiple`.
  - Verify: `gcc -Wall -Wextra -Werror -pedantic -Wconversion -Wstrict-prototypes -std=c99 -O2 tests/numbers/rational_test.c -o /tmp/test && /tmp/test`

- [x] **Task 16**: Implement `rational_compare`
  - Files: `lib/numbers/rational.h`, `tests/numbers/rational_test.c`
  - Do: Cross-multiply with sign logic. Takes 2 scratch Naturals.
  - Test: Add `test_rational_compare_equal`,
    `test_rational_compare_less`, `test_rational_compare_greater`,
    `test_rational_compare_negativeVsPositive`,
    `test_rational_compare_bothNegative`,
    `test_rational_compare_zeros`.
  - Verify: `gcc -Wall -Wextra -Werror -pedantic -Wconversion -Wstrict-prototypes -std=c99 -O2 tests/numbers/rational_test.c -o /tmp/test && /tmp/test`

- [x] **Task 17**: Implement `rational_mul`
  - Files: `lib/numbers/rational.h`, `tests/numbers/rational_test.c`
  - Do: `(a*c)/(b*d)`, set sign, simplify. Takes 5 scratch Naturals
    (for simplify).
  - Test: Add `test_rational_mul_basic`, `test_rational_mul_byZero`,
    `test_rational_mul_byOne`, `test_rational_mul_negatives`,
    `test_rational_mul_mixedSign`.
  - Verify: `gcc -Wall -Wextra -Werror -pedantic -Wconversion -Wstrict-prototypes -std=c99 -O2 tests/numbers/rational_test.c -o /tmp/test && /tmp/test`

- [x] **Task 18**: Implement `rational_div`
  - Files: `lib/numbers/rational.h`, `tests/numbers/rational_test.c`
  - Do: `(a*d)/(b*c)`, check B.numerator != 0, set sign, simplify.
  - Test: Add `test_rational_div_basic`, `test_rational_div_byOne`,
    `test_rational_div_byItself`, `test_rational_div_byZero`,
    `test_rational_div_negatives`.
  - Verify: `gcc -Wall -Wextra -Werror -pedantic -Wconversion -Wstrict-prototypes -std=c99 -O2 tests/numbers/rational_test.c -o /tmp/test && /tmp/test`

- [x] **Task 19**: Implement `rational_add`
  - Files: `lib/numbers/rational.h`, `tests/numbers/rational_test.c`
  - Do: Cross-multiply approach with sign logic (mirror `integer_add`
    pattern). Simplify result.
  - Test: Add `test_rational_add_sameDenom`,
    `test_rational_add_differentDenom`, `test_rational_add_negatives`,
    `test_rational_add_cancel`, `test_rational_add_zero`,
    `test_rational_add_mixedSign`.
  - Verify: `gcc -Wall -Wextra -Werror -pedantic -Wconversion -Wstrict-prototypes -std=c99 -O2 tests/numbers/rational_test.c -o /tmp/test && /tmp/test`

- [x] **Task 20**: Implement `rational_sub`
  - Files: `lib/numbers/rational.h`, `tests/numbers/rational_test.c`
  - Do: Same as add but with inverted sign logic (mirror `integer_sub`
    pattern). Simplify result.
  - Test: Add `test_rational_sub_sameDenom`,
    `test_rational_sub_resultZero`, `test_rational_sub_negative`,
    `test_rational_sub_mixedSign`.
  - Verify: `gcc -Wall -Wextra -Werror -pedantic -Wconversion -Wstrict-prototypes -std=c99 -O2 tests/numbers/rational_test.c -o /tmp/test && /tmp/test`

---

## Group E — Refactor

- [x] **Task 21**: Refactor `rational_snprint` to use `buffer.h`
  - Files: `lib/numbers/rational.h`
  - Do: Rewrite using `Buffer` for sign and `/` writes. Use
    `natural_snprint` directly into the buffer's backing array. Remove
    the TODO comment at line 88. Remove the `UNTESTED` marker at line 71.
  - Test: Existing `rational_test.c` tests cover `rational_snprint`.
  - Verify: `gcc -Wall -Wextra -Werror -pedantic -Wconversion -Wstrict-prototypes -std=c99 -O2 tests/numbers/rational_test.c -o /tmp/test && /tmp/test`

- [x] **Task 22**: Remove TODO comment block from `rational.h`
  - Files: `lib/numbers/rational.h`
  - Do: Delete the TODO comment block at lines 111–122 (all items
    implemented). Update `plans.md` to check off `Rationals`.
  - Test: None.
  - Verify: Visual inspection.

---

## Group F — Natural Optimizations (optional, implement last)

- [x] **Task 23**: Optimize `natural_multBase` — shift during copy
  - Files: `lib/numbers/natural.h`
  - Do: When `i_natural_pushDigit` would allocate, copy digits shifted
    right by one into the new array directly. Remove the TODO comment.
  - Test: Existing `natural_test.c`.
  - Verify: `cd tests && bash test`

- [x] **Task 24**: Optimize `natural_copy` — allocate `len+pad`
  - Files: `lib/numbers/natural.h`
  - Do: Replace `A->cap` with next-power-of-two of `A->len` (minimum
    `natural_MINNATVEC`). Remove the TODO comment.
  - Test: Existing `natural_test.c`.
  - Verify: `cd tests && bash test`

- [x] **Task 25**: Optimize `natural_divDigit` — pushDigit + reverse
  - Files: `lib/numbers/natural.h`
  - Do: Build quotient in reverse using `pushDigit`, then reverse the
    digit array. Add `i_natural_reverseDigits` helper. Remove the TODO
    comment.
  - Test: Existing `natural_test.c`.
  - Verify: `cd tests && bash test`

---

## Final Verification

- [x] **Task 26**: Run full portable test matrix
  - Do: `cd tests && bash testp`
  - Verify: All compilers × standards × optimization levels pass.
