/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_natural_H
#define PAO_natural_H

// remove when reimplemented memcpy
#include <string.h>

#include "../pao_basicTypes.h"
#include "../pao_status.h"
#include "../pao_allocator.h"
#include "../pao_util.h"
#include "../pao_order.h"
#include "../pao_config.h"

#if PAO_config_debug
  #include "../pao_debug.h"
#endif

typedef struct {
  u32* digits;
  u32  cap;
  u32  len;
} pao_Natural;

/* BEGIN: CONSTANTS */
#define PAO_natural_minNatVec 4
#define PAO_natural_digitsPerInt 9
#define PAO_natural_base 1000000000
/* END: CONSTANTS */

/* BEGIN: NATVEC */
static inline
u32* i_pao_natural_natVecAlloc(pao_Allocator* mem, usize size, char* func) {
  return (u32*)mem->alloc(mem->heap, size*sizeof(u32), func);
}

static inline
void i_pao_natural_natVecCopy(u32* source, usize len, u32* dest) {
  memcpy(dest, source, len*sizeof(u32));
}

static inline
void i_pao_natural_natVecFree(pao_Allocator* mem, u32* vec) {
  mem->free(mem->heap, vec);
}
/* END: NATVEC */

/* BEGIN: UTIL */

/* END: UTIL */
static inline
pao_Natural pao_natural_new(void) {
  pao_Natural n;
  n.cap = 0;
  n.len = 0;
  n.digits = NULL;
  return n;
}

static inline
void pao_natural_free(pao_Allocator* mem, pao_Natural n) {
  i_pao_natural_natVecFree(mem, n.digits);
}

static inline
pao_Status i_pao_natural_pushDigit(pao_Allocator* mem, u32 digit, pao_Natural* out) {
  if (out->cap == 0) {
    out->digits = i_pao_natural_natVecAlloc(mem, PAO_natural_minNatVec, (char*)__func__);
    if (out->digits == NULL) {
      return PAO_status_outOfMemory;
    }
    out->cap = PAO_natural_minNatVec;
  }
  if (out->len == out->cap) {
    u32 new_cap = 2 * out->cap;
    if (new_cap > (u32)INT32_MAX) {
      // this limitation is acceptable, a ~8GB number is unrealistic.
      return PAO_status_naturalNumberOverflow;
    }

    u32* new_vec = i_pao_natural_natVecAlloc(mem, new_cap, (char*)__func__);
    if (new_vec == NULL) {
      return PAO_status_outOfMemory;
    }
    i_pao_natural_natVecCopy(out->digits, out->len, new_vec);
    i_pao_natural_natVecFree(mem, out->digits);
    out->digits = new_vec;
    out->cap = new_cap;
  }
  u32 index = out->len;
  out->len++;
  out->digits[index] = digit;
  return PAO_status_ok;
}

static inline
pao_Status pao_natural_multBase(pao_Allocator* mem, pao_Natural* out) { 
  // TODO: OPT: if pushDigit has to allocate, then we can shift everything while copying. Saves O(n) operations.
  pao_Status st = i_pao_natural_pushDigit(mem, 0, out); PAO_status_check;

  i64 i = out->len - 1;
  while (0 < i) {
    out->digits[i] = out->digits[i-1];
    i--;
  }
  out->digits[0] = 0;
  return PAO_status_ok;
}

/* Expects an array with the most significant digit first,
ie, MSD -> LSD. This is why the code goes backwards to fill
the number.
*/
static inline
pao_Status pao_natural_setVec(pao_Allocator* mem, u32* digits, i32 len, pao_Natural* out) {
  #if PAO_config_debug
    if (mem == NULL || digits == NULL || out == NULL) {
      PAO_debug_fatalFmt("Some pointer parameter is null. mem = %p, digits = %p, out = %p.", (void*)mem, (void*)digits, (void*)out);
    }
    if (len < 0) {
      PAO_debug_fatalFmt("Length can't be less than zero. Length = %d", len);
    }
    {
      i32 i = 0;
      while (i < len) {
        if (PAO_natural_base <= digits[i]) {
          PAO_debug_fatalFmt("`digits` contains an invalid digit. digit[%d] = %d.", i, digits[i]);
        }
        i++;
      }
    }
  #endif

  out->len = 0;
  int i = len-1;
  while (0 <= i) {
    pao_Status st = i_pao_natural_pushDigit(mem, digits[i], out); PAO_status_check;
    i--;
  }
  return PAO_status_ok;
}

static inline
pao_Status pao_natural_set(pao_Allocator* mem, u32 digit, pao_Natural* out) {
  #if PAO_config_debug
    if (mem == NULL || out == NULL) {
      PAO_debug_fatalFmt("Some pointer parameter is null. mem = %p, out = %p.", (void*)mem, (void*)out);
    }
    if (PAO_natural_base <= digit) {
      PAO_debug_fatalFmt("`digit` is not a valid digit. digit = %d.", digit);
    }
  #endif

  if (digit == 0) {
    out->len = 0;
    return PAO_status_ok;
  }
  if (out->cap > 0) {
    out->len = 1;
    out->digits[0] = digit;
    return PAO_status_ok;
  }
  return i_pao_natural_pushDigit(mem, digit, out);
}

static inline
bool pao_natural_isZero(const pao_Natural* N) {
  return N->len == 0;
}

/* Copies the contents of `A` to `out`,
if `out` has enough space, no allocations are performed.
*/
static inline
pao_Status pao_natural_copy(pao_Allocator* mem, const pao_Natural* A, pao_Natural* out) {
  #if PAO_config_debug
    if (mem == NULL || A == NULL || out == NULL) {
      PAO_debug_fatalFmt("Some pointer parameter is null. mem = %p, A = %p, out = %p.", (void*)mem, (void*)A, (void*)out);
    }
    if (A == out) {
      PAO_debug_fatalFmt("Aliasing requirements not met. A = %p, out = %p.", (void*)A, (void*)out);
    }
  #endif

  if (pao_natural_isZero(A)) {
    return pao_natural_set(mem, 0, out);
  }
  // TODO: OPT: this should only allocate A->len+pad to next power of two, not A->cap, potentially wastes memory.
  if (out->cap < A->len) {
    u32* outVec = i_pao_natural_natVecAlloc(mem, A->cap, (char*)__func__);
    if (outVec == NULL) {
      return PAO_status_outOfMemory;
    }
    i_pao_natural_natVecFree(mem, out->digits);
    out->digits = outVec;
    out->cap = A->cap;
  }
  i_pao_natural_natVecCopy(A->digits, A->len, out->digits);
  out->len = A->len;
  return PAO_status_ok;
}

static inline
bool pao_natural_equalDigit(const pao_Natural* A, u32 digit) {
  if (A->len == 0 && digit == 0) {
    return true;
  }
  return A->len == 1 && A->digits[0] == digit;
}

static inline
bool pao_natural_equal(const pao_Natural* A, const pao_Natural* B) {
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
pao_Order pao_natural_compareDigit(const pao_Natural* A, u32 b) {
  if (b == 0 && A->len == 0) {
    return PAO_order_equal;
  }
  if (b != 0 && A->len == 0) {
    return PAO_order_less;
  }

  if (A->len > 1) {
    return PAO_order_greater;
  }
  u32 a = A->digits[0];
  if (a < b) {
    return PAO_order_less;
  } else if (b < a) {
    return PAO_order_greater;
  }
  return PAO_order_equal;
}

static inline
pao_Order pao_natural_compare(const pao_Natural* A, const pao_Natural* B) {
  if (A->len < B->len) {
    return PAO_order_less;
  } else if (B->len < A->len) {
    return PAO_order_greater;
  }
  // NOTE(1)
  if (A->len == 0) {
    return PAO_order_equal;
  }

  i64 i = A->len-1;
  while (0 <= i) {
    u32 a = A->digits[i];
    u32 b = B->digits[i];
    if (a < b) {
      return PAO_order_less;
    } else if (b < a) {
      return PAO_order_greater;
    }
    i--;
  }

  return PAO_order_equal;
  /* NOTE(1): If this happens, both are zero.
  */
}

// `A` and `B` might be aliased together,
// but neither may be aliased with `out`.
static inline
pao_Status pao_natural_add(pao_Allocator* mem, const pao_Natural* A, const pao_Natural* B, pao_Natural* out) {
  #if PAO_config_debug
    if (mem == NULL || A == NULL || B == NULL || out == NULL) {
      PAO_debug_fatalFmt("Some pointer parameter is null. mem = %p, A = %p, B = %p, out = %p.", (void*)mem, (void*)A, (void*)B, (void*)out);
    }
  #endif
  pao_Status st;

  u32 max_length = pao_util_maxU32(A->len, B->len);
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
    if (PAO_natural_base <= res) {
      carry = 1; // NOTE(1)
      digit = (u32)(res - PAO_natural_base); // SAFE(1)
    } else {
      carry = 0;
      digit = (u32)res; // SAFE(2)
    }
    if (i == out->len) {
      st = i_pao_natural_pushDigit(mem, 0, out); PAO_status_check;
    }
    out->digits[i] = digit;
    i++;
  }

  return PAO_status_ok;
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
pao_Status pao_natural_addDigit(pao_Allocator* mem, const pao_Natural* A, u32 B, pao_Natural* out) {
  #if PAO_config_debug
    if (mem == NULL || A == NULL || out == NULL) {
      PAO_debug_fatalFmt("Some pointer parameter is null. mem = %p, A = %p, out = %p.", (void*)mem, (void*)A, (void*)out);
    }
    if (PAO_natural_base <= B) {
      PAO_debug_fatalFmt("B is not a valid digit. B = %d.", B);
    }
  #endif

  if (pao_natural_isZero(A)) {
    return pao_natural_set(mem, B, out);
  }

  pao_Status st;
  u32 i = 0;
  u32 carry = B;
  i64 res = 0;

  do {
    res = carry;
    if (i < A->len) {
      res += A->digits[i];
    }

    if (PAO_natural_base <= res) {
      carry = 1;
      res -= PAO_natural_base;
    } else {
      carry = 0;
    }

    if (i == out->len) {
      st = i_pao_natural_pushDigit(mem, 0, out); PAO_status_check;
    }
    // SAFE(1):
    out->digits[i] = (u32)res;
    i++;
  } while (0 < carry || i < A->len);

  return PAO_status_ok;
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
pao_Status pao_natural_multDigit(pao_Allocator* mem, const pao_Natural* A, u32 B, pao_Natural* out) {
  #if PAO_config_debug
    if (mem == NULL || A == NULL || out == NULL) {
      PAO_debug_fatalFmt("Some pointer parameter is null. mem = %p, A = %p, out = %p.", (void*)mem, (void*)A, (void*)out);
    }

    if (A == out) {
      PAO_debug_fatalFmt("Aliasing requirements not met. A = %p, out = %p.", (void*)A, (void*)out);
    }

    if (PAO_natural_base <= B) {
      PAO_debug_fatalFmt("B is not a valid digit. B = %d.", B);
    }
  #endif
  pao_Status st;

  if (pao_natural_isZero(A) || B == 0) {
    return pao_natural_set(mem, 0, out);
  }
  out->len = pao_util_minU32(out->len, A->len);

  u32 i = 0;
  u32 carry = 0;

  while (i < A->len || carry > 0) {
    if (i == out->len) {
      st = i_pao_natural_pushDigit(mem, 0, out); PAO_status_check;
    }
    i64 res = carry;
    if (i < A->len) {
      res += (i64)A->digits[i] * (i64)B;
    }
    out->digits[i] = (u32)(res % PAO_natural_base); // SAFE(1)
    carry = (u32)(res / PAO_natural_base); // SAFE(2)

    i++; // UNSAFE(3)
  }
  return PAO_status_ok;
  /* SAFE(1): since `PAO_natural_base` is less than `U32_MAX` and `res`
     is always positive, this cast is safe.
     SAFE(2): Let us prove that `carry` is less than `PAO_natural_base` in
     any iteration. Let `PAO_natural_base` be called `K` for shorts.
     First, suppose that in the last iteration `carry < K`, then:
       res <= K-1 + (K-1)*(K-1) = K-1 + K^2 -2K + 1 = K^2-K
     When we divide both sides by `K` we get:
       carry = res/K <= K-1
     Which means `carry` is still less than `PAO_natural_base`.
     By induction, it can be proven that this is always the case,
     hence, the cast is safe.
     UNSAFE(3): If this overflows, our number has more than U32_MAX digits,
     which is to say we have a 16GB number...
  */
}

static inline
void i_pao_natural_removeLeadingZeroes(pao_Natural* out) {
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
                as the user properly uses pao_Natural.
  */
}

static inline
pao_Status pao_natural_mult(pao_Allocator* mem, const pao_Natural* A, const pao_Natural* B, pao_Natural* out) {
  #if PAO_config_debug
    if (mem == NULL || A == NULL || B == NULL || out == NULL) {
      PAO_debug_fatalFmt("Some pointer parameter is null. mem = %p, A = %p, B = %p, out = %p.", (void*)mem, (void*)A, (void*)B, (void*)out);
    }
    if (A == out || B == out) {
      PAO_debug_fatalFmt("Aliasing requirements not met. A = %p, B = %p, out = %p.", (void*)A, (void*)B, (void*)out);
    }
  #endif
  pao_Status st;

  if (pao_natural_isZero(A) || pao_natural_isZero(B)) {
    return pao_natural_set(mem, 0, out);
  }

  st = pao_natural_set(mem, 0, out); PAO_status_check;

  u32 i = 0;
  while (i < A->len) {
    u32 carry = 0;
    u32 j = 0;
    while (j < B->len || carry > 0) {
      if (i+j == out->len) {
        st = i_pao_natural_pushDigit(mem, 0, out); PAO_status_check;
      }
      i64 res = (i64)out->digits[i+j] + (i64)carry;
      if (j < B->len) {
        res += (i64)A->digits[i] * (i64)B->digits[j];
      }
      out->digits[i+j] = (u32)(res % PAO_natural_base); // SAFE(1)
      carry = (u32)(res / PAO_natural_base);             // SAFE(2)
      j++;
    }
    i++;
  }

  i_pao_natural_removeLeadingZeroes(out);
  return PAO_status_ok;
  /* SAFE(1): `res` is always non-negative, and `res % PAO_natural_base`
              is strictly less than PAO_natural_base, so the cast to u32 is safe.
     SAFE(2): We need to show `carry` stays below PAO_natural_base across iterations.
              Suppose that the value of `carry` in the previous iteration did not exceed PAO_natural_base.
              Lets refer to `PAO_natural_base` as `B` for shorts.
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
pao_Status pao_natural_distance(pao_Allocator* mem, const pao_Natural* A, const pao_Natural* B, pao_Natural* out) {
  #if PAO_config_debug
    if (mem == NULL || A == NULL || B == NULL || out == NULL) {
      PAO_debug_fatalFmt("Some pointer parameter is null. mem = %p, A = %p, B == %p, out = %p.", (void*)mem, (void*)A, (void*)B, (void*)out);
    }
  #endif
  pao_Status st;

  const pao_Natural* larger;
  const pao_Natural* smaller;
  pao_Order res = pao_natural_compare(A, B);
  if (res == PAO_order_equal) {
    return pao_natural_set(mem, 0, out);
  } else if (res == PAO_order_less) {
    larger = B;
    smaller = A;
  } else {
    larger = A;
    smaller = B;
  }
  if (pao_natural_isZero(smaller)) {
    return pao_natural_copy(mem, larger, out);
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
      digit = (u32)(PAO_natural_base + res); // SAFE(1)
    } else {
      carry = 0;
      digit = (u32)res; // SAFE(2)
    }

    if (i == out->len) {
      st = i_pao_natural_pushDigit(mem, 0, out); PAO_status_check;
    }
    out->digits[i] = digit;

    i++;
  }

  i_pao_natural_removeLeadingZeroes(out);
  return PAO_status_ok;
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
pao_Status pao_natural_distanceDigit(pao_Allocator* mem, const pao_Natural* A, u32 B, pao_Natural* out) {
  #if PAO_config_debug
    if (mem == NULL || A == NULL || out == NULL) {
      PAO_debug_fatalFmt("Some pointer parameter is null. mem = %p, A = %p, out = %p.", (void*)mem, (void*)A, (void*)out);
    }

    if (PAO_natural_base <= B) {
      PAO_debug_fatalFmt("B is not a valid digit. B = %d.", B);
    }
  #endif
  if (pao_natural_isZero(A)) {
    return pao_natural_set(mem, B, out);
  }

  if (A->len == 1) {
    u32 digit = A->digits[0];
    u32 result = 0;
    if (digit < B) {
      result = B - digit;
    } else {
      result = digit - B;
    }
    return pao_natural_set(mem, result, out);
  }

  // B < A, assuming A has no leading zeroes.
  pao_Status st;
  i64 res = 0;
  i64 carry = B;
  u32 i = 0;

  do {
    // SAFE(1):
    res = A->digits[i] - carry;
    if (res < 0) {
      carry = 1;
      res += PAO_natural_base;
    } else {
      carry = 0;
    }

    if (i == out->len) {
      st = i_pao_natural_pushDigit(mem, 0, out); PAO_status_check;
    }
    // SAFE(2):
    out->digits[i] = (u32)res;

    i++;
  } while (carry > 0 || i < A->len);
  i_pao_natural_removeLeadingZeroes(out);
  return PAO_status_ok;
  /* SAFE(1): since |A-B| < A, we can safely use `i` here, we will never
              have a situation where both `carry > 0` and `i >= A.len` are true.
     SAFE(2): see that because `A.digit[i]` and `B` are both positive numbers
              less than `PAO_natural_base`, then `res = A.digits[0] - B` is also less
              than the base, which is less than U32_MAX.
  */
}

// sets `out` to `PAO_order_equal` if `guess*B == idd` or if `(guess+1)*B > idd`
static inline
pao_Status i_pao_natural_testGuess(pao_Allocator* mem, const pao_Natural* idd, const pao_Natural* B, u32 guess, pao_Natural* scratch, pao_Order* out) {
  pao_Status st;
  st = pao_natural_multDigit(mem, B, guess, scratch); PAO_status_check;
  pao_Order res = pao_natural_compare(scratch, idd);
  if (res == PAO_order_equal || res == PAO_order_greater) {
    *out = res;
  } else {
    // NOTE(1)
    st = pao_natural_add(mem, B, scratch, scratch); PAO_status_check;
    res = pao_natural_compare(scratch, idd);
    if (res == PAO_order_greater) {
      *out = PAO_order_equal; // NOTE(2)
    } else {
      *out = PAO_order_less;
    }
  }
  return PAO_status_ok;
  /*  NOTE(1): Here we know `res == PAO_order_less`, so that `guess*B < idd`.
      NOTE(2): (guess+1)*B > IDD.
  */
}

#define I_PAO_natural_divMaxNumGuesses 30

/* Finds `Q` and `R` such that `A = Q*B + R`.
   DRAGONS:
*/
static inline
pao_Status pao_natural_div(pao_Allocator* mem, pao_Natural* scratch,
                         const pao_Natural* A, const pao_Natural* B,
                         pao_Natural* Q, pao_Natural* R) {
  #if PAO_config_debug
    if (mem == NULL || A == NULL || B == NULL || Q == NULL || R == NULL) {
      PAO_debug_fatalFmt("Some pointer parameter is null. mem = %p, A = %p, B = %p, Q = %p, R = %p.", (void*)mem, (void*)A, (void*)B, (void*)Q, (void*)R);
    }

    if (A == Q || A == R || B == Q || B == R) {
      PAO_debug_fatalFmt("Aliasing requirements not met. A = %p, B = %p, Q = %p, R = %p.", (void*)A, (void*)B, (void*)Q, (void*)R);
    }

    u64 maxspan = (u64)1 << I_PAO_natural_divMaxNumGuesses;
    if (maxspan <= PAO_natural_base) {
      PAO_debug_fatalFmt("Maximum number of guesses will not cover the span of PAO_natural_base: PAO_natural_base = %d, span = %ld", PAO_natural_base, maxspan);
    }
  #endif
  pao_Status st;

  if (pao_natural_isZero(B)) {
    return PAO_status_divisionByZero;
  }
  if (pao_natural_isZero(A)) {
    st = pao_natural_set(mem, 0, R); PAO_status_check;
    return pao_natural_set(mem, 0, Q);
  }
  st = pao_natural_set(mem, 0, scratch); PAO_status_check;
  st = pao_natural_set(mem, 0, Q); PAO_status_check;
  st = pao_natural_set(mem, 0, R); PAO_status_check;

  // NOTE(2)
  i64 i = (i64)A->len - 1;
  while (0 <= i) {
    st = pao_natural_multBase(mem, R);                  PAO_status_check;
    st = pao_natural_addDigit(mem, R, A->digits[i], R); PAO_status_check;

    pao_Order ord = pao_natural_compare(R, B);

    if (ord == PAO_order_less) {         // R < B
      if (pao_natural_isZero(Q) == false) {
        st = pao_natural_multBase(mem, Q); PAO_status_check;
      }
    } else if (ord == PAO_order_equal) { // R == B
      st = pao_natural_set(mem, 0, R);         PAO_status_check;
      st = pao_natural_multBase(mem, Q);       PAO_status_check;
      st = pao_natural_addDigit(mem, Q, 1, Q); PAO_status_check;
    } else {                            // R > B
      u32 low = 1; // NOTE(3)
      u32 high = PAO_natural_base;
      u32 guess = (low + high)/2;
      i32 j = 0;

      pao_Order res;
      st = i_pao_natural_testGuess(mem, R, B, guess, scratch, &res); PAO_status_check;

      // NOTE(4)
      while (res != PAO_order_equal && j < I_PAO_natural_divMaxNumGuesses) {
        if (res == PAO_order_less) {
          low = guess;
        } else if (res == PAO_order_greater) {
          high = guess;
        }
        guess = (low + high)/2;
        st = i_pao_natural_testGuess(mem, R, B, guess, scratch, &res); PAO_status_check;
        j++;
      }

      st = pao_natural_multDigit(mem, B, guess, scratch); PAO_status_check;
      st = pao_natural_distance(mem, R, scratch, R); PAO_status_check;
      st = pao_natural_multBase(mem, Q); PAO_status_check;
      st = pao_natural_addDigit(mem, Q, guess, Q); PAO_status_check;
    }
    
    i--;
  }

  return PAO_status_ok;
  /* NOTE(2): We implement naïve long division, following the article at:
                  https://en.wikipedia.org/wiki/Long_division#Algorithm_for_arbitrary_base
     NOTE(3): Since R > B, then `guess` is at least 1.
     NOTE(4): This digit we're searching for is unique and lives in an ordered space (the natural numbers up to BASE),
              so that we can use binary search. There are a few ways to reduce the size
              of this space, namely by finding closer upper and lower bounds. Also, the complexity is log(BASE),
              so the maximum number of iterations is 30. Since the base may change, we set it up to 32, ie,
              log(U32_MAX).
  */
}

/* Finds `Q` and `R` such that `A = Q*B + R`.
   `R` is guaranteed to be less than `B` by the Division Theorem,
   hence, it's a u32.
   DRAGONS:
*/
static inline
pao_Status pao_natural_divDigit(pao_Allocator* mem, const pao_Natural* A, u32 B, pao_Natural* Q, u32* R) {
  #if PAO_config_debug
    if (mem == NULL || A == NULL || Q == NULL || R == NULL) {
      PAO_debug_fatalFmt("Some pointer parameter is null. mem = %p, A = %p, Q = %p, R = %p.", (void*)mem, (void*)A, (void*)Q, (void*)R);
    }

    if (A == Q) {
      PAO_debug_fatalFmt("Aliasing requirements not met. A = %p, out = %p.", (void*)A, (void*)Q);
    }

    if (PAO_natural_base <= B) {
      PAO_debug_fatalFmt("B is not a valid digit. B = %d.", B);
    }
  #endif

  if (B == 0) {
    return PAO_status_divisionByZero;
  }
  if (pao_natural_isZero(A)) {
    *R = 0;
    pao_natural_set(mem, 0, Q);
    return PAO_status_ok;
  }

  pao_Status st = pao_natural_set(mem, 0, Q); PAO_status_check;
  *R = 0;
  i64 i = A->len - 1; // SAFE(3)
  i64 q = 0;
  i64 carry = 0;

  while (0 <= i) {
    i64 idd = (i64)A->digits[i] + carry * PAO_natural_base; // NOTE(1)
    q     = idd/B; // NOTE(3)
    carry = idd%B;  // NOTE(2)

    // TODO: this probably should be pushDigit + reverse
    st = pao_natural_multBase(mem, Q); PAO_status_check;
    st = pao_natural_addDigit(mem, Q, (u32)q, Q); PAO_status_check; // SAFE(2)

    i--;
  }
  i_pao_natural_removeLeadingZeroes(Q);
  *R = (u32)carry; // SAFE(1)
  return PAO_status_ok;
  /*
   NOTE(1): Since `B` is a digit, then `idd` is at most 2 digits.
   This means we can use an `i64` as intermediate dividend.
   NOTE(2): After this line, `carry` will be less than `B`. This follows
   immediately from the Division Theorem: `carry` becomes the intermediate remainder.
   SAFE(1): For the reasons stated above, `carry < B < PAO_natural_base`.
   NOTE(3): At the end of the loop, `carry` is less than `B`, which means an
   upper bound for `idd` at this point is
       (B-1)*PAO_natural_base + PAO_natural_base-1 = 
       B*PAO_natural_base - PAO_natural_base + PAO_natural_base - 1 = 
       B*PAO_natural_base - 1.
   Since `q = floor(idd/B)` then:
       floor(idd/B) <= 
       floor((B*PAO_natural_base - 1) / B) = 
       floor(PAO_natural_base - (1/B)).
   Since `0 < 1/B <= 1` then
       floor(PAO_natural_base - (1/B)) = PAO_natural_base - 1.
   So that `q <= PAO_natural_base - 1`.
   SAFE(2): Because of the reasons stated above, the cast is safe and
   `q` is a valid digit.
   SAFE(3): This subtraction is OK since we checked earlier that
   `A.len` is strictly bigger than zero (pao_natural_isZero)
  */
}

static inline
char* i_pao_natural_firstNonzeroChar(char* buffer, usize buffSize) {
  usize i = 0;
  while (i < buffSize && buffer[i] == '0') {
    i++;
  }
  return buffer+i;
}

static inline
void i_pao_natural_WriteU32(u32 n, char* buffer) {
  int i = 0;
  while (i < PAO_natural_digitsPerInt) {
    buffer[i] = '0';
    i++;
  }
  if (n == 0) {
    return;
  }

  char* b = buffer + PAO_natural_digitsPerInt -1;
  while (n > 0) {
    *b = (char)(n%10) + '0';
    b--;
    n = n/10;
  }
  return;
}

/* Only writes a number if the given buffer has sufficient size.
*/
static inline
size_t i_pao_natural_snprint(const pao_Natural* nat, char* buffer, usize buffSize, bool padLeft, bool padRight) {
  // NOTE(1):
  usize neededBytes = (usize)(nat->len * PAO_natural_digitsPerInt);
  if (buffSize == 0 || neededBytes >= buffSize) {
    return 0;
  }
  if (nat->len == 0) {
    *buffer = '0';
    return 1;
  }

  i64 i = (i64)nat->len -1;
  char* block = buffer;

  do {
    u32 currDigit = nat->digits[i];
    // SAFE(1):
    i_pao_natural_WriteU32(currDigit, block);
    block += PAO_natural_digitsPerInt;
    i--;
  } while (0 <= i);

  usize size = (uptr)block - (uptr)buffer;
  if (!padRight) {
    while (0 < size && *(buffer+size-1) == '0') {
        size--;
    }
  }

  // NOTE(2)
  if (!padLeft) {
    char* firstNonZero = i_pao_natural_firstNonzeroChar(buffer, size);
    if (buffer < firstNonZero) {
      uptr padLen = (uptr)firstNonZero - (uptr)buffer;
      size = size - padLen;

      uptr i = 0;
      while (i < size) {
        buffer[i] = firstNonZero[i];
        i++;
      }
    }
  }

  return size;
  /* NOTE(1): This checks if the buffer size is sufficient, we're generous here and
              don't care if padding is omitted. We also expect that no useless
              digits are present, ie: 000000000_000000001,
              which should be true for all arithmetic implemented here.
              If for some reason some useless digits are present and
              padd_left is false, then this will return 0.
     SAFE(1): We can proceed to write 9 bytes because we already know the buffer has enough space,
              see note(1).
     NOTE(2): If there are zeros on the left and we don't want padding,
              then we shift everything to the left until we get rid of the zeros.
  */
}

/* Only writes a number if the given buffer has sufficient size, ie,
it either fully writes the number or returns 0.
*/
/* TODO: refactor this to use pao_buffer
*/
static inline
usize pao_natural_snprint(const pao_Natural* nat, char* buffer, size_t buffSize) {
  #if PAO_config_debug
    if (nat == NULL || buffer == NULL) {
      PAO_debug_fatalFmt("Some pointer parameter is null. nat = %p, buffer = %p.", (void*)nat, (void*)buffer);
    }
  #endif
  return i_pao_natural_snprint(nat, buffer, buffSize, false, true);
}

#endif
