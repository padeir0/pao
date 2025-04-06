/*
Copyright 2025 Artur Iure Vianna Fernandes

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the “Software”), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/* TODO:
  natural_add,
  natural_distanceDigit, natural_distance,
  natural_multDigit,     natural_mult,
  natural_divDigit,      natural_div
*/

#ifndef PAO_natural_H
#define PAO_natural_H

// remove when finished debugging.
#include <stdio.h>
#include <string.h>

#include "pao_basicTypes.h"
#include "pao_status.h"
#include "pao_allocator.h"

typedef struct {
  u32* digits;
  u32  cap;
  u32  len;
} pao_Natural;

/* BEGIN: CONSTANTS */
#define PAO_NATURAL_minNatVec 4
#define PAO_NATURAL_digitsPerInt 9
#define PAO_NATURAL_base 1000000000
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
  return PAO_NATURAL_base <= digit;
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
bool i_pao_natural_pushDigit(pao_Allocator mem, pao_Natural* out, u32 digit) {
  if (out->cap == 0) {
    out->digits = i_pao_natural_natVecAlloc(mem, PAO_NATURAL_minNatVec, (char*)__func__);
    if (out->digits == NULL) {
      return false;
    }
    out->cap = PAO_NATURAL_minNatVec;
  }
  if (out->len == out->cap) {
    u32 new_cap = 2 * out->cap;
    u32* new_vec = i_pao_natural_natVecAlloc(mem, new_cap, (char*)__func__);
    if (new_vec == NULL) {
      return false;
    }
    i_pao_natural_natVecCopy(new_vec, out->digits, out->len);
    i_pao_natural_natVecFree(mem, out->digits);
    out->digits = new_vec;
    out->cap = new_cap;
  }
  u32 index = out->len;
  out->len++;
  out->digits[index] = digit;
  return true;
}

pao_status pao_natural_setVec(pao_Allocator mem, pao_Natural* out, u32* digits, i32 len) {
  out->len = 0;
  int i = len-1;
  while (0 <= i) {
    bool ok = i_pao_natural_pushDigit(mem, out, digits[i]);
    if (!ok) {
      return PAO_status_outOfMemory;
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
  i_pao_natural_pushDigit(mem, out, digit);
  return PAO_status_ok;
}

bool pao_natural_isZero(const pao_Natural N) {
  return N.len == 0;
}

bool pao_natural_equalDigit(const pao_Natural A, u32 digit) {
  if (A.len == 0 && digit == 0) {
    return true;
  }
  return A.len == 1 && A.digits[0] == digit;
}

bool pao_natural_equal(const pao_Natural A, const pao_Natural B) {
  if (A.len != B.len) {
    return false;
  }
  u32 i = 0;
  while (i < A.len) {
    if (A.digits[i] != B.digits[i]) {
      return false;
    }
    i++;
  }
  return true;
}

pao_status pao_natural_addDigit(pao_Allocator mem, const pao_Natural A, u32 B, pao_Natural* out) {
  if (i_pao_natural_notDigit(B)) {
    return PAO_status_invalidDigit;
  }
  if (pao_natural_isZero(A)) {
    pao_natural_set(mem, out, B);
    return PAO_status_ok;
  }
  if (out->len == 0) {
    i_pao_natural_pushDigit(mem, out, 0);
  }

  u32 i = 0;
  u32 carry = B;
  i64 res = 0;

  do {
    if (i == out->len) {
      i_pao_natural_pushDigit(mem, out, 0);
    }

    res = carry;
    res += A.digits[i];

    if (PAO_NATURAL_base <= res) {
      carry = 1;
      out->digits[i] = (u32)(res - PAO_NATURAL_base);
      /* UNSAFE:
         since our carry is set to the digit in the first iteration,
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
    } else {
      carry = 0;
      out->digits[i] = (u32)res;
    }
    i++;
  } while (0 < carry || i < A.len);

  return PAO_status_ok;
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
  while (i < PAO_NATURAL_digitsPerInt) {
    buffer[i] = '0';
    i++;
  }
  if (n == 0) {
    return;
  }

  char* b = buffer + PAO_NATURAL_digitsPerInt -1;
  while (n > 0) {
    *b = (char)(n%10) + '0';
    b--;
    n = n/10;
  }
  return;
}

static
size_t i_pao_natural_snprint(const pao_Natural nat, char* buffer, usize buffSize, bool padLeft, bool padRight) {
  // Checks if the buffer size is sufficient, we're generous here and
  // don't care if padding is omitted. We also expect that no useless
  // digits are present, ie: 000000000_000000001,
  // which should be true for all arithmetic implemented here.
  // If for some reason some useless digits are present and
  // padd_left is false, then this will return 0.
  usize neededBytes = (usize)(nat.len * PAO_NATURAL_digitsPerInt);
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
    i_pao_natural_WriteU32(currDigit, block);
    block += PAO_NATURAL_digitsPerInt;
    i--;
  } while (0 < i);

  usize size = (uptr)block - (uptr)buffer;
  if (!padRight) {
    while (0 < size && *(buffer+size-1) == '0') {
        size--;
    }
  }

  // if there are zeros on the left and we don't want padding,
  // then we shift everything to the left until we get rid of the zeros.
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
}

usize pao_natural_snprint(const pao_Natural nat, char* buffer, size_t buffSize) {
  return i_pao_natural_snprint(nat, buffer, buffSize, false, true);
}

#endif
