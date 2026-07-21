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
  i_natural_natVecFree(mem, n.digits);
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

static inline
Status natural_multBase(IAllocator* mem, Natural* out) { 
  // TODO: OPT: if pushDigit has to allocate, then we can shift everything while copying. Saves O(n) operations.
  Status st = i_natural_pushDigit(mem, 0, out); status_CHECK;

  i64 i = out->len - 1;
  while (0 < i) {
    out->digits[i] = out->digits[i-1];
    i--;
  }
  out->digits[0] = 0;
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
  // TODO: OPT: this should only allocate A->len+pad to next power of two, not A->cap, potentially wastes memory.
  if (out->cap < A->len) {
    u32* outVec = i_natural_natVecAlloc(mem, A->cap, (char*)__func__);
    if (outVec == NULL) {
      return status_OUTOFMEMORY;
    }
    i_natural_natVecFree(mem, out->digits);
    out->digits = outVec;
    out->cap = A->cap;
  }
  i_natural_natVecCopy(A->digits, A->len, out->digits);
  out->len = A->len;
  return status_OK;
}

static inline
bool natural_equalDigit(const Natural* A, u32 digit) {
  if (A->len == 0 && digit == 0) {
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
  if (b == 0 && A->len == 0) {
    return order_EQUAL;
  }
  if (b != 0 && A->len == 0) {
    return order_LESS;
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

// sets `out` to `order_EQUAL` if `guess*B == idd` or if `(guess+1)*B > idd`
static inline
Status i_natural_testGuess(IAllocator* mem, const Natural* idd, const Natural* B, u32 guess, Natural* scratch, Order* out) {
  Status st;
  st = natural_multDigit(mem, B, guess, scratch); status_CHECK;
  Order res = natural_compare(scratch, idd);
  if (res == order_EQUAL || res == order_GREATER) {
    *out = res;
  } else {
    // NOTE(1)
    st = natural_add(mem, B, scratch, scratch); status_CHECK;
    res = natural_compare(scratch, idd);
    if (res == order_GREATER) {
      *out = order_EQUAL; // NOTE(2)
    } else {
      *out = order_LESS;
    }
  }
  return status_OK;
  /*  NOTE(1): Here we know `res == order_LESS`, so that `guess*B < idd`.
      NOTE(2): (guess+1)*B > IDD.
  */
}

#define i_natural_DIVMAXNUMGUESSES 30

/* Finds `Q` and `R` such that `A = Q*B + R`.
   DRAGONS:
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

    u64 maxspan = (u64)1 << i_natural_DIVMAXNUMGUESSES;
    if (maxspan <= natural_BASE) {
      debug_FATALFMT("Maximum number of guesses will not cover the span of natural_BASE: natural_BASE = %d, span = %ld", natural_BASE, maxspan);
    }
  #endif
  Status st;

  if (natural_isZero(B)) {
    return status_DIVISIONBYZERO;
  }
  if (natural_isZero(A)) {
    st = natural_set(mem, 0, R); status_CHECK;
    return natural_set(mem, 0, Q);
  }
  st = natural_set(mem, 0, scratch); status_CHECK;
  st = natural_set(mem, 0, Q); status_CHECK;
  st = natural_set(mem, 0, R); status_CHECK;

  // NOTE(2)
  i64 i = (i64)A->len - 1;
  while (0 <= i) {
    st = natural_multBase(mem, R);                  status_CHECK;
    st = natural_addDigit(mem, R, A->digits[i], R); status_CHECK;

    Order ord = natural_compare(R, B);

    if (ord == order_LESS) {         // R < B
      if (natural_isZero(Q) == false) {
        st = natural_multBase(mem, Q); status_CHECK;
      }
    } else if (ord == order_EQUAL) { // R == B
      st = natural_set(mem, 0, R);         status_CHECK;
      st = natural_multBase(mem, Q);       status_CHECK;
      st = natural_addDigit(mem, Q, 1, Q); status_CHECK;
    } else {                            // R > B
      u32 low = 1; // NOTE(3)
      u32 high = natural_BASE;
      u32 guess = (low + high)/2;
      i32 j = 0;

      Order res;
      st = i_natural_testGuess(mem, R, B, guess, scratch, &res); status_CHECK;

      // NOTE(4)
      while (res != order_EQUAL && j < i_natural_DIVMAXNUMGUESSES) {
        if (res == order_LESS) {
          low = guess;
        } else if (res == order_GREATER) {
          high = guess;
        }
        guess = (low + high)/2;
        st = i_natural_testGuess(mem, R, B, guess, scratch, &res); status_CHECK;
        j++;
      }

      st = natural_multDigit(mem, B, guess, scratch); status_CHECK;
      st = natural_distance(mem, R, scratch, R); status_CHECK;
      st = natural_multBase(mem, Q); status_CHECK;
      st = natural_addDigit(mem, Q, guess, Q); status_CHECK;
    }
    
    i--;
  }

  return status_OK;
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
  i64 i = A->len - 1; // SAFE(3)
  i64 q = 0;
  i64 carry = 0;

  while (0 <= i) {
    i64 idd = (i64)A->digits[i] + carry * natural_BASE; // NOTE(1)
    q     = idd/B; // NOTE(3)
    carry = idd%B;  // NOTE(2)

    // TODO: this probably should be pushDigit + reverse
    st = natural_multBase(mem, Q); status_CHECK;
    st = natural_addDigit(mem, Q, (u32)q, Q); status_CHECK; // SAFE(2)

    i--;
  }
  i_natural_removeLeadingZeroes(Q);
  *R = (u32)carry; // SAFE(1)
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

/* Only writes a number if the given buffer has sufficient size.

OBS: `padRight = false` is used with decimal representation of numbers,
     so that leading zeroes are removed.
     `padLeft = true` is also used with decimal representation, so that
     if the digits [1, 1] are behind the period, then the number will read
     `.1000000001` and not `0.11` incorrectly.
*/
static inline
usize i_natural_snprint(const Natural* nat, char* buffer, usize buffSize, bool padLeft, bool padRight) {
  // NOTE(1):
  usize neededBytes = (usize)(nat->len * natural_DIGITSPERINT);
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
    i_natural_WriteU32(currDigit, block);
    block += natural_DIGITSPERINT;
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
    char* firstNonzero = i_natural_firstNonzeroChar(buffer, size);
    if (buffer < firstNonzero) {
      uptr padLen = (uptr)firstNonzero - (uptr)buffer;
      size = size - padLen;

      uptr i = 0;
      while (i < size) {
        buffer[i] = firstNonzero[i];
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

/* TODO: refactor this to use Buffer */
/* Only writes a number if the given buffer has sufficient size, ie,
it either fully writes the number or returns 0.
*/
static inline
usize natural_snprint(const Natural* nat, char* buffer, usize buffSize) {
  #if config_DEBUG
    if (nat == NULL || buffer == NULL) {
      debug_FATALFMT("Some pointer parameter is null. nat = %p, buffer = %p.", (void*)nat, (void*)buffer);
    }
  #endif
  return i_natural_snprint(nat, buffer, buffSize, false, true);
}

#endif
