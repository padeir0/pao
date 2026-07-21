/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_RATIONAL_H
#define PAO_RATIONAL_H

#include "../basicTypes.h"
#include "natural.h"

typedef struct {
  Natural numerator;
  Natural denominator;
  i8 sign;
} Rational;

// CREATES A +0/0 RATIONAL! It must be properly set later.
static inline
Rational rational_new(void) {
  Rational rat;
  rat.sign = +1;
  rat.numerator = natural_new();
  rat.denominator = natural_new();
  return rat;
}

static inline
Rational rational_create(i8 sign,
                         u32* num_digits, u32 num_len,
                         u32* den_digits, u32 den_len) {
  Rational rat;
  rat.sign = sign;
  rat.numerator = natural_create(num_digits, num_len);
  rat.denominator = natural_create(den_digits, den_len);
  return rat;
}

static inline
void rational_free(IAllocator* mem, Rational rat) {
  natural_free(mem, rat.numerator);
  natural_free(mem, rat.denominator);
}

static inline
Status rational_set(IAllocator* mem, i8 sign, u32 num, u32 den, Rational* out) {
  out->sign = sign;
  Status st = natural_set(mem, num, &out->numerator); status_CHECK;
  st = natural_set(mem, den, &out->denominator);      status_CHECK;
  return status_OK;
}

static inline
Status rational_setVec(IAllocator* mem, i8 sign,
                       u32* num_digits, i32 num_len,
                       u32* den_digits, i32 den_len,
                       Rational* out) {
  Status st;
  out->sign = sign;
  st = natural_setVec(mem, num_digits, num_len, &out->numerator);   status_CHECK;
  st = natural_setVec(mem, den_digits, den_len, &out->denominator); status_CHECK;
  return status_OK;
}

static inline
Status rational_isZero(const Rational* rat) {
  return natural_isZero(&rat->numerator);
}

// TODO: UNTESTED:
static inline
usize rational_snprint(const Rational* rat, char* buffer, usize size) {
  usize neededBytes = natural_printingSize(&rat->numerator);
  neededBytes += natural_printingSize(&rat->denominator);
  neededBytes += 1; // the '/' sign;
  if (rat->sign == -1) {
    neededBytes += 1;
  }
  if (neededBytes > size) {
    return 0;
  }
  usize start = 0;
  if (rat->sign == -1) {
    buffer[0] = '-';
    start++;
  }
  // TODO: refactor: use buffer.h for god's sake
  start += natural_snprint(&rat->numerator, buffer+start, size-start);
  buffer[start] = '/'; start++;
  start += natural_snprint(&rat->denominator, buffer+start, size-start);
  return neededBytes;
}

// TODO: UNTESTED:
Status rational_equal(IAllocator* mem,
                      const Rational* A, const Rational* B,
                      Natural* scr_a, Natural* scr_b,
                      bool* out) {
  if (A->sign != B->sign) {
    *out = false;
    return status_OK;
  }
  Status st;
  st = natural_mult(mem, &A->numerator, &B->denominator, scr_a); status_CHECK;
  st = natural_mult(mem, &B->numerator, &A->denominator, scr_b); status_CHECK;
  *out = natural_equal(scr_a, scr_b);
  return status_OK;
}

/*
TODO: rational_normalize (sets two rationals to the same denominator)
TODO: rational_compare

TODO: rational_simplify (uses gcd to simplify the fraction)

TODO: rational_add
TODO: rational_sub
TODO: rational_div
TODO: rational_mul
TODO: rational_neg
*/

#endif
