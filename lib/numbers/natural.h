/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_NATURAL_H
#define PAO_NATURAL_H

// remove when reimplemented memcpy
#include <string.h>

#include "../basicTypes.h"
#include "../status.h"
#include "../iallocator.h"
#include "../util.h"
#include "../order.h"
#include "../config.h"

#if config_DEBUG
  #include "../debug.h"
#endif

typedef struct {
  u32* digits;
  u32  cap;
  u32  len;
} Natural;

/* BEGIN: CONSTANTS */
#define natural_MINNATVEC 4
#define natural_DIGITSPERINT 9
#define natural_BASE 1000000000
/* END: CONSTANTS */

/* BEGIN: NATVEC */
static inline
u32* i_natural_natVecAlloc(IAllocator* mem, usize size, const char* func) {
  return (u32*)mem->alloc(mem->heap, size*sizeof(u32), func);
}

static inline
void i_natural_natVecCopy(u32* source, usize len, u32* dest) {
  memcpy(dest, source, len*sizeof(u32));
}

static inline
void i_natural_natVecFree(IAllocator* mem, u32* vec) {
  mem->free(mem->heap, vec);
}
/* END: NATVEC */

static inline
Natural natural_new(void) {
  Natural n;
  n.cap = 0;
  n.len = 0;
  n.digits = NULL;
  return n;
}

/* This is a version of `natural_new` that accepts a 
 * preallocated buffer. It can be combined with the NullAlloc
 * to create a wide fixed-precision number that exceeds 64bits.
 */
static inline
Natural natural_create(u32* digits, u32 size) {
  Natural n;
  n.cap = size;
  n.len = 0;
  n.digits = digits;
  return n;
}

static inline
void natural_free(IAllocator* mem, Natural n) {
  if (n.digits != NULL) {
    i_natural_natVecFree(mem, n.digits);
  }
}

static inline
Status i_natural_pushDigit(IAllocator* mem, u32 digit, Natural* out) {
  if (out->cap == 0) {
    out->digits = i_natural_natVecAlloc(mem, natural_MINNATVEC, (const char*)__func__);
    if (out->digits == NULL) {
      return status_OUTOFMEMORY;
    }
    out->cap = natural_MINNATVEC;
  }
  if (out->len == out->cap) {
    u32 new_cap = 2 * out->cap;
    if (new_cap > (u32)INT32_MAX) {
      // this limitation is acceptable, a ~8GB number is unrealistic.
      return status_NATURALNUMBEROVERFLOW;
    }

    u32* new_vec = i_natural_natVecAlloc(mem, new_cap, (char*)__func__);
    if (new_vec == NULL) {
      return status_OUTOFMEMORY;
    }
    i_natural_natVecCopy(out->digits, out->len, new_vec);
    i_natural_natVecFree(mem, out->digits);
    out->digits = new_vec;
    out->cap = new_cap;
  }
  u32 index = out->len;
  out->len++;
  out->digits[index] = digit;
  return status_OK;
}

/* Reverses the digit array in-place. Used to fix the order after building Q
   via pushDigit (which appends least-significant first when iterating from MSD). */
static inline
void i_natural_reverseDigits(Natural* N) {
  u32 lo = 0;
  u32 hi = N->len;
  while (lo + 1 < hi) {
    hi--;
    u32 tmp     = N->digits[lo];
    N->digits[lo] = N->digits[hi];
    N->digits[hi] = tmp;
    lo++;
  }
}

static inline
Status natural_multBase(IAllocator* mem, Natural* out) {
  if (out->len >= out->cap) {
    /* need to reallocate — copy shifted right by one into the new buffer */
    usize newCap = out->cap == 0 ? natural_MINNATVEC : out->cap * 2;
    u32* newVec = i_natural_natVecAlloc(mem, newCap, (char*)__func__);
    if (newVec == NULL) {
      return status_OUTOFMEMORY;
    }
    i_natural_natVecCopy(out->digits, out->len, newVec + 1); /* shift right by 1 */
    if (out->digits != NULL) {
      i_natural_natVecFree(mem, out->digits);
    }
    out->digits = newVec;
    out->cap    = (u32)newCap;
    out->len++;
    out->digits[0] = 0;
    return status_OK;
    /* NOTE(1): Instead of allocating with len+1 and then shifting in a
                separate pass, we copy digits into newVec+1 (already shifted)
                and write 0 at index 0. Saves O(n) operations per call. */
  }

  /* capacity is sufficient — shift in-place */
  i64 i = (i64)out->len; /* shift from out->len..1, writing to slots 1..len */
  while (0 < i) {
    out->digits[i] = out->digits[i-1];
    i--;
  }
  out->digits[0] = 0;
  out->len++;
  return status_OK;
}

/* Expects an array with the most significant digit first,
ie, MSD -> LSD. This is why the code goes backwards to fill
the number.
*/
static inline
Status natural_setVec(IAllocator* mem, u32* digits, i32 len, Natural* out) {
  #if config_DEBUG
    if (mem == NULL || digits == NULL || out == NULL) {
      debug_FATALFMT("Some pointer parameter is null. mem = %p, digits = %p, out = %p.", (void*)mem, (void*)digits, (void*)out);
    }
    if (len < 0) {
      debug_FATALFMT("Length can't be less than zero. Length = %d", len);
    }
    {
      i32 i = 0;
      while (i < len) {
        if (natural_BASE <= digits[i]) {
          debug_FATALFMT("`digits` contains an invalid digit. digit[%d] = %d.", i, digits[i]);
        }
        i++;
      }
    }
  #endif

  out->len = 0;
  int i = len-1;
  while (0 <= i) {
    Status st = i_natural_pushDigit(mem, digits[i], out); status_CHECK;
    i--;
  }
  return status_OK;
}

static inline
Status natural_set(IAllocator* mem, u32 digit, Natural* out) {
  #if config_DEBUG
    if (mem == NULL || out == NULL) {
      debug_FATALFMT("Some pointer parameter is null. mem = %p, out = %p.", (void*)mem, (void*)out);
    }
    if (natural_BASE <= digit) {
      debug_FATALFMT("`digit` is not a valid digit. digit = %d.", digit);
    }
  #endif

  if (digit == 0) {
    out->len = 0;
    return status_OK;
  }
  if (out->cap > 0) {
    out->len = 1;
    out->digits[0] = digit;
    return status_OK;
  }
  return i_natural_pushDigit(mem, digit, out);
}

static inline
bool natural_isZero(const Natural* N) {
  return N->len == 0;
}

/* Copies the contents of `A` to `out`,
if `out` has enough space, no allocations are performed.
*/
static inline
Status natural_copy(IAllocator* mem, const Natural* A, Natural* out) {
  #if config_DEBUG
    if (mem == NULL || A == NULL || out == NULL) {
      debug_FATALFMT("Some pointer parameter is null. mem = %p, A = %p, out = %p.", (void*)mem, (void*)A, (void*)out);
    }
    if (A == out) {
      debug_FATALFMT("Aliasing requirements not met. A = %p, out = %p.", (void*)A, (void*)out);
    }
  #endif

  if (natural_isZero(A)) {
    return natural_set(mem, 0, out);
  }
  /* allocate next power-of-two of A->len (min natural_MINNATVEC) */
  if (out->cap < A->len) {
    u32 newCap = natural_MINNATVEC;
    while (newCap < A->len) {
      newCap *= 2;
    }
    u32* outVec = i_natural_natVecAlloc(mem, newCap, (char*)__func__);
    if (outVec == NULL) {
      return status_OUTOFMEMORY;
    }
    if (out->digits != NULL) {
      i_natural_natVecFree(mem, out->digits);
    }
    out->digits = outVec;
    out->cap    = newCap;
    /* NOTE(1): We allocate the next power-of-two of A->len (rather than A->cap)
                to avoid copying a large allocation that holds mostly stale data.
                The minimum is natural_MINNATVEC to match the behaviour of other
                allocation sites. */
  }
  i_natural_natVecCopy(A->digits, A->len, out->digits);
  out->len = A->len;
  return status_OK;
}

static inline
bool natural_equalDigit(const Natural* A, u32 digit) {
  if (natural_isZero(A) && digit == 0) {
    return true;
  }
  return A->len == 1 && A->digits[0] == digit;
}

static inline
bool natural_equal(const Natural* A, const Natural* B) {
  if (A->len != B->len) {
    return false;
  }
  u32 i = 0;
  while (i < A->len) {
    if (A->digits[i] != B->digits[i]) {
      return false;
    }
    i++;
  }
  return true;
}

static inline
Order natural_compareDigit(const Natural* A, u32 b) {
  if (natural_isZero(A)) {
    if (b == 0) {
      return order_EQUAL;
    }
    if (b != 0) {
      return order_LESS;
    }
  }

  if (A->len > 1) {
    return order_GREATER;
  }
  u32 a = A->digits[0];
  if (a < b) {
    return order_LESS;
  } else if (b < a) {
    return order_GREATER;
  }
  return order_EQUAL;
}

static inline
Order natural_compare(const Natural* A, const Natural* B) {
  if (A->len < B->len) {
    return order_LESS;
  } else if (B->len < A->len) {
    return order_GREATER;
  }
  // NOTE(1)
  if (A->len == 0) {
    return order_EQUAL;
  }

  i64 i = A->len-1;
  while (0 <= i) {
    u32 a = A->digits[i];
    u32 b = B->digits[i];
    if (a < b) {
      return order_LESS;
    } else if (b < a) {
      return order_GREATER;
    }
    i--;
  }

  return order_EQUAL;
  /* NOTE(1): If this happens, both are zero.
  */
}

// `A` and `B` might be aliased together,
// but neither may be aliased with `out`.
static inline
Status natural_add(IAllocator* mem, const Natural* A, const Natural* B, Natural* out) {
  #if config_DEBUG
    if (mem == NULL || A == NULL || B == NULL || out == NULL) {
      debug_FATALFMT("Some pointer parameter is null. mem = %p, A = %p, B = %p, out = %p.", (void*)mem, (void*)A, (void*)B, (void*)out);
    }
  #endif
  Status st;

  u32 max_length = util_maxU32(A->len, B->len);
  u32 i = 0;
  i32 carry = 0;

  while (i < max_length || carry > 0) {
    i64 res = carry;
    if (i < A->len) {
      res += A->digits[i];
    }
    if (i < B->len) {
      res += B->digits[i];
    }

    u32 digit = 0;
    if (natural_BASE <= res) {
      carry = 1; // NOTE(1)
      digit = (u32)(res - natural_BASE); // SAFE(1)
    } else {
      carry = 0;
      digit = (u32)res; // SAFE(2)
    }
    if (i == out->len) {
      st = i_natural_pushDigit(mem, 0, out); status_CHECK;
    }
    out->digits[i] = digit;
    i++;
  }

  return status_OK;
  /* SAFE(1): `res` is at most equal to (BASE-1 + BASE-1) = (2*BASE - 2)
               this means that:
                 (res - BASE) <= (2*BASE - 2) - BASE
                               = BASE - 2
               ie, it fits both as a digit and as a u32.
     SAFE(2): in this branch, `res` is strictly less than BASE.
     NOTE(1): again, res <= 2*BASE - 2, so:
                 res / BASE <= (2* BASE - 2)/BASE
                             = 2 - 2/BASE
                             < 2.
              so the carry is at most an integer less than 2, ie, 1.
  */
}

// `A` and `out` may be the same object
static inline
Status natural_addDigit(IAllocator* mem, const Natural* A, u32 B, Natural* out) {
  #if config_DEBUG
    if (mem == NULL || A == NULL || out == NULL) {
      debug_FATALFMT("Some pointer parameter is null. mem = %p, A = %p, out = %p.", (void*)mem, (void*)A, (void*)out);
    }
    if (natural_BASE <= B) {
      debug_FATALFMT("B is not a valid digit. B = %d.", B);
    }
  #endif

  if (natural_isZero(A)) {
    return natural_set(mem, B, out);
  }

  Status st;
  u32 i = 0;
  u32 carry = B;
  i64 res = 0;

  do {
    res = carry;
    if (i < A->len) {
      res += A->digits[i];
    }

    if (natural_BASE <= res) {
      carry = 1;
      res -= natural_BASE;
    } else {
      carry = 0;
    }

    if (i == out->len) {
      st = i_natural_pushDigit(mem, 0, out); status_CHECK;
    }
    // SAFE(1):
    out->digits[i] = (u32)res;
    i++;
  } while (0 < carry || i < A->len);

  return status_OK;
  /* SAFE(1): since our carry is set to the digit in the first iteration,
              we need to prove the cast above is valid:

              if (carry < BASE) and (digit < BASE)
                 and ((res = carry+digit) >= BASE)
              then (res - BASE < BASE)
              proof:
                carry < BASE            implies
                carry + BASE < 2*BASE   implies
                carry + digit < 2*BASE  implies
                res - BASE < BASE
              as desired. \qed
  */
}

static inline
Status natural_multDigit(IAllocator* mem, const Natural* A, u32 B, Natural* out) {
  #if config_DEBUG
    if (mem == NULL || A == NULL || out == NULL) {
      debug_FATALFMT("Some pointer parameter is null. mem = %p, A = %p, out = %p.", (void*)mem, (void*)A, (void*)out);
    }

    if (A == out) {
      debug_FATALFMT("Aliasing requirements not met. A = %p, out = %p.", (void*)A, (void*)out);
    }

    if (natural_BASE <= B) {
      debug_FATALFMT("B is not a valid digit. B = %d.", B);
    }
  #endif
  Status st;

  if (natural_isZero(A) || B == 0) {
    return natural_set(mem, 0, out);
  }
  out->len = util_minU32(out->len, A->len);

  u32 i = 0;
  u32 carry = 0;

  while (i < A->len || carry > 0) {
    if (i == out->len) {
      st = i_natural_pushDigit(mem, 0, out); status_CHECK;
    }
    i64 res = carry;
    if (i < A->len) {
      res += (i64)A->digits[i] * (i64)B;
    }
    out->digits[i] = (u32)(res % natural_BASE); // SAFE(1)
    carry = (u32)(res / natural_BASE); // SAFE(2)

    i++; // UNSAFE(3)
  }
  return status_OK;
  /* SAFE(1): since `natural_BASE` is less than `U32_MAX` and `res`
     is always positive, this cast is safe.
     SAFE(2): Let us prove that `carry` is less than `natural_BASE` in
     any iteration. Let `natural_BASE` be called `K` for shorts.
     First, suppose that in the last iteration `carry < K`, then:
       res <= K-1 + (K-1)*(K-1) = K-1 + K^2 -2K + 1 = K^2-K
     When we divide both sides by `K` we get:
       carry = res/K <= K-1
     Which means `carry` is still less than `natural_BASE`.
     By induction, it can be proven that this is always the case,
     hence, the cast is safe.
     UNSAFE(3): If this overflows, our number has more than U32_MAX digits,
     which is to say we have a 16GB number...
  */
}

static inline
void i_natural_removeLeadingZeroes(Natural* out) {
  // UNSAFE(1):
  i32 i = (i32)out->len - 1;
  while (0 <= i && out->digits[i] == 0) {
    i--;
  }
  out->len = (u32)(i)+1;
  /* UNSAFE(1): all this casting shenannigans fails if a number passes I32_MAX digits,
                but I32_MAX * 4 bytes = 2GB * 4 = 8GB in a _single number_. Can you imagine
                any degenerate case where a single number has 8GB???
                We enforce this limit inside pushDigit, so that the function is safe as long
                as the user properly uses Natural.
  */
}

static inline
Status natural_mult(IAllocator* mem, const Natural* A, const Natural* B, Natural* out) {
  #if config_DEBUG
    if (mem == NULL || A == NULL || B == NULL || out == NULL) {
      debug_FATALFMT("Some pointer parameter is null. mem = %p, A = %p, B = %p, out = %p.", (void*)mem, (void*)A, (void*)B, (void*)out);
    }
    if (A == out || B == out) {
      debug_FATALFMT("Aliasing requirements not met. A = %p, B = %p, out = %p.", (void*)A, (void*)B, (void*)out);
    }
  #endif
  Status st;

  if (natural_isZero(A) || natural_isZero(B)) {
    return natural_set(mem, 0, out);
  }

  st = natural_set(mem, 0, out); status_CHECK;

  u32 i = 0;
  while (i < A->len) {
    u32 carry = 0;
    u32 j = 0;
    while (j < B->len || carry > 0) {
      if (i+j == out->len) {
        st = i_natural_pushDigit(mem, 0, out); status_CHECK;
      }
      i64 res = (i64)out->digits[i+j] + (i64)carry;
      if (j < B->len) {
        res += (i64)A->digits[i] * (i64)B->digits[j];
      }
      out->digits[i+j] = (u32)(res % natural_BASE); // SAFE(1)
      carry = (u32)(res / natural_BASE);             // SAFE(2)
      j++;
    }
    i++;
  }

  i_natural_removeLeadingZeroes(out);
  return status_OK;
  /* SAFE(1): `res` is always non-negative, and `res % natural_BASE`
              is strictly less than natural_BASE, so the cast to u32 is safe.
     SAFE(2): We need to show `carry` stays below natural_BASE across iterations.
              Suppose that the value of `carry` in the previous iteration did not exceed natural_BASE.
              Lets refer to `natural_BASE` as `B` for shorts.
              Then the largest `res` can be is:
                  (B-1) + (B-1) + (B-1)*(B-1)
                  = 2*B - 2 + B^2 - 2*B + 1
                  = B^2 - 1.
              Which means `res/B` is at most:
                  floor(B - 1/B) = B - 1.
              By induction this bound holds at every iteration.
  */
}

static inline
Status natural_distance(IAllocator* mem, const Natural* A, const Natural* B, Natural* out) {
  #if config_DEBUG
    if (mem == NULL || A == NULL || B == NULL || out == NULL) {
      debug_FATALFMT("Some pointer parameter is null. mem = %p, A = %p, B == %p, out = %p.", (void*)mem, (void*)A, (void*)B, (void*)out);
    }
  #endif
  Status st;

  const Natural* larger;
  const Natural* smaller;
  Order res = natural_compare(A, B);
  if (res == order_EQUAL) {
    return natural_set(mem, 0, out);
  } else if (res == order_LESS) {
    larger = B;
    smaller = A;
  } else {
    larger = A;
    smaller = B;
  }
  if (natural_isZero(smaller)) {
    return natural_copy(mem, larger, out);
  }

  i64 carry = 0;
  u32 i = 0;

  while (i < larger->len || carry > 0) {
    // NOTE(1)
    i64 res = -carry;
    if (i < larger->len) {
      res += (i64)larger->digits[i];
    }
    if (i < smaller->len) {
      res -= smaller->digits[i];
    }

    u32 digit = 0;
    if (res < 0) {
      carry = 1;
      digit = (u32)(natural_BASE + res); // SAFE(1)
    } else {
      carry = 0;
      digit = (u32)res; // SAFE(2)
    }

    if (i == out->len) {
      st = i_natural_pushDigit(mem, 0, out); status_CHECK;
    }
    out->digits[i] = digit;

    i++;
  }

  i_natural_removeLeadingZeroes(out);
  return status_OK;
  /* NOTE(1): `res` is not an i32 because `A->digits[i]` may be `u32_max`
              if BASE ever changes.
     SAFE(1): here `res < 0` and the largest negative number `res`
              can be is -BASE, the result is then 0.
     SAFE(2): `res` is at most BASE-1, when carry = 0, B->digits[i] = 0
              and A->digits[i] = BASE-1
  */
}

/*
Computes |A - B|, in other words:
  if B<A then A-B
  else B-A
*/
static inline
Status natural_distanceDigit(IAllocator* mem, const Natural* A, u32 B, Natural* out) {
  #if config_DEBUG
    if (mem == NULL || A == NULL || out == NULL) {
      debug_FATALFMT("Some pointer parameter is null. mem = %p, A = %p, out = %p.", (void*)mem, (void*)A, (void*)out);
    }

    if (natural_BASE <= B) {
      debug_FATALFMT("B is not a valid digit. B = %d.", B);
    }
  #endif
  if (natural_isZero(A)) {
    return natural_set(mem, B, out);
  }

  if (A->len == 1) {
    u32 digit = A->digits[0];
    u32 result = 0;
    if (digit < B) {
      result = B - digit;
    } else {
      result = digit - B;
    }
    return natural_set(mem, result, out);
  }

  // B < A, assuming A has no leading zeroes.
  Status st;
  i64 res = 0;
  i64 carry = B;
  u32 i = 0;

  do {
    // SAFE(1):
    res = A->digits[i] - carry;
    if (res < 0) {
      carry = 1;
      res += natural_BASE;
    } else {
      carry = 0;
    }

    if (i == out->len) {
      st = i_natural_pushDigit(mem, 0, out); status_CHECK;
    }
    // SAFE(2):
    out->digits[i] = (u32)res;

    i++;
  } while (carry > 0 || i < A->len);
  i_natural_removeLeadingZeroes(out);
  return status_OK;
  /* SAFE(1): since |A-B| < A, we can safely use `i` here, we will never
              have a situation where both `carry > 0` and `i >= A.len` are true.
     SAFE(2): see that because `A.digit[i]` and `B` are both positive numbers
              less than `natural_BASE`, then `res = A.digits[0] - B` is also less
              than the base, which is less than U32_MAX.
  */
}

/* Finds `Q` and `R` such that `A = Q*B + R`.
   `R` is guaranteed to be less than `B` by the Division Theorem,
   hence, it's a u32.
   DRAGONS:
*/
static inline
Status natural_divDigit(IAllocator* mem, const Natural* A, u32 B, Natural* Q, u32* R) {
  #if config_DEBUG
    if (mem == NULL || A == NULL || Q == NULL || R == NULL) {
      debug_FATALFMT("Some pointer parameter is null. mem = %p, A = %p, Q = %p, R = %p.", (void*)mem, (void*)A, (void*)Q, (void*)R);
    }

    if (A == Q) {
      debug_FATALFMT("Aliasing requirements not met. A = %p, out = %p.", (void*)A, (void*)Q);
    }

    if (natural_BASE <= B) {
      debug_FATALFMT("B is not a valid digit. B = %d.", B);
    }
  #endif

  if (B == 0) {
    return status_DIVISIONBYZERO;
  }
  if (natural_isZero(A)) {
    *R = 0;
    natural_set(mem, 0, Q);
    return status_OK;
  }

  Status st = natural_set(mem, 0, Q); status_CHECK;
  *R = 0;
  i64 i     = A->len - 1; /* SAFE(3) */
  i64 carry = 0;
  bool leadingZero = true; /* suppress leading zero digits in Q */

  while (0 <= i) {
    i64 idd = (i64)A->digits[i] + carry * natural_BASE; /* NOTE(1) */
    i64 q   = idd / B;                                  /* NOTE(3) */
    carry   = idd % B;                                  /* NOTE(2) */

    if (q != 0) { leadingZero = false; }
    if (!leadingZero) {
      st = i_natural_pushDigit(mem, (u32)q, Q); status_CHECK; /* SAFE(2) */
    }

    i--;
  }
  /* digits were pushed most-significant first (high→low), so Q is in reversed
     order: reverse to restore LSD-first invariant. */
  i_natural_reverseDigits(Q);
  *R = (u32)carry; /* SAFE(1) */
  return status_OK;
  /*
   NOTE(1): Since `B` is a digit, then `idd` is at most 2 digits.
   This means we can use an `i64` as intermediate dividend.
   NOTE(2): After this line, `carry` will be less than `B`. This follows
   immediately from the Division Theorem: `carry` becomes the intermediate remainder.
   SAFE(1): For the reasons stated above, `carry < B < natural_BASE`.
   NOTE(3): At the end of the loop, `carry` is less than `B`, which means an
   upper bound for `idd` at this point is
       (B-1)*natural_BASE + natural_BASE-1 = 
       B*natural_BASE - natural_BASE + natural_BASE - 1 = 
       B*natural_BASE - 1.
   Since `q = floor(idd/B)` then:
       floor(idd/B) <= 
       floor((B*natural_BASE - 1) / B) = 
       floor(natural_BASE - (1/B)).
   Since `0 < 1/B <= 1` then
       floor(natural_BASE - (1/B)) = natural_BASE - 1.
   So that `q <= natural_BASE - 1`.
   SAFE(2): Because of the reasons stated above, the cast is safe and
   `q` is a valid digit.
   SAFE(3): This subtraction is OK since we checked earlier that
   `A.len` is strictly bigger than zero (natural_isZero)
  */
}

/* Multiplies raw array `src[0..len-1]` by single digit `d`, writing to
   `dst[0..len-1]`. Returns the carry out of the most-significant position.
   `src` and `dst` may be the same pointer (in-place).
*/
static inline
u32 i_natural_rawMultDigit(const u32* src, u32 len, u32 d, u32* dst) {
  u32 carry = 0;
  u32 i = 0;
  while (i < len) {
    i64 prod = (i64)src[i] * (i64)d + (i64)carry;
    dst[i]  = (u32)(prod % natural_BASE); // SAFE(1)
    carry   = (u32)(prod / natural_BASE); // SAFE(2)
    i++;
  }
  return carry;
  /* SAFE(1): prod >= 0 and prod % BASE < BASE <= U32_MAX.
     SAFE(2): carry = prod/BASE. Worst case prod = (BASE-1)*(BASE-1)+(BASE-1)
              = BASE^2 - BASE. carry = (BASE^2-BASE)/BASE = BASE-1 < U32_MAX. */
}

/* Divides raw array `digits[0..len-1]` by single digit `d` in-place,
   MSD-to-LSD pass. Used for the D8 unnormalization step.
*/
static inline
void i_natural_rawDivDigit(u32* digits, u32 len, u32 d) {
  i64 carry = 0;
  i64 i = (i64)len - 1;
  while (0 <= i) {
    i64 dividend = carry * (i64)natural_BASE + (i64)digits[i];
    digits[i] = (u32)(dividend / (i64)d); // SAFE(1)
    carry      = dividend % (i64)d;       // SAFE(2)
    i--;
  }
  /* SAFE(1): carry < d < BASE, so dividend < BASE^2 < 2^63. Quotient digit
              <= BASE-1 by the division theorem (proven in natural_divDigit).
     SAFE(2): remainder < d < BASE, fits in i64. */
}

/* D4: Fused multiply-subtract: u[uOff..uOff+n] -= qhat * v[0..n-1].
   Returns 1 if the result went negative (borrow out), 0 otherwise.
*/
static inline
u32 i_natural_mulSub(u32* u, u32 uOff, const u32* v, u32 n, u32 qhat) {
  u32 borrow = 0;
  u32 i = 0;
  while (i < n) {
    i64 prod = (i64)qhat * (i64)v[i] + (i64)borrow; // SAFE(1)
    u32 low  = (u32)(prod % natural_BASE);
    borrow   = (u32)(prod / natural_BASE);
    if (u[uOff + i] < low) {
      u[uOff + i] = (u32)((i64)u[uOff + i] + (i64)natural_BASE - (i64)low); // SAFE(2)
      borrow++;
    } else {
      u[uOff + i] -= low;
    }
    i++;
  }
  /* handle the guard digit at position uOff+n */
  if (u[uOff + n] < borrow) {
    u[uOff + n] = (u32)((i64)u[uOff + n] + (i64)natural_BASE - (i64)borrow);
    return 1; /* went negative */
  }
  u[uOff + n] -= borrow;
  return 0;
  /* SAFE(1): worst case (BASE-1)*(BASE-1)+(BASE-1) = BASE^2-BASE < 2^63.
     SAFE(2): u[...] + BASE - low, worst case 0 + BASE - 0 = BASE < U32_MAX. */
}

/* D6: Add v[0..n-1] back to u[uOff..uOff+n], ignoring the top carry.
   Called only when D4 produced a negative result (extremely rare: ~2/BASE).
*/
static inline
void i_natural_addBack(u32* u, u32 uOff, const u32* v, u32 n) {
  u32 carry = 0;
  u32 i = 0;
  while (i < n) {
    i64 sum     = (i64)u[uOff + i] + (i64)v[i] + (i64)carry; // SAFE(1)
    u[uOff + i] = (u32)(sum % natural_BASE);
    carry       = (u32)(sum / natural_BASE);
    i++;
  }
  u[uOff + n] += carry; /* top carry cancels the borrow from D4 */
  /* SAFE(1): worst case (BASE-1)+(BASE-1)+1 = 2*BASE-1 < U32_MAX. */
}

/* Finds `Q` and `R` such that `A = Q*B + R` and `0 <= R < B`.
   Uses Knuth's Algorithm D (TAOCP 4.3.1) for multi-digit divisors.
   The `scratch` parameter is accepted but unused (kept for API stability).
*/
static inline
Status natural_div(IAllocator* mem, Natural* scratch,
                         const Natural* A, const Natural* B,
                         Natural* Q, Natural* R) {
  #if config_DEBUG
    if (mem == NULL || A == NULL || B == NULL || Q == NULL || R == NULL) {
      debug_FATALFMT("Some pointer parameter is null. mem = %p, A = %p, B = %p, Q = %p, R = %p.", (void*)mem, (void*)A, (void*)B, (void*)Q, (void*)R);
    }
    if (A == Q || A == R || B == Q || B == R) {
      debug_FATALFMT("Aliasing requirements not met. A = %p, B = %p, Q = %p, R = %p.", (void*)A, (void*)B, (void*)Q, (void*)R);
    }
  #endif

  (void)scratch; /* NOTE(1) */

  Status st;

  if (natural_isZero(B)) {
    return status_DIVISIONBYZERO;
  }
  if (natural_isZero(A)) {
    st = natural_set(mem, 0, Q); status_CHECK;
    return natural_set(mem, 0, R);
  }

  /* fast path: single-digit divisor */
  if (B->len == 1) {                               // NOTE(2)
    u32 r = 0;
    st = natural_divDigit(mem, A, B->digits[0], Q, &r); status_CHECK;
    return natural_set(mem, r, R);
  }

  /* fast path: A < B → Q=0, R=A */
  if (natural_compare(A, B) == order_LESS) {
    st = natural_set(mem, 0, Q); status_CHECK;
    return natural_copy(mem, A, R);
  }

  /* ----- Algorithm D ----- */
  u32 n = B->len;
  u32 m = A->len - n; /* NOTE(3) */

  /* D1: Normalize — choose d so MSD(v) >= BASE/2 after scaling */
  u32 d = (u32)(natural_BASE / ((i64)B->digits[n - 1] + 1)); // SAFE(1)

  /* allocate u[m+n+1] and v[n] */
  u32 uLen = m + n + 1;
  u32* u   = (u32*)mem->alloc(mem->heap, uLen * sizeof(u32), __func__);
  if (u == NULL) { return status_OUTOFMEMORY; }
  u32* v   = (u32*)mem->alloc(mem->heap, (usize)n * sizeof(u32), __func__);
  if (v == NULL) { mem->free(mem->heap, u); return status_OUTOFMEMORY; }
  u32* q   = (u32*)mem->alloc(mem->heap, (usize)(m + 1) * sizeof(u32), __func__);
  if (q == NULL) { mem->free(mem->heap, u); mem->free(mem->heap, v); return status_OUTOFMEMORY; }

  /* fill u[] = A * d, v[] = B * d */
  u32 uCarry = i_natural_rawMultDigit(A->digits, A->len, d, u);
  u[m + n]   = uCarry; /* guard digit, may be 0 */
  i_natural_rawMultDigit(B->digits, n, d, v); // SAFE(2)

  /* D2–D7: main loop, j runs from m down to 0 */
  /* NOTE(4): indices are LSD-first throughout.
     Knuth's u[j..j+n] in MSD-first corresponds to our u[(m-j)..(m-j+n)]
     with j_k = m - j_knuth. We iterate j_k from 0 to m (= j_knuth from m
     to 0) to keep the loop natural. */
  i64 j = (i64)m;
  while (0 <= j) {
    u32 jj = (u32)j; /* current quotient digit index */

    /* D3: Estimate qhat from top two digits of partial remainder */
    i64 twoDigit = (i64)u[jj + n] * (i64)natural_BASE + (i64)u[jj + n - 1]; // SAFE(3)
    i64 vTop     = (i64)v[n - 1];
    i64 qhat     = twoDigit / vTop;
    i64 rhat     = twoDigit % vTop;

    /* Knuth's correction: apply up to twice */
    i64 vNext = (n >= 2) ? (i64)v[n - 2] : 0; // SAFE(4)
    if (qhat >= (i64)natural_BASE ||
        qhat * vNext > (i64)natural_BASE * rhat + (i64)u[jj + n - 2]) { // SAFE(5)
      qhat--;
      rhat += vTop;
      if (rhat < (i64)natural_BASE) {
        if (qhat >= (i64)natural_BASE ||
            qhat * vNext > (i64)natural_BASE * rhat + (i64)u[jj + n - 2]) {
          qhat--;
        }
      }
    }

    /* D4: multiply and subtract */
    u32 borrow = i_natural_mulSub(u, jj, v, n, (u32)qhat); // SAFE(6)

    /* D5/D6: if remainder went negative, add back */
    if (borrow) {
      qhat--;
      i_natural_addBack(u, jj, v, n); /* DRAGON(1) */
    }

    q[jj] = (u32)qhat;
    j--;
  }

  /* D8: Unnormalize remainder (divide u[0..n-1] by d) */
  i_natural_rawDivDigit(u, n, d);

  /* Copy q[0..m] into Q, trimming leading zeros */
  st = natural_set(mem, 0, Q); status_CHECK;
  i64 qi = (i64)m;
  while (0 <= qi && q[qi] == 0) { qi--; }
  while (0 <= qi) {
    st = i_natural_pushDigit(mem, q[(u32)qi], Q); status_CHECK; // NOTE(5)
    qi--;
  }
  i_natural_reverseDigits(Q);

  /* Copy u[0..n-1] into R, trimming leading zeros */
  st = natural_set(mem, 0, R); status_CHECK;
  i64 ri = (i64)n - 1;
  while (0 <= ri && u[ri] == 0) { ri--; }
  while (0 <= ri) {
    st = i_natural_pushDigit(mem, u[(u32)ri], R); status_CHECK;
    ri--;
  }
  i_natural_reverseDigits(R);

  mem->free(mem->heap, u);
  mem->free(mem->heap, v);
  mem->free(mem->heap, q);
  return status_OK;

  /* NOTE(1): The `scratch` parameter was used by the old binary-search
              algorithm. Algorithm D allocates its own raw arrays internally
              and does not need an external Natural* scratch.
     NOTE(2): single-digit divisors are handled by natural_divDigit, which
              uses a simple i64 division at each step, avoiding all
              normalization overhead.
     NOTE(3): m = A.len - B.len >= 0 because the A < B case already returned.
     NOTE(4): We iterate j_k = m..0 (our LSD-first variable). Within u[], the
              partial remainder at step j_k occupies u[j_k .. j_k+n]. After
              step j_k, u[j_k] is the j_k-th digit of the remainder.
     NOTE(5): We push q[] MSD-first (qi from m down to 0), then reverse.
              pushDigit appends at index Q->len, so after the reverse the
              digits are in LSD-first order as required.
     SAFE(1): B->digits[n-1] >= 1 (it is the MSD of a nonzero natural, so
              no leading zeros), thus B->digits[n-1]+1 >= 2 and d < BASE.
              Also d >= 1 since B->digits[n-1] <= BASE-1 implies
              BASE/(BASE-1+1) = 1.
     SAFE(2): B * d never overflows u32[] because
              d * (B->digits[n-1] + 1) <= BASE, so
              d * B->digits[n-1] <= BASE - d < BASE. No carry out of v[].
     SAFE(3): u[jj+n] < BASE and u[jj+n-1] < BASE, so
              twoDigit < BASE^2 = 10^18 < 2^63. Safe in i64.
     SAFE(4): If n < 2 we cannot reach this code (single-digit case returned
              early). So n >= 2 and v[n-2] is always a valid access.
     SAFE(5): qhat * vNext: qhat <= BASE (before correction), vNext < BASE,
              so product <= BASE^2 < 2^63. BASE * rhat: rhat < BASE,
              so BASE*rhat < BASE^2 < 2^63. Both sides safe in i64.
     SAFE(6): qhat is in [0, BASE) after D3 correction, fits in u32.
     DRAGON(1): The add-back step fires with probability ~2/BASE ≈ 2e-9.
                It is exercised by test_natural_div_addback.
  */
}

static inline
Status natural_gcd(IAllocator* mem,
                   const Natural* A, const Natural* B,
                   Natural* out,
                   Natural* scr_a, Natural* scr_b,
                   Natural* scr_div, Natural* scr_q) {
  #if config_DEBUG
    if (mem == NULL || A == NULL || B == NULL || out == NULL) {
      debug_FATALFMT("Some pointer parameter is null. mem = %p, A = %p, B = %p, out = %p.", (void*)mem, (void*)A, (void*)B, (void*)out);
    }
    if (scr_a == NULL || scr_b == NULL || scr_div == NULL || scr_q == NULL) {
      debug_FATALFMT("Some scratch parameter is null. scr_a = %p, scr_b = %p, scr_c = %p, scr_d = %p.", (void*)scr_a, (void*)scr_b, (void*)scr_div, (void*)scr_q);
    }
  #endif
  Status st;
  Natural* scr_c = out;

  st = natural_set(mem, 0, scr_a); status_CHECK;
  st = natural_set(mem, 0, scr_b); status_CHECK;
  st = natural_set(mem, 0, scr_c); status_CHECK;
  st = natural_set(mem, 0, scr_div); status_CHECK;
  st = natural_set(mem, 0, scr_q); status_CHECK;

  st = natural_copy(mem, A, scr_a); status_CHECK;
  st = natural_copy(mem, B, scr_b); status_CHECK;

  while (natural_isZero(scr_b) == false) {
    st = natural_div(mem, scr_div, scr_a, scr_b, scr_q, scr_c); status_CHECK;
    Natural* hold = scr_a;
    scr_a = scr_b;
    scr_b = scr_c;
    scr_c = hold;
  }
  if (scr_a != out) {
    st = natural_copy(mem, scr_a, out); status_CHECK;
  }
  /* NOTE(1): After the Euclidean loop, scr_a (local) may point at any of the
              three pointer-swapped slots including `out` itself. When scr_a ==
              out the result is already in place and the copy is a no-op. */
  return status_OK;
}

static inline
char* i_natural_firstNonzeroChar(char* buffer, usize buffSize) {
  usize i = 0;
  while (i < buffSize && buffer[i] == '0') {
    i++;
  }
  return buffer+i;
}

static inline
void i_natural_WriteU32(u32 n, char* buffer) {
  memset(buffer, '0', natural_DIGITSPERINT);
  if (n == 0) {
    return;
  }

  char* b = buffer + natural_DIGITSPERINT -1;
  while (n > 0) {
    *b = (char)(n%10) + '0';
    b--;
    n = n/10;
  }
  return;
}

static inline
void i_natural_WriteU32Tight(u32 n, char* buffer, u32 digitCount) {
  char* b = buffer + digitCount - 1;
  while (buffer <= b) {
    *b = (char)(n%10) + '0';
    b--;
    n = n/10;
  }
}

static inline
u32 i_natural_digitCountU32(u32 n) {
  u32 count = 1;
  while (n >= 10) {
    n /= 10;
    count++;
  }
  return count;
}

static inline
usize natural_printingSize(const Natural* nat) {
  if (nat->len == 0) {
    return 1; // zero needs one byte.
  }
  // nat->len >= 1
  u32 msd = nat->digits[nat->len - 1];
  u32 msdWidth = i_natural_digitCountU32(msd);
  usize neededBytes = (usize)msdWidth + (usize)(nat->len - 1) * natural_DIGITSPERINT;
  return neededBytes;
}

/* Only writes a number if the given buffer has sufficient size, ie,
it either fully writes the number or returns 0.
*/
static inline
usize natural_snprint(const Natural* nat, char* buffer, usize size) {
  #if config_DEBUG
    if (nat == NULL || buffer == NULL) {
      debug_FATALFMT("Some pointer parameter is null. nat = %p, buffer = %p.", (void*)nat, (void*)buffer);
    }
  #endif

  usize neededBytes = natural_printingSize(nat);
  if (neededBytes > size) {
    return 0;
  }

  if (nat->len == 0) {
    *buffer = '0';
    return 1;
  }

  char* block = buffer;
  { // write msd
    u32 msd = nat->digits[nat->len - 1];
    u32 msdWidth = i_natural_digitCountU32(msd);
    i_natural_WriteU32Tight(msd, block, msdWidth);
    block += msdWidth;
  }

  i64 i = (i64)nat->len - 2;
  while (0 <= i) {
    // SAFE(1):
    i_natural_WriteU32(nat->digits[i], block);
    block += natural_DIGITSPERINT;
    i--;
  }

  usize writtenSize = (uptr)block - (uptr)buffer;
  return writtenSize;
  /* SAFE(1): We already know the buffer has enough space for the
              remaining (non-leading) limbs, since `neededBytes`
              accounts for them at full width, see note(1).
  */
}

#endif
