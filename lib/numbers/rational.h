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

/* Prints `rat` as "[-]num/den" into `buffer[0..size-1]`.
   Returns the number of bytes written, or 0 if the buffer is too small.
*/
static inline
usize rational_snprint(const Rational* rat, char* buffer, usize size) {
  usize neededBytes = natural_printingSize(&rat->numerator);
  neededBytes += natural_printingSize(&rat->denominator);
  neededBytes += 1; // the '/' separator;
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

  start += natural_snprint(&rat->numerator, buffer+start, size-start);
  buffer[start] = '/'; start++;
  start += natural_snprint(&rat->denominator, buffer+start, size-start);
  return neededBytes;
}

/* Returns true if A and B represent the same rational value,
   not necessarily that they have the exacts numerators and denominators.
*/
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

/* Negates `rat` in-place. Zero stays positive. */
static inline
void rational_neg(Rational* rat) {
  if (!natural_isZero(&rat->numerator)) {
    rat->sign = (i8)(rat->sign * -1);
  }
}

/* Reduces `rat` to lowest terms. If the numerator is zero, sets the
   denominator to 1 (canonical form).
   scr_gcd receives the GCD result; the remaining scratch Naturals are
   forwarded to natural_gcd and natural_div. All must be distinct. */
static inline
Status rational_simplify(IAllocator* mem, Rational* rat,
                         Natural* scratch[5]) {
  #if config_DEBUG
    if (mem == NULL || rat == NULL) {
      debug_FATALFMT("Some pointer parameter is null. mem = %p, rat = %p.", (void*)mem, (void*)rat);
    }
    { // i know GCD also checks this!!
      int i = 0;
      while (i < 5) {
        if (scratch[i] == NULL) {
          debug_FATALFMT("Some scratch parameter is null. scratch[%d] == NULL.", i);
        }
        i++;
      }
    }
    {
      int i = 0;
      while (i < 5) {
        int j = 0;
        while (j < 5) {
          if (i != j && scratch[i] == scratch[j]) {
            debug_FATALFMT("Aliasing requirements not met. scratch[%d] == scratch[%d].", i, j);
          }
          j++;
        }
        i++;
      }
    }
  #endif
  Status st;
  Natural* gcd = scratch[0];
  Natural* q = scratch[1];
  Natural* r = scratch[2];
  Natural* scr_div = scratch[3];
  Natural** gcd_scratch = scratch+1;

  // 0 is represented as 0/1
  if (natural_isZero(&rat->numerator)) {
    st = natural_set(mem, 1, &rat->denominator); status_CHECK;
    rat->sign = +1;
    return status_OK;
  }

  st = natural_gcd(mem,
                   &rat->numerator, &rat->denominator,
                   scratch[0],
                   gcd_scratch); status_CHECK;

  st = natural_div(mem, scr_div, &rat->numerator, gcd, q, r); status_CHECK;
  st = natural_copy(mem, q, &rat->numerator);                 status_CHECK;

  st = natural_div(mem, scr_div, &rat->denominator, gcd, q, r); status_CHECK;
  st = natural_copy(mem, q, &rat->denominator);                 status_CHECK;
  return status_OK;
  /* NOTE(1): We copy num/den into scr_a first so that natural_div's A
              argument is never aliased with its Q or R outputs. scr_b is
              reused as the remainder (always zero since gcd divides exactly).
  */
}

/* Compares two rationals. Returns LESS, EQUAL, or GREATER.
   Uses cross-multiplication: a/b vs c/d → compare a*d vs c*b.
   Denominators are always positive. */
static inline
Status rational_compare(IAllocator* mem,
                        const Rational* A, const Rational* B,
                        Natural* scr_a, Natural* scr_b,
                        Order* out) {
  #if config_DEBUG
    if (mem == NULL || A == NULL || B == NULL || scr_a == NULL || scr_b == NULL || out == NULL) {
      debug_FATALFMT("Null parameter. mem=%p A=%p B=%p scr_a=%p scr_b=%p out=%p",
                     (void*)mem, (void*)A, (void*)B, (void*)scr_a, (void*)scr_b, (void*)out);
    }
  #endif
  Status st;

  bool aIsZero = natural_isZero(&A->numerator);
  bool bIsZero = natural_isZero(&B->numerator);

  if (aIsZero && bIsZero) {
    *out = order_EQUAL;
    return status_OK;
  }

  if (A->sign != B->sign) {
    *out = (A->sign > B->sign) ? order_GREATER : order_LESS;
    return status_OK;
  }

  /* same sign: cross-multiply and compare magnitudes */
  st = natural_mult(mem, &A->numerator,   &B->denominator, scr_a); status_CHECK;
  st = natural_mult(mem, &B->numerator,   &A->denominator, scr_b); status_CHECK;

  Order natOrd = natural_compare(scr_a, scr_b);

  if (A->sign < 0) {
    *out = order_invert(natOrd);
  } else {
    *out = natOrd;
  }

  return status_OK;
}

/* Adds A and B, writing the (unsimplified) result to out.
   scr_a and scr_b are scratch Naturals for the cross products.
   out must be freshly constructed (rational_new) or already freed. */
static inline
Status rational_add(IAllocator* mem,
                    const Rational* A, const Rational* B,
                    Natural* scr_a, Natural* scr_b,
                    Rational* out) {
  Status st;

  /* cross products: A.num * B.den and B.num * A.den */
  st = natural_mult(mem, &A->numerator, &B->denominator, scr_a); status_CHECK;
  st = natural_mult(mem, &B->numerator, &A->denominator, scr_b); status_CHECK;
  /* denominator: A.den * B.den */
  st = natural_mult(mem, &A->denominator, &B->denominator, &out->denominator); status_CHECK;

  if (A->sign == B->sign) {
    /* same sign: just add the numerators */
    out->sign = A->sign;
    st = natural_add(mem, scr_a, scr_b, &out->numerator); status_CHECK;
  } else {
    /* different signs: subtract smaller from larger; sign = sign of larger */
    Order ord = natural_compare(scr_a, scr_b);
    if (ord == order_EQUAL) {
      out->sign = +1;
      st = natural_set(mem, 0, &out->numerator); status_CHECK;
    } else if (ord == order_GREATER) {
      out->sign = A->sign;
      st = natural_distance(mem, scr_a, scr_b, &out->numerator); status_CHECK;
    } else {
      out->sign = B->sign;
      st = natural_distance(mem, scr_b, scr_a, &out->numerator); status_CHECK;
    }
  }

  return status_OK;
}

/* Subtracts B from A, writing the (unsimplified) result to out. */
static inline
Status rational_sub(IAllocator* mem,
                    const Rational* A, const Rational* B,
                    Natural* scr_a, Natural* scr_b,
                    Rational* out) {
  /* flip B's sign and add */
  Rational negB;
  negB.sign = (i8)(B->sign * -1);
  if (natural_isZero(&B->numerator)) {
    negB.sign = +1;
  }
  negB.numerator   = B->numerator;
  negB.denominator = B->denominator;
  return rational_add(mem, A, &negB, scr_a, scr_b, out);
}

/* Multiplies A and B: (a/b) * (c/d) = (a*c)/(b*d). */
static inline
Status rational_mul(IAllocator* mem,
                    const Rational* A, const Rational* B,
                    Rational* out) {
  Status st;
  out->sign = (i8)(A->sign * B->sign);
  if (natural_isZero(&A->numerator) || natural_isZero(&B->numerator)) {
    out->sign = +1;
    st = natural_set(mem, 0, &out->numerator);    status_CHECK;
    st = natural_set(mem, 1, &out->denominator);  status_CHECK;
    return status_OK;
  }
  st = natural_mult(mem, &A->numerator,   &B->numerator,   &out->numerator);   status_CHECK;
  st = natural_mult(mem, &A->denominator, &B->denominator, &out->denominator); status_CHECK;
  return status_OK;
}

/* Divides A by B: (a/b) / (c/d) = (a*d)/(b*c).
   Returns status_DIVISIONBYZERO if B's numerator is zero. */
static inline
Status rational_div(IAllocator* mem,
                    const Rational* A, const Rational* B,
                    Rational* out) {
  if (natural_isZero(&B->numerator)) {
    return status_DIVISIONBYZERO;
  }
  Status st;
  out->sign = (i8)(A->sign * B->sign);
  if (natural_isZero(&A->numerator)) {
    out->sign = +1;
    st = natural_set(mem, 0, &out->numerator);    status_CHECK;
    st = natural_set(mem, 1, &out->denominator);  status_CHECK;
    return status_OK;
  }
  st = natural_mult(mem, &A->numerator,   &B->denominator, &out->numerator);   status_CHECK;
  st = natural_mult(mem, &A->denominator, &B->numerator,   &out->denominator); status_CHECK;
  return status_OK;
}

#endif
