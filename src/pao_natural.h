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
  natural_distance_digit, natural_distance,
  natural_mult_digit,     natural_mult,
  natural_div_digit,      natural_div
*/

#ifndef PAO_NATURAL_H
#define PAO_NATURAL_H

// remove when finished debugging.
#include <stdio.h>
#include <string.h>

#include "pao_basic_types.h"
#include "pao_status.h"
#include "pao_allocator.h"

typedef struct {
  u32* digits;
  i32  cap;
  i32  len;
} pao_Natural;

/* BEGIN: CONSTANTS */
#define PAO_NATURAL_MIN_NAT_VEC 4
#define PAO_NATURAL_DIGITS_PER_INT 9
#define PAO_NATURAL_BASE 1000000000
/* END: CONSTANTS */

/* BEGIN: NATVEC */
static inline
u32* _pao_natural_natvec_alloc(pao_Allocator mem, int size) {
  return (u32*)mem.alloc(mem.heap, size*sizeof(u32));
}

static inline
void _pao_natural_natvec_copy(u32* dest, u32* source, int len) {
  memcpy(dest, source, len*sizeof(u32));
}

static inline
void _pao_natural_natvec_free(pao_Allocator mem, u32* vec) {
  mem.free(mem.heap, vec);
}
/* END: NATVEC */

/* BEGIN: UTIL */

static inline
bool _pao_natural_not_digit(u32 digit) {
  return PAO_NATURAL_BASE <= digit;
}

/* END: UTIL */
pao_Natural pao_natural_empty() {
  pao_Natural n;
  n.cap = 0;
  n.len = 0;
  n.digits = NULL;
  return n;
}

static
bool _pao_natural_push_digit(pao_Allocator mem, pao_Natural* out, u32 digit) {
  if (out->cap == 0) {
    out->digits = _pao_natural_natvec_alloc(mem, PAO_NATURAL_MIN_NAT_VEC);
    if (out->digits == NULL) {
      return false;
    }
    out->cap = PAO_NATURAL_MIN_NAT_VEC;
  }
  if (out->len == out->cap) {
    i32 new_cap = 2 * out->cap;
    u32* new_vec = _pao_natural_natvec_alloc(mem, new_cap);
    if (new_vec == NULL) {
      return false;
    }
    _pao_natural_natvec_copy(new_vec, out->digits, out->len);
    _pao_natural_natvec_free(mem, out->digits);
    out->digits = new_vec;
    out->cap = new_cap;
  }
  int index = out->len;
  out->len++;
  out->digits[index] = digit;
  return true;
}

pao_status pao_natural_set_vec(pao_Allocator mem, pao_Natural* out, u32* digits, i32 len) {
  out->len = 0;
  int i = len-1;
  while (0 <= i) {
    bool ok = _pao_natural_push_digit(mem, out, digits[i]);
    if (!ok) {
      return pao_status_OUT_OF_MEMORY;
    }
    i--;
  }
  return pao_status_OK;
}

pao_status pao_natural_set(pao_Allocator mem, pao_Natural* out, u32 digit) {
  if (_pao_natural_not_digit(digit)){
    return pao_status_INVALID_DIGIT;
  }
  if (digit == 0) {
    out->len = 0;
    return pao_status_OK;
  }
  if (out->cap > 0) {
    out->len = 1;
    out->digits[0] = digit;
    return pao_status_OK;
  }
  _pao_natural_push_digit(mem, out, digit);
  return pao_status_OK;
}

bool pao_natural_is_zero(const pao_Natural N) {
  return N.len == 0;
}

bool pao_natural_equal_digit(const pao_Natural A, u32 digit) {
  if (A.len == 0 && digit == 0) {
    return true;
  }
  return A.len == 1 && A.digits[0] == digit;
}

bool pao_natural_equal(const pao_Natural A, const pao_Natural B) {
  if (A.len != B.len) {
    return false;
  }
  i32 i = 0;
  while (i < A.len) {
    if (A.digits[i] != B.digits[i]) {
      return false;
    }
    i++;
  }
  return true;
}

pao_status pao_natural_add_digit(pao_Allocator mem, const pao_Natural A, u32 B, pao_Natural* out) {
  if (_pao_natural_not_digit(B)) {
    return pao_status_INVALID_DIGIT;
  }
  if (pao_natural_is_zero(A)) {
    pao_natural_set(mem, out, B);
    return pao_status_OK;
  }
  if (out->len == 0) {
    _pao_natural_push_digit(mem, out, 0);
  }

  int i = 0;
  u32 carry = B;
  i64 res = 0;

  do {
    if (i == out->len) {
      _pao_natural_push_digit(mem, out, 0);
    }

    res = carry;
    res += A.digits[i];

    if (PAO_NATURAL_BASE <= res) {
      carry = 1;
      out->digits[i] = res - PAO_NATURAL_BASE;
      /* since our carry is set to the digit in the first iteration,
         we need to prove the operation above is valid:

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
      out->digits[i] = res;
    }
    i++;
  } while (0 < carry || i < A.len);

  return pao_status_OK;
}

static
char* _pao_natural_first_nonzero_char(char* buffer, usize bufflen) {
  usize i = 0;
  while (i < bufflen && buffer[i] == '0') {
    i++;
  }
  return buffer+i;
}

static
int _pao_natural_write_u32(u32 n, char* buffer) {
  int i = 0;
  while (i < PAO_NATURAL_DIGITS_PER_INT) {
    buffer[i] = '0';
    i++;
  }
  if (n == 0) {
    return PAO_NATURAL_DIGITS_PER_INT;
  }

  char* b = buffer + PAO_NATURAL_DIGITS_PER_INT -1;
  while (n > 0) {
    *b = (char)(n%10) + '0';
    b--;
    n = n/10;
  }

  int distance = (uptr)b - (uptr)buffer;
  return 9 - distance;
}

static
size_t _pao_natural_snprint(const pao_Natural nat, char* buffer, usize bufflen, bool pad_left, bool pad_right) {
  // Checks if the buffer size is sufficient, we're generous here and
  // don't care if padding is omitted. We also expect that no useless
  // digits are present, ie: 000000000_000000001,
  // which should be true for all arithmetic implemented here.
  // If for some reason some useless digits are present and
  // padd_left is false, then this will return 0.
  usize needed_bytes = (usize)(nat.len * PAO_NATURAL_DIGITS_PER_INT);
  if (bufflen == 0 || needed_bytes >= bufflen) {
    return 0;
  }
  if (nat.len == 0) {
    *buffer = '0';
    return 1;
  }

  int i = nat.len -1;
  char* block = buffer;

  while (0 <= i) {
    u32 curr_digit = nat.digits[i];
    _pao_natural_write_u32(curr_digit, block);
    block += PAO_NATURAL_DIGITS_PER_INT;
    i--;
  }

  usize size = (uptr)block - (uptr)buffer;
  if (!pad_right) {
    while (0 < size && *(buffer+size-1) == '0') {
        size--;
    }
  }

  // if there are zeros on the left and we don't want padding,
  // then we shift everything to the left until we get rid of the zeros.
  if (!pad_left) {
    char* first_nonzero = _pao_natural_first_nonzero_char(buffer, size);
    if (buffer < first_nonzero) {
      uptr pad_len = (uptr)first_nonzero - (uptr)buffer;
      size = size - pad_len;

      uptr i = 0;
      while (i < size) {
        buffer[i] = first_nonzero[i];
        i++;
      }
    }
  }

  return size;
}

usize pao_natural_snprint(const pao_Natural nat, char* buffer, size_t bufflen) {
  return _pao_natural_snprint(nat, buffer, bufflen, false, true);
}

#endif /* PAO_NATURAL_H */
