# Replace `natural_div` with Knuth's Algorithm D — Requirements

**Goal**: Replace the current binary-search-based long division in `natural_div`
with Knuth's Algorithm D (TAOCP 4.3.1), producing identical results via a
more efficient and well-understood O(m·n) algorithm.

---

## Items

1. **Implement D1 — Normalization**: Compute scale factor `d = β / (MSD(B) + 1)`.
   Multiply both A and B by `d` into temporary raw `u32[]` arrays, so that the
   MSD of the normalized divisor ≥ ⌊β/2⌋ = 500000000.

2. **Implement D3 — Quotient digit estimation**: Estimate q̂ from the leading
   digits of the partial remainder and divisor. Apply Knuth's correction test
   using the next digit to guarantee q̂ ≤ q+1 after normalization.

3. **Implement D4 — Multiply and subtract**: Fused multiply-subtract step:
   `u[j..j+n] -= q̂ × v[0..n-1]`, carried out in a single pass with borrow
   propagation.

4. **Implement D5/D6 — Test remainder and add-back**: If D4 produced a negative
   result (borrow out), decrement q̂ and add the divisor back. This corrects the
   rare case where q̂ was overestimated by 1.

5. **Implement D8 — Unnormalize**: Divide the remainder digits by the scale
   factor `d` to recover the true remainder.

6. **Remove old helpers**: Delete `i_natural_testGuess` and
   `i_natural_DIVMAXNUMGUESSES`, which are only used by the current
   binary-search division.

7. **Add single-digit fast path**: When `B.len == 1`, delegate to
   `natural_divDigit` and wrap the scalar remainder into `R`.

8. **Write new tests**: Add tests targeting Algorithm D edge cases — add-back
   trigger, equal-length operands, large quotient, maximum-digit carries,
   and a multi-digit division theorem property test.

---

## Out of Scope

- Changing the public API signature of `natural_div`.
- Optimizing `natural_divDigit` (already correct and efficient).
- Sub-quadratic division (Burnikel-Ziegler, Newton-based).
- Changing the base from 10⁹.

---

## Affected Modules

| File | Change |
|---|---|
| [natural.h](file:///home/ff/dev/pao/lib/numbers/natural.h) | Rewrite `natural_div` (lines 708–824), remove `i_natural_testGuess` and `i_natural_DIVMAXNUMGUESSES`, add new internal helpers |
| [natural_test.c](file:///home/ff/dev/pao/tests/numbers/natural/natural_test.c) | Add ≥6 new test functions, register in `Tester` array |

### Callers (unchanged API, must verify via test suite)

| File | Usage |
|---|---|
| [natural.h](file:///home/ff/dev/pao/lib/numbers/natural.h#L931) | `natural_gcd` calls `natural_div` in the Euclidean loop |
| [integer.h](file:///home/ff/dev/pao/lib/numbers/integer.h#L189) | `integer_div` delegates to `natural_div` |
| [rational.h](file:///home/ff/dev/pao/lib/numbers/rational.h#L166) | `rational_simplify` and `rational_normalize` call `natural_div` |

---

## Dependencies

None. All items are independent of other pending TODOs. The existing callers all
use the same `natural_div` signature — no sequencing constraints.

---

## Risks

1. **Off-by-one in quotient estimation** — This is the #1 source of bugs in
   Algorithm D implementations. Mitigated by: (a) rigorous overflow proofs in
   the design phase, (b) explicit add-back test case, (c) the existing
   property-based tests 4/5 that sweep all divisors.

2. **`i64` overflow** — Algorithm D requires multiplying two base-β digits and
   adding a carry. With β = 10⁹, the worst case intermediate is
   `(β-1)·(β-1) + (β-1) = β² - β` ≈ 10¹⁸, which fits in `i64` (max ≈ 9.2×10¹⁸).
   Safe, but must be verified for every intermediate expression.

---

## Resolved Questions

> **Q1 — Scratch parameter**: **Decided: keep and ignore.** The `Natural* scratch`
> parameter stays in the signature for API stability. Algorithm D allocates raw
> `u32[]` arrays internally and does not use the caller-provided scratch.

> **Q2 — Single-digit fast path**: **Decided: yes.** Delegate to
> `natural_divDigit` when `B.len == 1`.
