# Replace `natural_div` with Algorithm D — Design

---

## Item 1 — D1: Normalization

### Approach

Compute scale factor `d = natural_BASE / (MSD(B) + 1)` where MSD(B) is
`B->digits[B->len - 1]`. Both the dividend and divisor are multiplied by `d`
into temporary raw `u32[]` arrays allocated via the `IAllocator`:

- `u[]` of size `m + n + 1` (dividend may gain one digit from the carry).
- `v[]` of size `n` (divisor does not gain a digit because
  `d × MSD(B) < natural_BASE` by construction).

The multiplication is a single-pass `multDigit`-style loop on raw arrays:

```
carry = 0
i = 0
while i < len:
    prod = (i64)digits[i] * d + carry
    out[i] = (u32)(prod % BASE)
    carry  = (u32)(prod / BASE)
    i++
if carry > 0:
    out[len] = carry   // only for u[], never for v[]
```

Helper: `i_natural_rawMultDigit(const u32* src, u32 len, u32 d, u32* dst)`
— returns the final carry.

### Invariants

- After normalization: `v[n-1] >= BASE/2` (= 500000000).
- `d` is in range `[1, BASE-1]`. When MSD(B) >= BASE/2, `d == 1` (no-op).
- The normalized dividend `u[]` has at most `m + n + 1` digits.
- `d × (MSD(B) + 1) <= BASE` by integer division definition.

### Correctness criteria

- `d * B_normalized == d * B_original` digit-by-digit.
- MSD of `v[]` is `>= 500000000` for all valid inputs.

### Safety proofs

- `prod = (i64)digits[i] * d + carry`: worst case is
  `(BASE-1) * (BASE-1) + (BASE-1) = BASE² - BASE` ≈ 10¹⁸ < 2⁶³. Safe.
- `prod % BASE` and `prod / BASE` both fit in `u32` since `BASE < U32_MAX`.

---

## Item 2 — D3: Quotient digit estimation

### Approach

For each iteration `j` (from MSD to LSD of the quotient), estimate the
quotient digit from the top digits of the partial remainder:

```
// u[j], u[j+1], u[j+2] are the leading digits of the partial remainder
// v[n-1], v[n-2] are the leading digits of the normalized divisor
// All indices are in LSD-first order (translated from Knuth's MSD-first)

twoDigit = (i64)u[n+j] * BASE + u[n+j-1]    // top two digits of partial rem
qhat = (u32)(twoDigit / v[n-1])
rhat = (u32)(twoDigit % v[n-1])

// Knuth's correction: reduce qhat if it's too large
if qhat == BASE || (i64)qhat * v[n-2] > (i64)BASE * rhat + u[n+j-2]:
    qhat--
    rhat += v[n-1]
    if rhat < BASE:
        if qhat == BASE || (i64)qhat * v[n-2] > (i64)BASE * rhat + u[n+j-2]:
            qhat--
```

### Invariants

- After correction: `qhat <= q + 1` (Knuth Theorem B).
- After the full D4/D5/D6 cycle: `qhat == q` exactly.
- `qhat` is always in `[0, BASE-1]` after correction (it's a valid digit).

### Safety proofs

- `twoDigit = u[n+j] * BASE + u[n+j-1]`: worst case is
  `(BASE-1) * BASE + (BASE-1) = BASE² - 1` ≈ 10¹⁸ < 2⁶³. Safe in `i64`.
- `qhat * v[n-2]`: worst case `(BASE-1) * (BASE-1) = BASE² - 2·BASE + 1` < 2⁶³. Safe.
- `BASE * rhat + u[n+j-2]`: worst case `BASE * (BASE-1) + (BASE-1) = BASE² - 1` < 2⁶³. Safe.

### Edge case: `n+j-2` underflow

When `n == 2` and `j == 0`, `n+j-2 == 0` — valid (lowest index).
When `n >= 3`, always valid.
When `n < 2`, we should not be here (single-digit divisors are handled by the
fast path).

---

## Item 3 — D4: Multiply and subtract

### Approach

Fused multiply-subtract: `u[j..j+n] -= qhat × v[0..n-1]`.

Helper: `i_natural_mulSub(u32* u, u32 uOffset, const u32* v, u32 n, u32 qhat)`
— returns 1 if the result went negative (borrow out), 0 otherwise.

```
borrow = 0
i = 0
while i < n:
    prod = (i64)qhat * v[i] + borrow
    low  = (u32)(prod % BASE)
    borrow = (u32)(prod / BASE)
    if u[uOffset + i] < low:
        u[uOffset + i] = (u32)((i64)u[uOffset + i] + BASE - low)
        borrow++
    else:
        u[uOffset + i] -= low
    i++
// handle the (n+1)th position
if u[uOffset + n] < borrow:
    u[uOffset + n] = (u32)((i64)u[uOffset + n] + BASE - borrow)
    return 1   // went negative
else:
    u[uOffset + n] -= (u32)borrow
    return 0
```

### Invariants

- After D4, `u[j..j+n]` holds the partial remainder (possibly negative if
  borrow returned 1).
- The borrow is at most 1 after the loop because `qhat` is at most `q + 1`.

### Safety proofs

- `prod = (i64)qhat * v[i] + borrow`: worst case
  `(BASE-1)*(BASE-1) + (BASE-1) = BASE² - BASE` < 2⁶³. Safe.
- `(i64)u[...] + BASE - low`: worst case `0 + BASE - 0 = BASE` fits in `i64`.
- `borrow` stays within `[0, BASE]` at all times, fits in `u32` since `BASE < U32_MAX`.

---

## Item 4 — D5/D6: Test remainder and add-back

### Approach

If `i_natural_mulSub` returned 1 (borrow out), the quotient digit was
overestimated by 1. Correct:

```
if borrow:
    qhat--
    // add v[0..n-1] back to u[j..j+n], ignoring top carry
    carry = 0
    i = 0
    while i < n:
        sum = (i64)u[uOffset + i] + v[i] + carry
        u[uOffset + i] = (u32)(sum % BASE)
        carry = (u32)(sum / BASE)
        i++
    u[uOffset + n] += (u32)carry  // this carry cancels the borrow
```

Helper: `i_natural_addBack(u32* u, u32 uOffset, const u32* v, u32 n)`.

### Invariants

- After add-back, `u[j..j+n]` is non-negative and `< v` (correct partial remainder).
- The top carry from add-back cancels the borrow from D4 exactly (Knuth's proof).

### Correctness criteria

- This step fires with probability ≈ 2/β ≈ 2×10⁻⁹ — extremely rare, but must
  be correct. We write an explicit test to trigger it.

### Safety proofs

- `sum = u[...] + v[i] + carry`: worst case
  `(BASE-1) + (BASE-1) + 1 = 2·BASE - 1` fits in `i64`.

---

## Item 5 — D8: Unnormalize

### Approach

Divide the remainder digits `u[0..n-1]` by `d` to recover the true remainder.
This is a single-pass operation mirroring `divDigit` on raw arrays:

```
carry = 0
i = n - 1
while 0 <= i:
    dividend = carry * BASE + u[i]
    u[i] = (u32)(dividend / d)
    carry = (u32)(dividend % d)
    i--
```

Helper: `i_natural_rawDivDigit(u32* digits, u32 len, u32 d)`.

Then copy `u[0..n-1]` into `R`, trimming leading zeros.
Copy the quotient digits `q[0..m]` into `Q`, trimming leading zeros.

### Safety proofs

- `dividend = carry * BASE + u[i]`: `carry < d < BASE`, so
  `carry * BASE + u[i] < BASE² - 1 < 2⁶³`. Safe.

---

## Item 6 — Remove old helpers

### Approach

Delete `i_natural_testGuess` (lines 708–730) and
`i_natural_DIVMAXNUMGUESSES` (line 732). These are only referenced inside
the old `natural_div` body. Grep confirms no other callers.

---

## Item 7 — Single-digit fast path

### Approach

At the top of `natural_div`, after the existing early-return checks for B=0
and A=0, add:

```c
if (B->len == 1) {
    u32 r;
    st = natural_divDigit(mem, A, B->digits[0], Q, &r); status_CHECK;
    return natural_set(mem, r, R);
}
```

This avoids the normalization/Algorithm D overhead for a common case.

### Invariants

- `B->len == 1` means B is a single digit in `[1, BASE-1]`.
- `natural_divDigit` already handles all edge cases correctly.
- The scalar remainder `r` is wrapped into `R` via `natural_set`.

---

## Item 8 — New tests

### Test strategy

| Test | What it verifies |
|---|---|
| `test_natural_div_addback` | Force D6 add-back by constructing `A` and `B` where q̂ overestimates by 1. Use: `A = B * (BASE-1) + (B-1)`, so the true quotient digit is `BASE-2` but the estimation may guess `BASE-1`. |
| `test_natural_div_single_digit_divisor` | A has many digits, B has 1 digit — exercises the new fast path. Verify against `natural_divDigit` result. |
| `test_natural_div_equal_length` | A and B have the same number of digits (`m == 0`). |
| `test_natural_div_large_quotient` | A is much larger than B (large `m`), producing a many-digit quotient. |
| `test_natural_div_max_digits` | All digits at `BASE-1 = 999999999` to stress carry propagation. |
| `test_natural_div_theorem_multidigit` | Property test: for several multi-digit A,B pairs, verify `A == Q*B + R` and `R < B`. |

### Edge cases covered by existing tests

- `B == 0` → `status_DIVISIONBYZERO` (test 6)
- `A == 0` → Q=0, R=0 (test 2)
- `A < B` → Q=0, R=A (implicitly tested)
- `A == B` → Q=1, R=0 (test 8)
- `A / 1 == A` (test 7)
- Division theorem property over a sweep (tests 4, 5)
- Round-trip `mult then div` (growShrink_6)

---

## Internal helper summary

All new helpers are `static inline`, prefixed with `i_natural_`, and operate
on raw `u32[]` arrays to avoid allocating `Natural` structs:

| Helper | Signature | Purpose |
|---|---|---|
| `i_natural_rawMultDigit` | `(const u32* src, u32 len, u32 d, u32* dst) → u32 carry` | Multiply raw array by single digit |
| `i_natural_rawDivDigit` | `(u32* digits, u32 len, u32 d)` | Divide raw array by single digit in-place |
| `i_natural_mulSub` | `(u32* u, u32 uOff, const u32* v, u32 n, u32 qhat) → u32 borrow` | D4: fused multiply-subtract |
| `i_natural_addBack` | `(u32* u, u32 uOff, const u32* v, u32 n)` | D6: add divisor back after overestimation |

---

## Main `natural_div` structure (pseudocode)

```
natural_div(mem, scratch, A, B, Q, R):
    // debug checks (unchanged)
    // early returns: B==0, A==0

    // fast path (item 7)
    if B.len == 1:
        divDigit(mem, A, B.digits[0], Q, &r)
        set(mem, r, R)
        return

    // A < B check
    if compare(A, B) == LESS:
        set(mem, 0, Q)
        copy(mem, A, R)
        return

    n = B.len
    m = A.len - n

    // D1: Normalize
    d = BASE / (B.digits[n-1] + 1)
    allocate u[m+n+1], v[n]
    carry_u = rawMultDigit(A.digits, A.len, d, u)
    u[m+n] = carry_u
    rawMultDigit(B.digits, n, d, v)

    // D2-D7: Main loop
    allocate q[m+1]
    j = m
    while 0 <= j:
        // D3: estimate
        twoDigit = (i64)u[n+j] * BASE + u[n+j-1]
        qhat = twoDigit / v[n-1]
        rhat = twoDigit % v[n-1]
        // correction (up to 2 times)
        ...
        // D4: multiply and subtract
        borrow = mulSub(u, j, v, n, qhat)
        // D5/D6: test and add back
        if borrow:
            qhat--
            addBack(u, j, v, n)
        q[j] = qhat
        j--

    // D8: Unnormalize remainder
    rawDivDigit(u, n, d)

    // Copy q[] into Q, u[0..n-1] into R, trim leading zeros
    ...

    // Free u[], v[], q[]
```
