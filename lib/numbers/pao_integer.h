/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_integer_H
#define PAO_integer_H

#include "../pao_basicTypes.h"
#include "../pao_allocator.h"
#include "../pao_status.h"
#include "pao_natural.h"

typedef struct {
  pao_Natural abs;
  i8 sign;
} pao_Integer;

// returns a integer set to zero
static inline
pao_Integer pao_integer_new(void) {
  pao_Integer i;
  i.sign = +1;
  i.abs = pao_natural_new();
  return i;
}

static inline
void pao_integer_free(pao_Allocator* mem, pao_Integer i) {
  pao_natural_free(mem, i.abs);
}

static inline
pao_Status pao_integer_set(pao_Allocator* mem, i32 num, pao_Integer* out) {
  #if PAO_config_debug
    if (mem == NULL || out == NULL) {
      PAO_debug_fatalFmt("Some pointer parameter is null. mem = %p, out = %p.", (void*)mem, (void*)out);
    }
    if (PAO_natural_base <= pao_util_absI32(num)) {
      PAO_debug_fatalFmt("`num` does not fit into a single bignum digit. num = %d.", num);
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
  return pao_natural_set(mem, digit, &out->abs);
}

static inline
pao_Status pao_integer_copy(pao_Allocator* mem, const pao_Integer* A, pao_Integer* out) {
  out->sign = A->sign;
  return pao_natural_copy(mem, &A->abs, &out->abs);
}

static inline
pao_Status pao_integer_setVec(pao_Allocator* mem, i8 sign, u32* digits, i32 length, pao_Integer* i) {
  i->sign = sign;
  return pao_natural_setVec(mem, digits, length, &i->abs);
}

static inline
bool pao_integer_equal(const pao_Integer* A, const pao_Integer* B) {
  if (pao_natural_isZero(&A->abs) && pao_natural_isZero(&B->abs)) {
    return true;
  }
  return A->sign == B->sign && pao_natural_equal(&A->abs, &B->abs);
}

static inline
bool pao_integer_isZero(const pao_Integer* A) {
  return pao_natural_isZero(&A->abs);
}

static inline
pao_Order pao_integer_compare(const pao_Integer* A, const pao_Integer* B) {
  if (pao_natural_isZero(&A->abs) && pao_natural_isZero(&B->abs)) {
    return PAO_order_equal;
  }
  if (A->sign == +1 && B->sign == +1) {
    return pao_natural_compare(&A->abs, &B->abs);
  } else if (A->sign == -1 && B->sign == -1) {
    return pao_order_invert(pao_natural_compare(&A->abs, &B->abs));
  } else if (A->sign == -1 && B->sign == +1) {
    return PAO_order_less;
  } else if (A->sign == +1 && B->sign == -1) {
    return PAO_order_greater;
  }
  // unreachable
  return PAO_order_equal;
}

static inline
usize pao_integer_snprint(const pao_Integer* A, char* buffer, usize buffSize) {
  if (buffSize == 0) {
    return 0;
  }
  if (A->sign == -1) {
    buffer[0] = '-';
    return pao_natural_snprint(&A->abs, buffer+1, buffSize-1) + 1;
  }
  return pao_natural_snprint(&A->abs, buffer, buffSize);
}

static inline
pao_Status pao_integer_add(pao_Allocator* mem, const pao_Integer* A, const pao_Integer* B, pao_Integer* out) {
  pao_Status st;
  if (A->sign == B->sign) {
    st = pao_natural_add(mem, &A->abs, &B->abs, &out->abs); PAO_status_check;
    out->sign = A->sign;
    return PAO_status_ok;
  }
  pao_Order res = pao_natural_compare(&A->abs, &B->abs); // NOTE(1)
  st = pao_natural_distance(mem, &A->abs, &B->abs, &out->abs); PAO_status_check;

  if (res == PAO_order_less) {
    out->sign = B->sign;
  } else if (res == PAO_order_greater) {
    out->sign = A->sign;
  } else {
    out->sign = +1;
  }
  return PAO_status_ok;
  /* NOTE(1): this line must come before the `distance` computation,
              since `out` may be aliased with either `A` or `B` at
              any time. Meaning their values might change.
  */
}

static inline
pao_Status pao_integer_sub(pao_Allocator* mem, const pao_Integer* A, const pao_Integer* B, pao_Integer* out) {
  pao_Status st;
  if (A->sign != B->sign) {
    st = pao_natural_add(mem, &A->abs, &B->abs, &out->abs); PAO_status_check;
    out->sign = A->sign;
    return PAO_status_ok;
  }
  pao_Order res = pao_natural_compare(&A->abs, &B->abs); // NOTE(1);
  st = pao_natural_distance(mem, &A->abs, &B->abs, &out->abs); PAO_status_check;

  if (res == PAO_order_greater) {
    out->sign = A->sign;
  } else if (res == PAO_order_less) {
    out->sign = A->sign * -1;
  } else {
    out->sign = +1;
  }
  return PAO_status_ok;
  /* NOTE(1): this line must come before the `distance` computation,
              since `out` may be aliased with either `A` or `B` at
              any time. Meaning their values might change.
  */
}

static inline
pao_Status pao_integer_mult(pao_Allocator* mem, const pao_Integer* A, const pao_Integer* B, pao_Integer* out) {
  pao_Status st = pao_natural_mult(mem, &A->abs, &B->abs, &out->abs); PAO_status_check;
  out->sign = A->sign * B->sign;
  if (pao_natural_isZero(&out->abs)) {
    out->sign = +1;
  }
  return PAO_status_ok;
}

static inline
pao_Status pao_integer_div(pao_Allocator* mem, pao_Natural* scratch, const pao_Integer* A, const pao_Integer* B, pao_Integer* Q, pao_Integer* R) {
  pao_Status st = pao_natural_div(mem, scratch, &A->abs, &B->abs, &Q->abs, &R->abs); PAO_status_check;

  Q->sign = A->sign * B->sign;
  if (pao_natural_isZero(&Q->abs)) {
    Q->sign = +1;
  }

  R->sign = A->sign;
  if (pao_natural_isZero(&R->abs)) {
    R->sign = +1;
  }
  return PAO_status_ok;
}
#endif
