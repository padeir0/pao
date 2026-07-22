# Replace `natural_div` with Algorithm D — Tasks

---

- [x] **Task 1**: Add `i_natural_rawMultDigit` helper
  - Files: `lib/numbers/natural.h`
  - Do: Add `static inline u32 i_natural_rawMultDigit(const u32* src, u32 len, u32 d, u32* dst)` — multiplies a raw `u32[]` array by a single digit `d`, writing the result to `dst[]`. Returns the final carry. Single-pass, no allocation.
  - Test: No dedicated test — this is an internal helper tested through `natural_div`.
  - Verify: `gcc -Wall -Wextra -Werror -pedantic -Wconversion -Wstrict-prototypes -std=c99 -O2 tests/numbers/natural/natural_test.c -o /tmp/natural_test && /tmp/natural_test`

- [x] **Task 2**: Add `i_natural_rawDivDigit` helper
  - Files: `lib/numbers/natural.h`
  - Do: Add `static inline void i_natural_rawDivDigit(u32* digits, u32 len, u32 d)` — divides a raw `u32[]` array by a single digit `d` in-place (MSD-to-LSD pass). Used for unnormalization (D8).
  - Test: No dedicated test — internal helper tested through `natural_div`.
  - Verify: same as Task 1.

- [x] **Task 3**: Add `i_natural_mulSub` helper
  - Files: `lib/numbers/natural.h`
  - Do: Add `static inline u32 i_natural_mulSub(u32* u, u32 uOff, const u32* v, u32 n, u32 qhat)` — fused multiply-subtract: `u[uOff..uOff+n] -= qhat × v[0..n-1]`. Returns 1 if the result went negative (borrow out), 0 otherwise. This is Algorithm D step D4.
  - Test: No dedicated test — internal helper tested through `natural_div`.
  - Verify: same as Task 1.

- [x] **Task 4**: Add `i_natural_addBack` helper
  - Files: `lib/numbers/natural.h`
  - Do: Add `static inline void i_natural_addBack(u32* u, u32 uOff, const u32* v, u32 n)` — adds `v[0..n-1]` back to `u[uOff..uOff+n]`, ignoring top carry. This is Algorithm D step D6 (add-back correction).
  - Test: No dedicated test — internal helper tested through `natural_div`.
  - Verify: same as Task 1.

- [x] **Task 5**: Rewrite `natural_div` with Algorithm D
  - Files: `lib/numbers/natural.h`
  - Do: Replace the body of `natural_div` (lines ~757–824) with the Algorithm D implementation:
    1. Keep existing debug checks and early returns (B=0, A=0).
    2. Add single-digit fast path: if `B->len == 1`, delegate to `natural_divDigit`.
    3. Add `A < B` fast path: Q=0, R=copy(A).
    4. D1: Compute `d`, allocate raw `u[]` and `v[]` via `IAllocator`, normalize.
    5. D2–D7: Main loop producing quotient digits into raw `q[]` array.
    6. D8: Unnormalize remainder.
    7. Copy `q[]` into `Q` and `u[0..n-1]` into `R`, trimming leading zeros.
    8. Free raw arrays.
  - Test: All 8 existing `natural_div` tests must still pass.
  - Verify: same as Task 1.

- [x] **Task 6**: Remove old binary-search helpers
  - Files: `lib/numbers/natural.h`
  - Do: Delete `i_natural_testGuess` (lines 708–730) and `i_natural_DIVMAXNUMGUESSES` (line 732). Grep confirms no other callers.
  - Test: Full test suite passes.
  - Verify: same as Task 1.

- [x] **Task 7**: Add `test_natural_div_addback`
  - Files: `tests/numbers/natural/natural_test.c`
  - Do: Construct inputs that force the D6 add-back step. Use `A = B × (BASE-1) + (B - 1)` with a multi-digit `B` so the estimation overshoots. Verify `Q*B + R == A` and `R < B`.
  - Test: New test passes.
  - Verify: same as Task 1.

- [x] **Task 8**: Add `test_natural_div_single_digit_divisor`
  - Files: `tests/numbers/natural/natural_test.c`
  - Do: A has many digits (≥3), B has 1 digit. Verify result matches `natural_divDigit`. Exercises the new fast path.
  - Test: New test passes.
  - Verify: same as Task 1.

- [x] **Task 9**: Add `test_natural_div_equal_length`
  - Files: `tests/numbers/natural/natural_test.c`
  - Do: A and B have the same number of digits (`m == 0`). Verify division theorem holds.
  - Test: New test passes.
  - Verify: same as Task 1.

- [x] **Task 10**: Add `test_natural_div_large_quotient`
  - Files: `tests/numbers/natural/natural_test.c`
  - Do: A is much larger than B (e.g., A has 6 digits, B has 2), producing a many-digit quotient. Verify via `Q*B + R == A`.
  - Test: New test passes.
  - Verify: same as Task 1.

- [x] **Task 11**: Add `test_natural_div_max_digits`
  - Files: `tests/numbers/natural/natural_test.c`
  - Do: All digits are `BASE-1 = 999999999`. Stresses carry propagation in normalization, multiply-subtract, and unnormalization.
  - Test: New test passes.
  - Verify: same as Task 1.

- [x] **Task 12**: Add `test_natural_div_theorem_multidigit`
  - Files: `tests/numbers/natural/natural_test.c`
  - Do: Property test over several multi-digit (A, B) pairs — verify `A == Q*B + R` and `R < B` for each. Use hand-picked values spanning 2–5 digit operands.
  - Test: New test passes.
  - Verify: same as Task 1.

- [x] **Task 13**: Full verification
  - Files: all
  - Do: Run `cd tests && bash test` and `cd tests && bash testp`. Self-review all changes against the AGENTS.md checklist.
  - Test: All tests pass across gcc/clang × c99/c17 × -O0/-O2.
  - Verify: `cd tests && bash testp`
