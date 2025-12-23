/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/


/* TODO:
  natural_add,
  natural_distance,
  natural_mult,
  natural_div
*/

#ifndef PAO_natural_H
#define PAO_natural_H

// remove when finished debugging.
#include <stdio.h>
#include <string.h>

#include "pao_basicTypes.h"
#include "pao_status.h"
#include "pao_allocator.h"
#include "pao_util.h"

#include "pao_debug.h"

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
u32* i_pao_natural_natVecAlloc(pao_Allocator mem, usize size, char* func) {
  return (u32*)mem.alloc(mem.heap, size*sizeof(u32), func);
}

static inline
void i_pao_natural_natVecCopy(u32* dest, u32* source, usize len) {
  memcpy(dest, source, len*sizeof(u32));
}

static inline
void i_pao_natural_natVecFree(pao_Allocator mem, u32* vec) {
  mem.free(mem.heap, vec);
}
/* END: NATVEC */

/* BEGIN: UTIL */

static inline
bool i_pao_natural_notDigit(u32 digit) {
  return PAO_natural_base <= digit;
}

/* END: UTIL */
pao_Natural pao_natural_empty(void) {
  pao_Natural n;
  n.cap = 0;
  n.len = 0;
  n.digits = NULL;
  return n;
}

static
pao_status i_pao_natural_pushDigit(pao_Allocator mem, pao_Natural* out, u32 digit) {
  if (out->cap == 0) {
    out->digits = i_pao_natural_natVecAlloc(mem, PAO_natural_minNatVec, (char*)__func__);
    if (out->digits == NULL) {
      return PAO_status_outOfMemory;
    }
    out->cap = PAO_natural_minNatVec;
  }
  if (out->len == out->cap) {
    u32 new_cap = 2 * out->cap;
    if (new_cap > (u32)I32_MAX) {
      // this limitation is acceptable, a ~8GB number is unrealistic.
      return PAO_status_naturalNumberOverflow;
    }

    u32* new_vec = i_pao_natural_natVecAlloc(mem, new_cap, (char*)__func__);
    if (new_vec == NULL) {
      return PAO_status_outOfMemory;
    }
    i_pao_natural_natVecCopy(new_vec, out->digits, out->len);
    i_pao_natural_natVecFree(mem, out->digits);
    out->digits = new_vec;
    out->cap = new_cap;
  }
  u32 index = out->len;
  out->len++;
  out->digits[index] = digit;
  return PAO_status_ok;
}

pao_status pao_natural_multBase(pao_Allocator mem, pao_Natural* out) { 
  pao_status st = i_pao_natural_pushDigit(mem, out, 0);
  if (st != PAO_status_ok) {
    return st;
  }
  i64 i = out->len - 1;
  while (0 < i) {
    out->digits[i] = out->digits[i-1];
    i--;
  }
  out->digits[0] = 0;
  return PAO_status_ok;
}

pao_status pao_natural_setVec(pao_Allocator mem, pao_Natural* out, u32* digits, i32 len) {
  out->len = 0;
  int i = len-1;
  while (0 <= i) {
    pao_status st = i_pao_natural_pushDigit(mem, out, digits[i]);
    if (st != PAO_status_ok) {
      return st;
    }
    i--;
  }
  return PAO_status_ok;
}

pao_status pao_natural_set(pao_Allocator mem, pao_Natural* out, u32 digit) {
  if (i_pao_natural_notDigit(digit)){
    return PAO_status_invalidDigit;
  }
  if (digit == 0) {
    out->len = 0;
    return PAO_status_ok;
  }
  if (out->cap > 0) {
    out->len = 1;
    out->digits[0] = digit;
    return PAO_status_ok;
  }
  return i_pao_natural_pushDigit(mem, out, digit);
}

static inline
bool pao_natural_isZero(const pao_Natural* N) {
  return N->len == 0;
}

bool pao_natural_equalDigit(const pao_Natural* A, u32 digit) {
  if (A->len == 0 && digit == 0) {
    return true;
  }
  return A->len == 1 && A->digits[0] == digit;
}

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

// A must be different from OUT
pao_status pao_natural_addDigit(pao_Allocator mem, const pao_Natural* A, u32 B, pao_Natural* out) {
  if (i_pao_natural_notDigit(B)) {
    return PAO_status_invalidDigit;
  }
  if (pao_natural_isZero(A)) {
    return pao_natural_set(mem, out, B);
  }

  u32 i = 0;
  u32 carry = B;
  i64 res = 0;
  i64 startingLen = A->len; // NOTE(1)

  do {
    res = carry;
    if (i < startingLen) {
      res += A->digits[i];
    }

    if (PAO_natural_base <= res) {
      carry = 1;
      res -= PAO_natural_base;
    } else {
      carry = 0;
    }

    // SAFE(1):
    pao_status st = i_pao_natural_pushDigit(mem, out, (u32)res);
    if (st != PAO_status_ok) {
      return st;
    }
    i++;
  } while (0 < carry || i < startingLen);

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
     NOTE(1): We need this line since `A` might be equals `out`,
     then `pushDigit` will alter `A->len` and the loop will be infinite.
  */
}

pao_status pao_natural_multDigit(pao_Allocator mem, const pao_Natural* A, u32 B, pao_Natural* out) {
  if (i_pao_natural_notDigit(B)) {
    return PAO_status_invalidDigit;
  }
  if (pao_natural_isZero(A) || B == 0) {
    return pao_natural_set(mem, out, 0);
  }
  out->len = pao_util_minU32(out->len, A->len);

  u32 i = 0;
  u32 carry = 0;

  while (i < A->len || carry > 0) {
    if (i == out->len) {
      pao_status st = i_pao_natural_pushDigit(mem, out, 0);
      if (st != PAO_status_ok) {
        return st;
      }
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
     SAFE(2): `res` is always positive and PAO_natural_base is a large
     number. Let us find an upper bound for `res`.
     Suppose `A.digit[i]` and `B` are the largest they can be, ie,
     `PAO_natural_base-1`, and `carry` starts at `0`. Then `res`
     is `(PAO_natural_base-1)^2` so that:
         (PAO_natural_base-1)^2 < (PAO_natural_base)^2
     When we divide `res / PAO_natural_base`, we get that `carry`
     is less than `PAO_natural_base`. On the next iteration,
     if both `A.digit[i]` and `B` are the largest they can be again,
     an upper bound for `res` is:
         (PAO_natural_base-1)^2 + PAO_natural_base
     So that:
         (PAO_natural_base-1)^2 + PAO_natural_base                 < 
         (PAO_natural_base)(PAO_natural_base-1) + PAO_natural_base = 
         (PAO_natural_base)(1 + PAO_natural_base-1)                = 
         (PAO_natural_base)^2.
     Which means `carry` is still less than `PAO_natural_base`.
     By induction, it can be proven that this is always the case,
     hence, the cast is safe.
     UNSAFE(3): If this overflows, our number has more than U32_MAX digits,
     which is to say we have a 16GB number...
  */
}

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

/* Finds `Q` and `R` such that `A = Q*B + R`.
   `R` is guaranteed to be less than `B` by the Division Theorem,
   hence, it's a u32.
   UNTESTED: TODO:
   DRAGONS:
*/
pao_status pao_natural_divDigit(pao_Allocator mem, const pao_Natural* A, u32 B, pao_Natural* Q, u32* R) {
  if (B == 0) {
    return PAO_status_divisionByZero;
  }
  if (pao_natural_isZero(A)) {
    *R = 0;
    pao_natural_set(mem, Q, 0);
    return PAO_status_ok;
  }

  i64 idd = 0; // NOTE(1)
  i64 i = A->len - 1; // SAFE(3)
  i64 q = 0;

  while (0 <= i) {
    idd *= PAO_natural_base;
    idd += A->digits[i];
    q = idd / B; // NOTE(3)
    idd -= q*B;  // NOTE(2)

    pao_status st = i_pao_natural_pushDigit(mem, Q, (u32)q); // SAFE(2):
    if (st != PAO_status_ok) {
      return st;
    }

    i--;
  }
  i_pao_natural_removeLeadingZeroes(Q);
  *R = (u32)idd; // SAFE(1):
  return PAO_status_ok;
  /*
   NOTE(1): Since `B` is a digit, then `idd` is at most 2 digits.
   This means we can use an `i64` as intermediate dividend.
   NOTE(2): After this line, `idd` will be less than `B`. This follows
   immediately from the Division Theorem: `idd` becomes the intermediate remainder.
   SAFE(1): For the reasons stated above, `idd < B < PAO_natural_base`.
   NOTE(3): At the end of the loop, `idd` is less than `B`, which means an
   upper bound for `idd` at this point is
       (B-1)*PAO_natural_base + PAO_natural_base-1 = 
       B*PAO_natural_base - PAO_natural_base + PAO_natural_base - 1 = 
       B*PAO_natural_base - 1.
   Since `q = floor(idd/B)` then:
       floor(idd/B) < 
       floor((B*PAO_natural_base - 1) / B) = 
       floor(PAO_natural_base - (1/B)).
   Since `0 < 1/B <= 1` then
       floor(PAO_natural_base - (1/B)) = PAO_natural_base - 1.
   So that `q < PAO_natural_base - 1`.
   SAFE(2): Because of the reasons stated above, the cast is safe and
   `q` is a valid digit.
   SAFE(3): This subtraction is OK since we checked earlier that
   `A.len` is strictly bigger than zero (pao_natural_isZero)
  */
}

/*
Computes |A - B|, in other words:
  if B<A then A-B
  else B-A
*/
pao_status pao_natural_distanceDigit(pao_Allocator mem, const pao_Natural* A, u32 B, pao_Natural* out) {
  if (pao_natural_isZero(A)) {
    return pao_natural_set(mem, out, B);
  }

  if (A->len == 1) {
    u32 digit = A->digits[0];
    u32 result = 0;
    if (digit < B) {
      result = B - digit;
    } else {
      result = digit - B;
    }
    return pao_natural_set(mem, out, result);
  }

  // B < A, assuming A has no leading zeroes.
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
    // SAFE(2):
    pao_status st = i_pao_natural_pushDigit(mem, out, (u32)res);
    if (st != PAO_status_ok) {
      return st;
    }
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

static
char* i_pao_natural_firstNonzeroChar(char* buffer, usize buffSize) {
  usize i = 0;
  while (i < buffSize && buffer[i] == '0') {
    i++;
  }
  return buffer+i;
}

static
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
static
size_t i_pao_natural_snprint(const pao_Natural nat, char* buffer, usize buffSize, bool padLeft, bool padRight) {
  // NOTE(1):
  usize neededBytes = (usize)(nat.len * PAO_natural_digitsPerInt);
  if (buffSize == 0 || neededBytes >= buffSize) {
    return 0;
  }
  if (nat.len == 0) {
    *buffer = '0';
    return 1;
  }

  i64 i = (i64)nat.len -1;
  char* block = buffer;

  do {
    u32 currDigit = nat.digits[i];
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
usize pao_natural_snprint(const pao_Natural nat, char* buffer, size_t buffSize) {
  return i_pao_natural_snprint(nat, buffer, buffSize, false, true);
}

#endif
