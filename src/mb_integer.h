/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef MB_integer_H
#define MB_integer_H

#include "mb_basicTypes.h"
#include "mb_allocator.h"
#include "mb_status.h"
#include "mb_natural.h"

typedef struct {
  mb_Natural abs;
  i8 sign;
} mb_Integer;

// returns a integer set to zero
mb_Integer mb_integer_new(void) {
  mb_Integer i;
  i.sign = +1;
  i.abs = mb_natural_new();
  return i;
}

void mb_integer_free(mb_Allocator* mem, mb_Integer i) {
  mb_natural_free(mem, i.abs);
}

mb_Status mb_integer_set(mb_Allocator* mem, i32 num, mb_Integer* i) {
  i8 sign;
  u32 digit;
  if (num >= 0) {
    sign = +1;
    digit = (u32)num;
  } else {
    sign = -1;
    digit = (u32)(-num);
  }
  i->sign = sign;
  return mb_natural_set(mem, digit, &i->abs);
}

// TODO: UNTESTED:
mb_Status mb_integer_copy(mb_Allocator* mem, const mb_Integer* A, mb_Integer* out) {
  out->sign = A->sign;
  return mb_natural_copy(mem, &A->abs, &out->abs);
}

mb_Status mb_integer_setVec(mb_Allocator* mem, i8 sign, u32* digits, i32 length, mb_Integer* i) {
  i->sign = sign;
  return mb_natural_setVec(mem, digits, length, &i->abs);
}

bool mb_integer_equal(const mb_Integer* A, const mb_Integer* B) {
  if (mb_natural_isZero(&A->abs) && mb_natural_isZero(&B->abs)) {
    return true;
  }
  return A->sign == B->sign && mb_natural_equal(&A->abs, &B->abs);
}

bool mb_integer_isZero(const mb_Integer* A) {
  return mb_natural_isZero(&A->abs);
}

// TODO: UNTESTED:
mb_Order mb_integer_compare(const mb_Integer* A, const mb_Integer* B) {
  if (mb_natural_isZero(&A->abs) && mb_natural_isZero(&B->abs)) {
    return MB_order_equal;
  }
  if (A->sign == +1 && B->sign == +1) {
    return mb_natural_compare(&A->abs, &B->abs);
  } else if (A->sign == -1 && B->sign == -1) {
    return mb_order_invert(mb_natural_compare(&A->abs, &B->abs));
  } else if (A->sign == -1 && B->sign == +1) {
    return MB_order_less;
  } else if (A->sign == +1 && B->sign == -1) {
    return MB_order_greater;
  }
  // unreachable
  return MB_order_equal;
}

usize mb_integer_snprint(const mb_Integer* A, char* buffer, size_t buffSize) {
  if (buffSize == 0) {
    return 0;
  }
  if (A->sign == -1) {
    buffer[0] = '-';
    return mb_natural_snprint(&A->abs, buffer+1, buffSize-1) + 1;
  }
  return mb_natural_snprint(&A->abs, buffer, buffSize);
}

mb_Status mb_integer_add(mb_Allocator* mem, const mb_Integer* A, const mb_Integer* B, mb_Integer* out) {
  mb_Status st;
  if (A->sign == B->sign) {
    st = mb_natural_add(mem, &A->abs, &B->abs, &out->abs); MB_status_check;
    out->sign = A->sign;
    return MB_status_ok;
  }
  mb_Order res = mb_natural_compare(&A->abs, &B->abs); // NOTE(1)
  st = mb_natural_distance(mem, &A->abs, &B->abs, &out->abs); MB_status_check;

  if (res == MB_order_less) {
    out->sign = B->sign;
  } else if (res == MB_order_greater) {
    out->sign = A->sign;
  } else {
    out->sign = +1;
  }
  return MB_status_ok;
  /* NOTE(1): this line must come before the `distance` computation,
              since `out` may be aliased with either `A` or `B` at
              any time. Meaning their values might change.
  */
}

mb_Status mb_integer_sub(mb_Allocator* mem, const mb_Integer* A, const mb_Integer* B, mb_Integer* out) {
  mb_Status st;
  if (A->sign != B->sign) {
    st = mb_natural_add(mem, &A->abs, &B->abs, &out->abs); MB_status_check;
    out->sign = A->sign;
    return MB_status_ok;
  }
  mb_Order res = mb_natural_compare(&A->abs, &B->abs); // NOTE(1);
  st = mb_natural_distance(mem, &A->abs, &B->abs, &out->abs); MB_status_check;

  if (res == MB_order_greater) {
    out->sign = A->sign;
  } else if (res == MB_order_less) {
    out->sign = A->sign * -1;
  } else {
    out->sign = +1;
  }
  return MB_status_ok;
  /* NOTE(1): this line must come before the `distance` computation,
              since `out` may be aliased with either `A` or `B` at
              any time. Meaning their values might change.
  */
}

mb_Status mb_integer_mult(mb_Allocator* mem, const mb_Integer* A, const mb_Integer* B, mb_Integer* out) {
  mb_Status st = mb_natural_mult(mem, &A->abs, &B->abs, &out->abs); MB_status_check;
  out->sign = A->sign * B->sign;
  if (mb_natural_isZero(&out->abs)) {
    out->sign = +1;
  }
  return MB_status_ok;
}

mb_Status mb_integer_div(mb_Allocator* mem, mb_Natural* scratch, const mb_Integer* A, const mb_Integer* B, mb_Integer* Q, mb_Integer* R) {
  mb_Status st = mb_natural_div(mem, scratch, &A->abs, &B->abs, &Q->abs, &R->abs); MB_status_check;

  Q->sign = A->sign * B->sign;
  if (mb_natural_isZero(&Q->abs)) {
    Q->sign = +1;
  }

  R->sign = A->sign;
  if (mb_natural_isZero(&R->abs)) {
    R->sign = +1;
  }
  return MB_status_ok;
}
#endif
