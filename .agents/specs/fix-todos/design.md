# Fix All In-Code TODOs — Design

For each requirement item, this document defines the approach, invariants,
correctness criteria, test strategy, and edge cases.

---

## Items 1–8: Bug Fixes

These are mechanical, one-to-few-line changes. Grouped together because
they share the same design pattern: fix, compile, verify existing tests pass.

---

### Item 1 — Remove trailing `;` in debug macros

**Approach**: In all four macros in `lib/debug.h`, change `} while (0);` to
`} while (0)`.

**Invariants**: Macros must be usable in `if/else` without dangling-else
issues.

**Correctness criteria**: `if (cond) debug_PRINT("x"); else foo();` compiles
and behaves correctly.

**Test strategy**: Existing tests exercise these macros indirectly through
every debug check in the library. No new tests needed — compilation under
`-Wall -Wextra -Werror -pedantic` is the test.

**Edge cases**: None.

---

### Item 2 — Add `static inline` to `i_flAlloc_within`

**Approach**: Prepend `static inline` to the function definition at
`lib/alloc/flAlloc.h` line 229.

**Invariants**: Function must remain callable from `flAlloc_free`.

**Correctness criteria**: Multi-TU compilation no longer causes linker errors.

**Test strategy**: Existing `flAlloc_test.c` covers this function indirectly
through `flAlloc_free`. No new tests needed.

**Edge cases**: None.

---

### Item 3 — Fix `util_absI32` / `util_absI64` UB

**Approach**:
- Change `util_absI32` return type from `i32` to `u32`.
- Change `util_absI64` return type from `i64` to `u64`.
- Cast to unsigned before negation: `return -(u32)a;`

**Invariants**:
- `util_absI32(x) >= 0` (trivially true: unsigned return).
- `util_absI32(INT32_MIN)` == `2147483648u` (not UB).
- `util_absI32(0)` == `0`.

**Correctness criteria**:
- The cast `-(u32)a` is well-defined: C99 §6.2.5/9 guarantees unsigned
  arithmetic wraps modulo 2^N.
- Only known caller: `integer.h` line 50 debug check
  `natural_BASE <= util_absI32(num)`. Both sides become unsigned, comparison
  remains valid.

**Test strategy**: No new test file — the change is purely about removing UB.
The existing `integer_test.c` exercises the caller path. If desired, a
`util_test.c` can be added later (out of scope per requirements).

**Edge cases**: `INT32_MIN`, `INT64_MIN`, `0`, `1`, `-1`.

---

### Item 4 — Fix `%ld` format for `size_t`

**Approach**: In `lib/alloc/flAlloc.h` and `lib/alloc/pool.h`, replace `%ld`
with `%zu` wherever the argument is `usize`/`size_t`.

**Invariants**: Format string matches argument type.

**Correctness criteria**: `-Wformat` (included in `-Wall`) will catch
mismatches.

**Test strategy**: Existing tests + compiler warnings. No new tests.

**Edge cases**: None.

---

### Item 5 — Add `const` to `buffer_writeString`

**Approach**: Change parameter from `char* s` to `const char* s` at
`lib/buffer.h` line 122.

**Invariants**: The function only reads `s`, never modifies it.

**Correctness criteria**: Passing a string literal compiles without warnings.

**Test strategy**: Existing `buffer_test.c` covers this function.

**Edge cases**: None.

---

### Item 6 — Add `static inline` to `rational_equal`

**Approach**: Prepend `static inline` to the function definition at
`lib/numbers/rational.h` line 96.

**Invariants**: Function must remain callable. Same fix as item 2.

**Correctness criteria**: Multi-TU compilation succeeds.

**Test strategy**: Existing `rational_test.c` already tests `rational_equal`.

**Edge cases**: None.

---

### Item 7 — Replace `bzero` with `memset`

**Approach**: In `tests/common.h`:
- Replace `bzero(print_buff, DEFAULT_SIZE)` with
  `memset(print_buff, 0, DEFAULT_SIZE)`.
- Remove `#include <strings.h>`.

**Invariants**: Buffer is zeroed before each test.

**Correctness criteria**: `memset(p, 0, n)` is equivalent to `bzero(p, n)`.

**Test strategy**: All existing tests use `run_tests()` which calls this code.

**Edge cases**: None.

---

### Item 8 — Fix typo `sucesses`

**Approach**: In `tests/common.h` line 34, rename `sucesses` to `successes`.

**Invariants**: None (local variable).

**Correctness criteria**: Cosmetic fix only.

**Test strategy**: Existing tests compile and run.

**Edge cases**: None.

---

## Item 9 — Align `linearAlloc_alloc`

**Approach**: Round up allocation sizes to the next multiple of `WORD`
(which is `sizeof(void*)`, i.e. 8 on amd64/arm64).

```c
static inline
void* linearAlloc_alloc(LinearAlloc* a, usize size) {
  usize aligned = (size + WORD - 1) & ~(WORD - 1);
  if (a->allocated + aligned > a->buffSize) {
    return NULL;
  }
  void* out = (void*)(a->buffer + a->allocated);
  a->allocated += aligned;
  return out;
}
```

The start of the usable buffer (`a->buffer = buffer + sizeof(LinearAlloc)`)
is already aligned because `sizeof(LinearAlloc)` contains a pointer, so the
struct itself forces pointer alignment. Document this with a `SAFE` footnote.

**Invariants**:
- Every returned pointer is aligned to `WORD`.
- `aligned >= size` (rounding up never shrinks).
- `a->allocated` is always a multiple of `WORD` after any allocation.

**Correctness criteria**:
- `(size + WORD - 1) & ~(WORD - 1)` rounds up correctly for all `size >= 0`.
- The bitwise AND requires `WORD` to be a power of two — `sizeof(void*)`
  is always a power of two on supported architectures.

**Test strategy** (modify `linearAlloc_test.c`):
- `test_alignment_basic`: Allocate 1 byte, check that the returned pointer
  is WORD-aligned. Allocate another 1 byte, check alignment again.
- `test_alignment_used`: Allocate 1 byte, verify `used()` returns `WORD`
  (not 1), confirming padding is accounted for.
- `test_alignment_exact`: Allocate exactly `WORD` bytes, verify no extra
  padding.

**Edge cases**:
- `size = 0` → `aligned = 0`, returns current pointer (valid but empty).
- `size = 1` → `aligned = WORD`.
- `size = WORD` → `aligned = WORD` (no padding).
- `size = WORD + 1` → `aligned = 2 * WORD`.

> **Impact on existing tests**: `test_usedAndAvailable` allocates 10 bytes
> at a time and asserts `used == (i+1) * 10`. After alignment, used will be
> `(i+1) * 16` (on 64-bit). This test must be updated.
> Similarly, `test_individualFreeFails` asserts `used == 16`, which happens
> to already be aligned — no change needed.

---

## Item 10 — Pool O(1) `pool_available`

**Approach**: Add a `usize freeCount` field to the `Pool` struct. Maintain it
as follows:
- `i_pool_setList`: count nodes while building the list, set `freeCount`.
- `pool_alloc`: decrement `freeCount` on success.
- `pool_free`: increment `freeCount`.
- `pool_available`: return `p->freeCount * p->chunkSize`.

In debug mode, add an assertion in `pool_available` that walks the list and
verifies the count matches.

**Invariants**:
- `freeCount * chunkSize == sum of all free chunks` (verified in debug mode).
- `freeCount` is decremented only when `pool_alloc` succeeds (returns
  non-NULL).
- `freeCount` never exceeds `size / chunkSize`.

**Correctness criteria**: `pool_available()` returns the same value as the
old O(n) implementation, but in O(1).

**Test strategy** (modify `pool_test.c`):
- `test_pool_available_afterAllocs`: Allocate several chunks, verify
  `pool_available()` decreases by `chunkSize` each time.
- `test_pool_available_afterFree`: Free some chunks, verify it increases.
- `test_pool_available_afterFreeAll`: Verify full capacity restored.

**Edge cases**:
- Pool with exactly 1 chunk.
- Pool fully exhausted (`freeCount == 0`).
- `pool_freeAll` resets count correctly.

---

## Items 11–13: Buffer Functions

---

### Items 11–12 — `buffer_copy` and `buffer_copySome`

**Approach**:
```c
// Copies min(in->len, out available) readable bytes from in to out.
// Does NOT consume bytes from in (does not advance in->start).
static inline
usize buffer_copy(const Buffer* in, Buffer* out);

// Copies min(len, in->len, out available) bytes from in to out.
// Does NOT consume bytes from in.
static inline
usize buffer_copySome(const Buffer* in, Buffer* out, usize len);
```

Implementation: `buffer_copySome` is the core. `buffer_copy` calls it with
`len = in->len`.

The copy reads from `in->ptr + in->start` and writes to
`out->ptr + out->start + out->len`. This is a plain byte-by-byte copy using
a `while` loop (no `memcpy` to stay consistent with `buffer_writeString`).

**Invariants**:
- `in` is not modified (const pointer, no side effects).
- `out->len` increases by exactly the return value.
- Return value <= `in->len` and <= available space in `out`.

**Correctness criteria**:
- Copies the correct bytes starting at `in->start`.
- Stops at whichever limit is reached first (input length, requested length,
  output space).

**Test strategy** (add to `buffer_test.c`):
- `test_buffer_copy_full`: Copy all bytes, verify contents match.
- `test_buffer_copy_outSmaller`: Output has less space than input length.
- `test_buffer_copy_empty`: Input has 0 length.
- `test_buffer_copySome_exact`: Request exactly available bytes.
- `test_buffer_copySome_overRequest`: Request more than available.
- `test_buffer_copySome_zero`: Request 0 bytes.
- `test_buffer_copy_afterRead`: Read some bytes from input first, then copy
  — verifies that copy starts at `in->start`, not `in->ptr`.

**Edge cases**:
- `in->len == 0`.
- `out` is full.
- `in` and `out` are the same buffer (undefined — document as disallowed).

---

### Item 13 — Test `buffer_toHex` and remove UNTESTED

**Approach**: Tests already exist in `buffer_test.c` (`test_buffer_toHex_1`,
`test_buffer_toHex_empty`, `test_buffer_toHex_outOfSpace`,
`test_buffer_toHex_appends`). The UNTESTED marker is stale.

Verify known-issues #4 (write offset bug): inspect `writeStart` on line 139.
Current code sets `writeStart = out->len`, which is correct — it records the
output length *before* writing, so the return value `out->len - writeStart`
is the number of bytes written by this call. The `snprintf` offset uses
`i_buffer_absLen(out)`, which is `out->start + out->len` — also correct.

**Action**: Remove the `UNTESTED: (TODO:)` marker from line 135. No code
change needed beyond the marker removal.

**Test strategy**: Existing tests are sufficient. Verify they pass.

---

## Items 14–21: Rational Arithmetic

All new rational functions follow these shared conventions:

### Shared Design Decisions

**Scratch parameters**: Following the pattern in `natural_gcd` and
`rational_equal`, all functions that need temporary `Natural` values take
them as explicit parameters. The caller provides pre-allocated scratch
Naturals. This avoids hidden allocation.

**Aliasing**: Disallowed — `out` must not alias `A` or `B`. Debug checks
will enforce this in `config_DEBUG` mode.

**Sign handling**: Zero is always positive (`sign = +1`). After any operation
that could produce zero, normalize the sign.

**Simplification**: `rational_add`, `rational_sub`, `rational_mul`, and
`rational_div` all simplify their result via `rational_simplify` before
returning.

---

### Item 14 — `rational_simplify`

**Approach**:
```
rational_simplify(mem, rat, scr_a, scr_b, scr_c, scr_div, scr_q):
  if numerator is zero:
    set denominator to 1
    set sign to +1
    return OK
  gcd = natural_gcd(mem, &rat->numerator, &rat->denominator, ...)
  rat->numerator = rat->numerator / gcd
  rat->denominator = rat->denominator / gcd
```

Needs 5 scratch Naturals (same as `natural_gcd` which takes 4, plus one for
the quotient from `natural_divDigit` or `natural_div`).

Actually, let me trace the scratch usage:
- `natural_gcd` needs: `scr_a`, `scr_b`, `scr_div`, `scr_q` (4 scratch).
  It uses `out` as `scr_c` internally.
- After GCD, we need `natural_div` to divide num and den by gcd. `natural_div`
  needs 1 scratch.

So the signature will be:
```c
static inline
Status rational_simplify(IAllocator* mem, Rational* rat,
                         Natural* scr_gcd,
                         Natural* scr_a, Natural* scr_b,
                         Natural* scr_div, Natural* scr_q);
```

That's 5 scratch Naturals total: `scr_gcd` receives the GCD result, then we
reuse the others for division.

**Invariants**:
- `rat->numerator / gcd` and `rat->denominator / gcd` have no common factors.
- The value of the rational (numerator/denominator) is unchanged.
- Zero numerator → denominator becomes 1 (canonical form).

**Correctness criteria**:
- `gcd(simplified_num, simplified_den) == 1`.
- `original_num * simplified_den == original_den * simplified_num`
  (cross-multiply check).

**Test strategy**:
- `test_rational_simplify_basic`: 6/4 → 3/2.
- `test_rational_simplify_alreadySimplified`: 3/7 → 3/7.
- `test_rational_simplify_zeroNumerator`: 0/5 → 0/1.
- `test_rational_simplify_one`: 5/5 → 1/1.
- `test_rational_simplify_negative`: -6/4 → -3/2.
- `test_rational_simplify_large`: GCD with multi-digit naturals.

**Edge cases**: Zero numerator, denominator = 1 (already simplified),
GCD = denominator (result is integer).

---

### Item 15 — `rational_neg`

**Approach**:
```c
static inline
void rational_neg(Rational* rat) {
  if (!natural_isZero(&rat->numerator)) {
    rat->sign *= -1;
  }
}
```

No allocator needed. No scratch needed.

**Invariants**:
- Zero stays positive.
- `neg(neg(x)) == x`.

**Correctness criteria**: Trivial — sign flip with zero guard.

**Test strategy**:
- `test_rational_neg_positive`: +3/4 → -3/4.
- `test_rational_neg_negative`: -3/4 → +3/4.
- `test_rational_neg_zero`: 0/5 stays +0/5.
- `test_rational_neg_doubleNeg`: neg(neg(x)) == x.

**Edge cases**: Zero.

---

### Item 16 — `rational_normalize`

**Approach**: Given two rationals A and B, set both to equivalent fractions
with the same denominator. The common denominator is `lcm(dA, dB)`.

```
lcm(a, b) = a * b / gcd(a, b)

rational_normalize(mem, A, B, scr_gcd, scr_a, scr_b, scr_div, scr_q):
  gcd = natural_gcd(dA, dB, ...)
  // scale_A = dB / gcd
  // scale_B = dA / gcd
  // new_denom = dA * scale_A  (== lcm)
  A->numerator *= scale_A
  A->denominator = new_denom
  B->numerator *= scale_B
  B->denominator = new_denom (copy)
```

This modifies both A and B in place.

Signature:
```c
static inline
Status rational_normalize(IAllocator* mem,
                          Rational* A, Rational* B,
                          Natural* scr_gcd,
                          Natural* scr_a, Natural* scr_b,
                          Natural* scr_div, Natural* scr_q);
```

**Invariants**:
- `A->denominator == B->denominator` after the call.
- The values of A and B (as fractions) are unchanged.

**Correctness criteria**:
- `new_A_num / new_denom == old_A_num / old_A_den`.
- `new_B_num / new_denom == old_B_num / old_B_den`.

**Test strategy**:
- `test_rational_normalize_sameDenom`: 1/4 and 3/4 → unchanged.
- `test_rational_normalize_different`: 1/2 and 1/3 → 3/6 and 2/6.
- `test_rational_normalize_oneMultiple`: 1/3 and 1/6 → 2/6 and 1/6.

**Edge cases**: Same denominator (no-op), one denominator divides the other.

---

### Item 17 — `rational_compare`

**Approach**: Cross-multiply to avoid division:
`a/b vs c/d` → compare `a*d` vs `c*b`, accounting for signs.

```c
static inline
Status rational_compare(IAllocator* mem,
                        const Rational* A, const Rational* B,
                        Natural* scr_a, Natural* scr_b,
                        Order* out);
```

Sign logic:
- If both zero → EQUAL.
- If signs differ → positive is GREATER.
- If both positive → compare `A.num * B.den` vs `B.num * A.den`.
- If both negative → invert the natural comparison.

**Invariants**:
- Reflexive: `compare(x, x) == EQUAL`.
- Anti-symmetric: `compare(a, b) == LESS ↔ compare(b, a) == GREATER`.

**Correctness criteria**: Cross-multiplication preserves ordering because
denominators are always positive Naturals.

**Test strategy**:
- `test_rational_compare_equal`: 1/2 vs 2/4 → EQUAL.
- `test_rational_compare_less`: 1/3 vs 1/2 → LESS.
- `test_rational_compare_greater`: 3/4 vs 1/4 → GREATER.
- `test_rational_compare_negativeVsPositive`: -1/2 vs 1/2 → LESS.
- `test_rational_compare_bothNegative`: -1/2 vs -1/3 → LESS.
- `test_rational_compare_zeros`: 0/3 vs 0/7 → EQUAL.

**Edge cases**: Zeros, different signs, equivalent fractions.

---

### Item 18 — `rational_add`

**Approach**: `a/b + c/d = (a*d + c*b) / (b*d)`, then simplify.

When signs differ, this becomes subtraction of naturals. Delegate to
`natural_add` or `natural_distance` based on sign comparison, mirroring
`integer_add`.

```c
static inline
Status rational_add(IAllocator* mem,
                    const Rational* A, const Rational* B,
                    Rational* out,
                    Natural* scr_gcd,
                    Natural* scr_a, Natural* scr_b,
                    Natural* scr_div, Natural* scr_q);
```

Algorithm:
```
ad = A.num * B.den
cb = B.num * A.den
out.den = A.den * B.den

if same sign:
  out.num = ad + cb
  out.sign = A.sign
else:
  compare ad vs cb
  out.num = |ad - cb|
  out.sign = sign of whichever was larger
  if ad == cb: out.sign = +1 (zero)

simplify(out)
```

**Invariants**:
- `out == A + B` as a fraction.
- Result is in simplified form.
- Zero is positive.

**Correctness criteria**: Standard fraction addition formula. Simplification
ensures canonical form.

**Test strategy**:
- `test_rational_add_sameDenom`: 1/4 + 1/4 = 1/2.
- `test_rational_add_differentDenom`: 1/2 + 1/3 = 5/6.
- `test_rational_add_negatives`: -1/2 + -1/3 = -5/6.
- `test_rational_add_cancel`: 1/2 + (-1/2) = 0.
- `test_rational_add_zero`: x + 0 = x.
- `test_rational_add_mixedSign`: 3/4 + (-1/4) = 1/2.

**Edge cases**: Result is zero, result needs simplification, one operand is
zero.

---

### Item 19 — `rational_sub`

**Approach**: `a/b - c/d = a/b + (-c/d)`. Implement by negating B's sign and
delegating to `rational_add`.

However, since `A` and `B` are `const`, we can't negate in-place. Instead,
duplicate the sign logic inline (same as `integer_sub` does relative to
`integer_add`):

```
ad = A.num * B.den
cb = B.num * A.den
out.den = A.den * B.den

if signs differ (A.sign != B.sign):
  out.num = ad + cb        // subtraction of opposite signs = addition
  out.sign = A.sign
else:
  compare ad vs cb
  out.num = |ad - cb|
  out.sign from comparison (same as addition's different-sign case but
  relative to A.sign)

simplify(out)
```

Same signature as `rational_add`.

**Invariants**: `out == A - B`.

**Test strategy**:
- `test_rational_sub_sameDenom`: 3/4 - 1/4 = 1/2.
- `test_rational_sub_resultZero`: 1/2 - 1/2 = 0.
- `test_rational_sub_negative`: 1/4 - 3/4 = -1/2.
- `test_rational_sub_mixedSign`: 1/2 - (-1/3) = 5/6.

**Edge cases**: Result is zero, result is negative.

---

### Item 20 — `rational_mul`

**Approach**: `(a/b) * (c/d) = (a*c) / (b*d)`, then simplify.

```c
static inline
Status rational_mul(IAllocator* mem,
                    const Rational* A, const Rational* B,
                    Rational* out,
                    Natural* scr_gcd,
                    Natural* scr_a, Natural* scr_b,
                    Natural* scr_div, Natural* scr_q);
```

Sign: `out.sign = A.sign * B.sign`. Normalize zero to positive.

**Invariants**: `out == A * B`, simplified.

**Test strategy**:
- `test_rational_mul_basic`: 2/3 * 3/4 = 1/2.
- `test_rational_mul_byZero`: x * 0 = 0.
- `test_rational_mul_byOne`: x * 1 = x.
- `test_rational_mul_negatives`: -2/3 * -3/4 = 1/2.
- `test_rational_mul_mixedSign`: 2/3 * -3/4 = -1/2.

**Edge cases**: Multiply by zero, multiply by 1/1, result needs
simplification.

---

### Item 21 — `rational_div`

**Approach**: `(a/b) / (c/d) = (a*d) / (b*c)`. Check for division by zero
(c == 0). Then simplify.

Same signature as `rational_mul`.

Sign: `out.sign = A.sign * B.sign`.

**Invariants**: `out == A / B`, simplified. Returns `status_DIVISIONBYZERO`
if B's numerator is zero.

**Test strategy**:
- `test_rational_div_basic`: (2/3) / (4/5) = 5/6.
- `test_rational_div_byOne`: x / 1 = x.
- `test_rational_div_byItself`: x / x = 1.
- `test_rational_div_byZero`: returns `status_DIVISIONBYZERO`.
- `test_rational_div_negatives`: signs combine correctly.

**Edge cases**: Division by zero, A is zero (result is zero).

---

## Item 22 — Refactor `rational_snprint`

**Approach**: Rewrite the body to use `Buffer` for writing instead of raw
pointer arithmetic:

```c
static inline
usize rational_snprint(const Rational* rat, char* buffer, usize size) {
  Buffer buf = buffer_create((byte*)buffer, size);
  if (rat->sign == -1) {
    if (buffer_writeByte(&buf, '-') == 0) return 0;
  }
  usize n = natural_snprint(&rat->numerator, (char*)(buf.ptr + i_buffer_absLen(&buf)), size - i_buffer_absLen(&buf));
  if (n == 0 && !natural_isZero(&rat->numerator)) return 0;
  buf.len += n;
  if (buffer_writeByte(&buf, '/') == 0) return 0;
  usize d = natural_snprint(&rat->denominator, (char*)(buf.ptr + i_buffer_absLen(&buf)), size - i_buffer_absLen(&buf));
  if (d == 0 && !natural_isZero(&rat->denominator)) return 0;
  buf.len += d;
  return buf.len;
}
```

Note: `natural_snprint` writes to a `char*` not a `Buffer`, so we can't use
`buffer_writeString` directly. We write the sign and `/` via `buffer_writeByte`
and let `natural_snprint` write directly into the buffer's backing array.

Alternative: keep the current implementation and just remove the TODO comment,
since the current code works and the refactor doesn't simplify much. The
`natural_snprint` API doesn't accept a `Buffer`, so the integration is awkward.

**Test strategy**: Existing `rational_test.c` tests cover `rational_snprint`
thoroughly. The refactor must not change behavior.

---

## Items 23–25: Natural Optimizations

**Note**: These are marked `OPT` in source. Since the open question about
deferral was not answered, I include the design but recommend implementing
them last.

---

### Item 23 — Optimize `natural_multBase`

**Approach**: When `i_natural_pushDigit` has to allocate a new array, copy
the digits shifted right by one position during the copy, avoiding the
separate O(n) shift loop.

```
if needs realloc:
  alloc new array
  copy digits[0..len-1] into new[1..len]
  new[0] = 0
else:
  shift in-place (existing code)
```

**Invariants**: Digits are shifted right by one, digit[0] becomes 0.

**Test strategy**: Existing `natural_test.c` extensively tests multiplication
and division which use `multBase`. No new tests needed.

---

### Item 24 — Optimize `natural_copy`

**Approach**: When `out->cap < A->len`, allocate `A->len` rounded up to the
next power of two (with a minimum of `natural_MINNATVEC`), instead of
`A->cap`.

```c
u32 newCap = natural_MINNATVEC;
while (newCap < A->len) {
  newCap *= 2;
}
```

**Invariants**: `out->cap >= A->len` after copy. Wastes at most 2x memory
instead of potentially copying A's full capacity.

**Test strategy**: Existing tests cover `natural_copy`. No new tests needed.

---

### Item 25 — Optimize `natural_divDigit`

**Approach**: Instead of `multBase(Q) + addDigit(Q, q, Q)` per iteration
(which shifts all digits each time), build the quotient in reverse using
`pushDigit`, then reverse the digit array.

```
while i >= 0:
  compute quotient digit q
  pushDigit(q, Q)
  i--
reverse Q->digits[0..Q->len-1]
removeLeadingZeroes(Q)
```

Add a helper `i_natural_reverseDigits(Natural* n)`.

**Invariants**: Same mathematical result as the current implementation.

**Test strategy**: Existing `natural_test.c` tests division thoroughly. No new
tests needed.

---

## Open Question Resolutions

Since requirements were approved without answering the open questions, I
assume:

1. **`iallocator.h` TODO**: Remove the comment (it's a design question, not
   actionable code).
2. **Natural optimizations**: Include in the plan but implement last (Phase 6
   in task ordering).
3. **`rational_snprint` refactor**: Do the refactor since it's an explicit
   in-code TODO.
