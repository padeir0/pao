/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_INTEGER_H
#define PAO_INTEGER_H

#include "../basicTypes.h"
#include "../iallocator.h"
#include "../status.h"
#include "natural.h"

typedef struct {
  Natural abs;
  i8 sign;
} Integer;

// returns a integer set to zero
static inline
Integer integer_new(void) {
  Integer i;
  i.sign = +1;
  i.abs = natural_new();
  return i;
}

/* This is a version of `integer_new` that accepts a 
 * preallocated buffer. */
static inline
Integer integer_create(u32* digits, u32 size) {
  Integer i;
  i.sign = +1;
  i.abs = natural_create(digits, size);
  return i;
}

static inline
void integer_free(IAllocator* mem, Integer i) {
  natural_free(mem, i.abs);
}

static inline
Status integer_set(IAllocator* mem, i32 num, Integer* out) {
  #if config_DEBUG
    if (mem == NULL || out == NULL) {
      debug_FATALFMT("Some pointer parameter is null. mem = %p, out = %p.", (void*)mem, (void*)out);
    }
    if (natural_BASE <= util_absI32(num)) {
      debug_FATALFMT("`num` does not fit into a single bignum digit. num = %d.", num);
    }
  #endif
  i8 sign;
  u32 digit;
  if (num >= 0) {
    sign = +1;
    digit = (u32)num;
  } else {
    sign = -1;
    digit = (u32)(-num);
  }
  out->sign = sign;
  return natural_set(mem, digit, &out->abs);
}

static inline
Status integer_copy(IAllocator* mem, const Integer* A, Integer* out) {
  out->sign = A->sign;
  return natural_copy(mem, &A->abs, &out->abs);
}

static inline
Status integer_setVec(IAllocator* mem, i8 sign, u32* digits, i32 length, Integer* i) {
  i->sign = sign;
  return natural_setVec(mem, digits, length, &i->abs);
}

static inline
bool integer_equal(const Integer* A, const Integer* B) {
  if (natural_isZero(&A->abs) && natural_isZero(&B->abs)) {
    return true;
  }
  return A->sign == B->sign && natural_equal(&A->abs, &B->abs);
}

static inline
bool integer_isZero(const Integer* A) {
  return natural_isZero(&A->abs);
}

static inline
Order integer_compare(const Integer* A, const Integer* B) {
  if (natural_isZero(&A->abs) && natural_isZero(&B->abs)) {
    return order_EQUAL;
  }
  if (A->sign == +1 && B->sign == +1) {
    return natural_compare(&A->abs, &B->abs);
  } else if (A->sign == -1 && B->sign == -1) {
    return order_invert(natural_compare(&A->abs, &B->abs));
  } else if (A->sign == -1 && B->sign == +1) {
    return order_LESS;
  } else if (A->sign == +1 && B->sign == -1) {
    return order_GREATER;
  }
  // unreachable
  return order_EQUAL;
}

static inline
usize integer_snprint(const Integer* A, char* buffer, usize buffSize) {
  if (buffSize == 0) {
    return 0;
  }
  if (A->sign == -1) {
    buffer[0] = '-';
    return natural_snprint(&A->abs, buffer+1, buffSize-1) + 1;
  }
  return natural_snprint(&A->abs, buffer, buffSize);
}

static inline
Status integer_add(IAllocator* mem, const Integer* A, const Integer* B, Integer* out) {
  Status st;
  if (A->sign == B->sign) {
    st = natural_add(mem, &A->abs, &B->abs, &out->abs); status_CHECK;
    out->sign = A->sign;
    return status_OK;
  }
  Order res = natural_compare(&A->abs, &B->abs); // NOTE(1)
  st = natural_distance(mem, &A->abs, &B->abs, &out->abs); status_CHECK;

  if (res == order_LESS) {
    out->sign = B->sign;
  } else if (res == order_GREATER) {
    out->sign = A->sign;
  } else {
    out->sign = +1;
  }
  return status_OK;
  /* NOTE(1): this line must come before the `distance` computation,
              since `out` may be aliased with either `A` or `B` at
              any time. Meaning their values might change.
  */
}

static inline
Status integer_sub(IAllocator* mem, const Integer* A, const Integer* B, Integer* out) {
  Status st;
  if (A->sign != B->sign) {
    st = natural_add(mem, &A->abs, &B->abs, &out->abs); status_CHECK;
    out->sign = A->sign;
    return status_OK;
  }
  Order res = natural_compare(&A->abs, &B->abs); // NOTE(1);
  st = natural_distance(mem, &A->abs, &B->abs, &out->abs); status_CHECK;

  if (res == order_GREATER) {
    out->sign = A->sign;
  } else if (res == order_LESS) {
    out->sign = A->sign * -1;
  } else {
    out->sign = +1;
  }
  return status_OK;
  /* NOTE(1): this line must come before the `distance` computation,
              since `out` may be aliased with either `A` or `B` at
              any time. Meaning their values might change.
  */
}

static inline
Status integer_mult(IAllocator* mem, const Integer* A, const Integer* B, Integer* out) {
  Status st = natural_mult(mem, &A->abs, &B->abs, &out->abs); status_CHECK;
  out->sign = A->sign * B->sign;
  if (natural_isZero(&out->abs)) {
    out->sign = +1;
  }
  return status_OK;
}

static inline
Status integer_div(IAllocator* mem, Natural* scratch, const Integer* A, const Integer* B, Integer* Q, Integer* R) {
  Status st = natural_div(mem, scratch, &A->abs, &B->abs, &Q->abs, &R->abs); status_CHECK;

  Q->sign = A->sign * B->sign;
  if (natural_isZero(&Q->abs)) {
    Q->sign = +1;
  }

  R->sign = A->sign;
  if (natural_isZero(&R->abs)) {
    R->sign = +1;
  }
  return status_OK;
}
#endif
