#include "../../lib/numbers/rational.h"
#include "../../lib/basicTypes.h"
#include "../../lib/status.h"
#include <stdio.h>
#include <string.h>
#include "../common.h"

char test_buffer[DEFAULT_SIZE];

/* backed by FLAlloc (see common.h), so info().used is a real
 * allocation count and isAllFree() is a real leak check.
 */
i_FailAllocHeap _heap;
IAllocator _alloc;
#define alloc (&_alloc)

bool isAllFree(void) {
  return alloc->info(alloc->heap).used == 0;
}

/* BEGIN: testing snprint */
bool test_rational_snprint_positive(void) {
  Status s;
  Rational rat = rational_new();
  s = rational_set(alloc, +1, 3, 4, &rat); checkStatus(s);

  usize written = rational_snprint(&rat, test_buffer, DEFAULT_SIZE);
  bool ok = written > 0 && strncmp("3/4", test_buffer, written) == 0;

  rational_free(alloc, rat);
  return ok && isAllFree();
}

bool test_rational_snprint_negative(void) {
  Status s;
  Rational rat = rational_new();
  s = rational_set(alloc, -1, 3, 4, &rat); checkStatus(s);

  usize written = rational_snprint(&rat, test_buffer, DEFAULT_SIZE);
  bool ok = written > 0 && strncmp("-3/4", test_buffer, written) == 0;

  rational_free(alloc, rat);
  return ok && isAllFree();
}

bool test_rational_snprint_zero(void) {
  Status s;
  Rational rat = rational_new();
  s = rational_set(alloc, +1, 0, 5, &rat); checkStatus(s);

  usize written = rational_snprint(&rat, test_buffer, DEFAULT_SIZE);
  bool ok = written > 0 && strncmp("0/5", test_buffer, written) == 0;

  rational_free(alloc, rat);
  return ok && isAllFree();
}

// numerator and denominator with different digit counts: this is
// exactly the case that would have caught the neededBytes bug,
// which summed the numerator's printing size twice instead of
// numerator + denominator.
bool test_rational_snprint_asymmetricSizes(void) {
  Status s;
  Rational rat = rational_new();
  s = rational_set(alloc, +1, 7, 123456789, &rat); checkStatus(s);

  usize written = rational_snprint(&rat, test_buffer, DEFAULT_SIZE);
  bool ok = written > 0 && strncmp("7/123456789", test_buffer, written) == 0;

  rational_free(alloc, rat);
  return ok && isAllFree();
}

// buffer too small must fail without a partial write, exactly
// like natural_snprint.
bool test_rational_snprint_tooSmall(void) {
  Status s;
  Rational rat = rational_new();
  s = rational_set(alloc, +1, 3, 4, &rat); checkStatus(s);

  usize written = rational_snprint(&rat, test_buffer, 2); /* "3/4" needs 3 */
  bool ok = written == 0;

  rational_free(alloc, rat);
  return ok && isAllFree();
}
/* END: testing snprint */

/* BEGIN: testing equal */
static bool i_rationalTest_equalCheck(i8 signA, u32 numA, u32 denA,
                                      i8 signB, u32 numB, u32 denB,
                                      bool expected) {
  Status s;
  Rational A = rational_new();
  Rational B = rational_new();
  Natural scr_a = natural_new();
  Natural scr_b = natural_new();
  bool out;

  s = rational_set(alloc, signA, numA, denA, &A); checkStatus(s);
  s = rational_set(alloc, signB, numB, denB, &B); checkStatus(s);

  s = rational_equal(alloc, &A, &B, &scr_a, &scr_b, &out); checkStatus(s);

  bool ok = out == expected;

  rational_free(alloc, A);
  rational_free(alloc, B);
  natural_free(alloc, scr_a);
  natural_free(alloc, scr_b);
  return ok && isAllFree();
}

// 1/2 == 2/4 (cross multiplication agrees: 1*4 == 2*2)
bool test_rational_equal_equivalentFractions(void) {
  return i_rationalTest_equalCheck(+1, 1, 2, +1, 2, 4, true);
}

// 1/2 != 1/3
bool test_rational_equal_different(void) {
  return i_rationalTest_equalCheck(+1, 1, 2, +1, 1, 3, false);
}

// same magnitude, different sign
bool test_rational_equal_differentSign(void) {
  return i_rationalTest_equalCheck(+1, 1, 2, -1, 1, 2, false);
}

// 0/5 == 0/3: both zero, regardless of denominator
bool test_rational_equal_bothZero(void) {
  return i_rationalTest_equalCheck(+1, 0, 5, +1, 0, 3, true);
}

// a rational equals itself
bool test_rational_equal_reflexive(void) {
  return i_rationalTest_equalCheck(-1, 22, 7, -1, 22, 7, true);
}
/* END: testing equal */

/* BEGIN: testing neg */
bool test_rational_neg_positive(void) {
  Status s;
  Rational rat = rational_new();
  s = rational_set(alloc, +1, 3, 4, &rat); checkStatus(s);
  rational_neg(&rat);
  bool ok = rat.sign == -1;
  rational_free(alloc, rat);
  return ok && isAllFree();
}

bool test_rational_neg_negative(void) {
  Status s;
  Rational rat = rational_new();
  s = rational_set(alloc, -1, 3, 4, &rat); checkStatus(s);
  rational_neg(&rat);
  bool ok = rat.sign == +1;
  rational_free(alloc, rat);
  return ok && isAllFree();
}

bool test_rational_neg_zero(void) {
  Status s;
  Rational rat = rational_new();
  s = rational_set(alloc, +1, 0, 5, &rat); checkStatus(s);
  rational_neg(&rat);
  bool ok = rat.sign == +1; /* zero must stay positive */
  rational_free(alloc, rat);
  return ok && isAllFree();
}

bool test_rational_neg_doubleNeg(void) {
  Status s;
  Rational rat = rational_new();
  s = rational_set(alloc, +1, 3, 4, &rat); checkStatus(s);
  rational_neg(&rat);
  rational_neg(&rat);
  bool ok = rat.sign == +1;
  rational_free(alloc, rat);
  return ok && isAllFree();
}
/* END: testing neg */

/* BEGIN: testing simplify */
#define I_SIMPLIFY_SCRATCH_COUNT 5
static bool i_rationalTest_simplifyCheck(i8 sign, u32 num, u32 den,
                                         u32 expNum, u32 expDen) {
  Status s;
  Rational rat = rational_new();
  Natural scr_gcd = natural_new();
  Natural scr_a   = natural_new();
  Natural scr_b   = natural_new();
  Natural scr_div = natural_new();
  Natural scr_q   = natural_new();

  s = rational_set(alloc, sign, num, den, &rat); checkStatus(s);
  s = rational_simplify(alloc, &rat,
                        &scr_gcd, &scr_a, &scr_b, &scr_div, &scr_q);
  checkStatus(s);

  char buf[64];
  usize written = rational_snprint(&rat, buf, sizeof(buf));
  (void)written;

  bool numOk = rat.numerator.len == 0
                 ? expNum == 0
                 : rat.numerator.digits[0] == expNum;
  bool denOk = rat.denominator.len == 0
                 ? expDen == 0
                 : rat.denominator.digits[0] == expDen;
  bool signOk = natural_isZero(&rat.numerator)
                  ? rat.sign == +1
                  : rat.sign == sign;

  rational_free(alloc, rat);
  natural_free(alloc, scr_gcd);
  natural_free(alloc, scr_a);
  natural_free(alloc, scr_b);
  natural_free(alloc, scr_div);
  natural_free(alloc, scr_q);
  return numOk && denOk && signOk && isAllFree();
}

bool test_rational_simplify_basic(void) {
  return i_rationalTest_simplifyCheck(+1, 6, 4, 3, 2);
}

bool test_rational_simplify_alreadySimplified(void) {
  return i_rationalTest_simplifyCheck(+1, 3, 7, 3, 7);
}

bool test_rational_simplify_zeroNumerator(void) {
  return i_rationalTest_simplifyCheck(+1, 0, 5, 0, 1);
}

bool test_rational_simplify_one(void) {
  return i_rationalTest_simplifyCheck(+1, 5, 5, 1, 1);
}

bool test_rational_simplify_negative(void) {
  return i_rationalTest_simplifyCheck(-1, 6, 4, 3, 2);
}
/* END: testing simplify */

/* BEGIN: testing normalize */
static bool i_rationalTest_normalizeCheck(
  i8 signA, u32 numA, u32 denA,
  i8 signB, u32 numB, u32 denB,
  u32 expNumA, u32 expNumB, u32 expDen
) {
  Status s;
  Rational A = rational_new();
  Rational B = rational_new();
  Natural scr_gcd = natural_new();
  Natural scr_a   = natural_new();
  Natural scr_b   = natural_new();
  Natural scr_div = natural_new();
  Natural scr_q   = natural_new();

  s = rational_set(alloc, signA, numA, denA, &A); checkStatus(s);
  s = rational_set(alloc, signB, numB, denB, &B); checkStatus(s);
  s = rational_normalize(alloc, &A, &B,
                         &scr_gcd, &scr_a, &scr_b, &scr_div, &scr_q);
  checkStatus(s);

  bool denMatch = A.denominator.digits[0] == B.denominator.digits[0];
  bool numAOk   = A.numerator.digits[0] == expNumA;
  bool numBOk   = B.numerator.digits[0] == expNumB;
  bool denOk    = A.denominator.digits[0] == expDen;

  rational_free(alloc, A);
  rational_free(alloc, B);
  natural_free(alloc, scr_gcd);
  natural_free(alloc, scr_a);
  natural_free(alloc, scr_b);
  natural_free(alloc, scr_div);
  natural_free(alloc, scr_q);
  return denMatch && numAOk && numBOk && denOk && isAllFree();
}

bool test_rational_normalize_sameDenom(void) {
  /* 1/4 and 3/4 — already same denom, must be unchanged */
  return i_rationalTest_normalizeCheck(+1,1,4, +1,3,4, 1,3,4);
}

bool test_rational_normalize_different(void) {
  /* 1/2 and 1/3 → 3/6 and 2/6 */
  return i_rationalTest_normalizeCheck(+1,1,2, +1,1,3, 3,2,6);
}

bool test_rational_normalize_oneMultiple(void) {
  /* 1/3 and 1/6 → 2/6 and 1/6 */
  return i_rationalTest_normalizeCheck(+1,1,3, +1,1,6, 2,1,6);
}
/* END: testing normalize */

/* BEGIN: testing compare */
static bool i_rationalTest_compareCheck(i8 signA, u32 numA, u32 denA,
                                        i8 signB, u32 numB, u32 denB,
                                        Order expected) {
  Status s;
  Rational A = rational_new();
  Rational B = rational_new();
  Natural scr_a = natural_new();
  Natural scr_b = natural_new();
  Order result;

  s = rational_set(alloc, signA, numA, denA, &A); checkStatus(s);
  s = rational_set(alloc, signB, numB, denB, &B); checkStatus(s);
  s = rational_compare(alloc, &A, &B, &scr_a, &scr_b, &result);
  checkStatus(s);

  rational_free(alloc, A);
  rational_free(alloc, B);
  natural_free(alloc, scr_a);
  natural_free(alloc, scr_b);
  return result == expected && isAllFree();
}

bool test_rational_compare_less(void) {
  return i_rationalTest_compareCheck(+1,1,3, +1,1,2, order_LESS);
}

bool test_rational_compare_equal(void) {
  return i_rationalTest_compareCheck(+1,2,4, +1,1,2, order_EQUAL);
}

bool test_rational_compare_greater(void) {
  return i_rationalTest_compareCheck(+1,3,4, +1,1,2, order_GREATER);
}

bool test_rational_compare_positiveVsNegative(void) {
  return i_rationalTest_compareCheck(+1,1,2, -1,1,2, order_GREATER);
}

bool test_rational_compare_negativeVsPositive(void) {
  return i_rationalTest_compareCheck(-1,1,2, +1,1,2, order_LESS);
}

bool test_rational_compare_bothNegative(void) {
  /* -1/2 vs -1/3: -0.5 < -0.33, so less */
  return i_rationalTest_compareCheck(-1,1,2, -1,1,3, order_LESS);
}

bool test_rational_compare_bothZero(void) {
  return i_rationalTest_compareCheck(+1,0,1, +1,0,1, order_EQUAL);
}

bool test_rational_compare_zeroVsPositive(void) {
  return i_rationalTest_compareCheck(+1,0,1, +1,1,2, order_LESS);
}
/* END: testing compare */

/* BEGIN: testing arithmetic */

/* Helper: compute op(A,B) and check numerator, denominator, sign.
   num/den checked as single digits (fits in one u32). */
static bool i_rationalTest_addCheck(
  i8 signA, u32 numA, u32 denA,
  i8 signB, u32 numB, u32 denB,
  i8 expSign, u32 expNum, u32 expDen
) {
  Status s;
  Rational A = rational_new();
  Rational B = rational_new();
  Rational out = rational_new();
  Natural scr_a = natural_new();
  Natural scr_b = natural_new();

  s = rational_set(alloc, signA, numA, denA, &A); checkStatus(s);
  s = rational_set(alloc, signB, numB, denB, &B); checkStatus(s);
  s = rational_add(alloc, &A, &B, &scr_a, &scr_b, &out);
  checkStatus(s);

  bool numOk  = out.numerator.len   > 0 ? out.numerator.digits[0]   == expNum : expNum == 0;
  bool denOk  = out.denominator.len > 0 ? out.denominator.digits[0] == expDen : expDen == 0;
  bool signOk = natural_isZero(&out.numerator) ? out.sign == +1 : out.sign == expSign;

  rational_free(alloc, A);
  rational_free(alloc, B);
  rational_free(alloc, out);
  natural_free(alloc, scr_a);
  natural_free(alloc, scr_b);
  return numOk && denOk && signOk && isAllFree();
}

bool test_rational_add_sameDenom(void) {
  /* 1/4 + 2/4 = 3/4, but via cross-mult: 4/16 → 12/16 */
  /* Actually: 1/4 + 2/4 = (1*4 + 2*4)/(4*4) = 12/16 */
  return i_rationalTest_addCheck(+1,1,4, +1,2,4, +1,12,16);
}

bool test_rational_add_differentDenom(void) {
  /* 1/2 + 1/3 = (1*3 + 1*2)/(2*3) = 5/6 */
  return i_rationalTest_addCheck(+1,1,2, +1,1,3, +1,5,6);
}

bool test_rational_add_negPlusPos(void) {
  /* -1/2 + 1/4 = (-1*4 + 1*2)/(2*4) = -2/8 → since |A|>|B| sign=A=-1 */
  return i_rationalTest_addCheck(-1,1,2, +1,1,4, -1,2,8);
}

bool test_rational_add_oppositeCancel(void) {
  /* 1/2 + (-1/2) = 0 */
  return i_rationalTest_addCheck(+1,1,2, -1,1,2, +1,0,4);
}

bool test_rational_add_zero(void) {
  /* 0/1 + 1/3 = 1/3 via cross-mult: (0*3 + 1*1)/(1*3) = 1/3 */
  return i_rationalTest_addCheck(+1,0,1, +1,1,3, +1,1,3);
}

/* sub tests */
static bool i_rationalTest_subCheck(
  i8 signA, u32 numA, u32 denA,
  i8 signB, u32 numB, u32 denB,
  i8 expSign, u32 expNum, u32 expDen
) {
  Status s;
  Rational A = rational_new();
  Rational B = rational_new();
  Rational out = rational_new();
  Natural scr_a = natural_new();
  Natural scr_b = natural_new();

  s = rational_set(alloc, signA, numA, denA, &A); checkStatus(s);
  s = rational_set(alloc, signB, numB, denB, &B); checkStatus(s);
  s = rational_sub(alloc, &A, &B, &scr_a, &scr_b, &out);
  checkStatus(s);

  bool numOk  = out.numerator.len   > 0 ? out.numerator.digits[0]   == expNum : expNum == 0;
  bool denOk  = out.denominator.len > 0 ? out.denominator.digits[0] == expDen : expDen == 0;
  bool signOk = natural_isZero(&out.numerator) ? out.sign == +1 : out.sign == expSign;

  rational_free(alloc, A);
  rational_free(alloc, B);
  rational_free(alloc, out);
  natural_free(alloc, scr_a);
  natural_free(alloc, scr_b);
  return numOk && denOk && signOk && isAllFree();
}

bool test_rational_sub_basic(void) {
  /* 3/4 - 1/4: cross: (3*4 - 1*4)/(4*4) = 8/16 */
  return i_rationalTest_subCheck(+1,3,4, +1,1,4, +1,8,16);
}

bool test_rational_sub_givesNegative(void) {
  /* 1/4 - 1/2: cross: (1*2 - 1*4)/(4*2) → |4| > |2| → sign=-1, num=2, den=8 */
  return i_rationalTest_subCheck(+1,1,4, +1,1,2, -1,2,8);
}

bool test_rational_sub_zero(void) {
  /* 1/2 - 1/2 = 0 */
  return i_rationalTest_subCheck(+1,1,2, +1,1,2, +1,0,4);
}

/* mul tests */
static bool i_rationalTest_mulCheck(
  i8 signA, u32 numA, u32 denA,
  i8 signB, u32 numB, u32 denB,
  i8 expSign, u32 expNum, u32 expDen
) {
  Status s;
  Rational A = rational_new();
  Rational B = rational_new();
  Rational out = rational_new();

  s = rational_set(alloc, signA, numA, denA, &A); checkStatus(s);
  s = rational_set(alloc, signB, numB, denB, &B); checkStatus(s);
  s = rational_mul(alloc, &A, &B, &out);
  checkStatus(s);

  bool numOk  = out.numerator.len   > 0 ? out.numerator.digits[0]   == expNum : expNum == 0;
  bool denOk  = out.denominator.len > 0 ? out.denominator.digits[0] == expDen : expDen == 0;
  bool signOk = natural_isZero(&out.numerator) ? out.sign == +1 : out.sign == expSign;

  rational_free(alloc, A);
  rational_free(alloc, B);
  rational_free(alloc, out);
  return numOk && denOk && signOk && isAllFree();
}

bool test_rational_mul_basic(void) {
  /* (2/3) * (3/4) = 6/12 */
  return i_rationalTest_mulCheck(+1,2,3, +1,3,4, +1,6,12);
}

bool test_rational_mul_negative(void) {
  /* (-1/2) * (2/3) = -2/6 */
  return i_rationalTest_mulCheck(-1,1,2, +1,2,3, -1,2,6);
}

bool test_rational_mul_bothNegative(void) {
  /* (-1/2) * (-1/3) = 1/6 */
  return i_rationalTest_mulCheck(-1,1,2, -1,1,3, +1,1,6);
}

bool test_rational_mul_byZero(void) {
  /* (1/2) * (0/1) = 0 */
  return i_rationalTest_mulCheck(+1,1,2, +1,0,1, +1,0,1);
}

/* div tests */
static bool i_rationalTest_divCheck(
  i8 signA, u32 numA, u32 denA,
  i8 signB, u32 numB, u32 denB,
  i8 expSign, u32 expNum, u32 expDen
) {
  Status s;
  Rational A = rational_new();
  Rational B = rational_new();
  Rational out = rational_new();

  s = rational_set(alloc, signA, numA, denA, &A); checkStatus(s);
  s = rational_set(alloc, signB, numB, denB, &B); checkStatus(s);
  s = rational_div(alloc, &A, &B, &out);
  checkStatus(s);

  bool numOk  = out.numerator.len   > 0 ? out.numerator.digits[0]   == expNum : expNum == 0;
  bool denOk  = out.denominator.len > 0 ? out.denominator.digits[0] == expDen : expDen == 0;
  bool signOk = natural_isZero(&out.numerator) ? out.sign == +1 : out.sign == expSign;

  rational_free(alloc, A);
  rational_free(alloc, B);
  rational_free(alloc, out);
  return numOk && denOk && signOk && isAllFree();
}

bool test_rational_div_basic(void) {
  /* (1/2) / (3/4) = (1*4)/(2*3) = 4/6 */
  return i_rationalTest_divCheck(+1,1,2, +1,3,4, +1,4,6);
}

bool test_rational_div_negative(void) {
  /* (-2/3) / (1/2) = (-2*2)/(3*1) = -4/3 */
  return i_rationalTest_divCheck(-1,2,3, +1,1,2, -1,4,3);
}

bool test_rational_div_zeroNumerator(void) {
  /* (0/1) / (1/2) = 0 */
  return i_rationalTest_divCheck(+1,0,1, +1,1,2, +1,0,1);
}

bool test_rational_div_byZero(void) {
  Status s;
  Rational A = rational_new();
  Rational B = rational_new();
  Rational out = rational_new();
  s = rational_set(alloc, +1, 1, 2, &A);
  s = rational_set(alloc, +1, 0, 1, &B);
  s = rational_div(alloc, &A, &B, &out);
  rational_free(alloc, A);
  rational_free(alloc, B);
  rational_free(alloc, out);
  (void)s;
  return s == status_DIVISIONBYZERO && isAllFree();
}
/* END: testing arithmetic */

/* BEGIN: DRIVER CODE */
Tester tests[] = {
  {"test_rational_snprint_positive", test_rational_snprint_positive},
  {"test_rational_snprint_negative", test_rational_snprint_negative},
  {"test_rational_snprint_zero", test_rational_snprint_zero},
  {"test_rational_snprint_asymmetricSizes", test_rational_snprint_asymmetricSizes},
  {"test_rational_snprint_tooSmall", test_rational_snprint_tooSmall},

  {"test_rational_equal_equivalentFractions", test_rational_equal_equivalentFractions},
  {"test_rational_equal_different", test_rational_equal_different},
  {"test_rational_equal_differentSign", test_rational_equal_differentSign},
  {"test_rational_equal_bothZero", test_rational_equal_bothZero},
  {"test_rational_equal_reflexive", test_rational_equal_reflexive},

  {"test_rational_neg_positive", test_rational_neg_positive},
  {"test_rational_neg_negative", test_rational_neg_negative},
  {"test_rational_neg_zero", test_rational_neg_zero},
  {"test_rational_neg_doubleNeg", test_rational_neg_doubleNeg},

  {"test_rational_simplify_basic", test_rational_simplify_basic},
  {"test_rational_simplify_alreadySimplified", test_rational_simplify_alreadySimplified},
  {"test_rational_simplify_zeroNumerator", test_rational_simplify_zeroNumerator},
  {"test_rational_simplify_one", test_rational_simplify_one},
  {"test_rational_simplify_negative", test_rational_simplify_negative},

  {"test_rational_normalize_sameDenom", test_rational_normalize_sameDenom},
  {"test_rational_normalize_different", test_rational_normalize_different},
  {"test_rational_normalize_oneMultiple", test_rational_normalize_oneMultiple},

  {"test_rational_compare_less", test_rational_compare_less},
  {"test_rational_compare_equal", test_rational_compare_equal},
  {"test_rational_compare_greater", test_rational_compare_greater},
  {"test_rational_compare_positiveVsNegative", test_rational_compare_positiveVsNegative},
  {"test_rational_compare_negativeVsPositive", test_rational_compare_negativeVsPositive},
  {"test_rational_compare_bothNegative", test_rational_compare_bothNegative},
  {"test_rational_compare_bothZero", test_rational_compare_bothZero},
  {"test_rational_compare_zeroVsPositive", test_rational_compare_zeroVsPositive},

  {"test_rational_add_sameDenom", test_rational_add_sameDenom},
  {"test_rational_add_differentDenom", test_rational_add_differentDenom},
  {"test_rational_add_negPlusPos", test_rational_add_negPlusPos},
  {"test_rational_add_oppositeCancel", test_rational_add_oppositeCancel},
  {"test_rational_add_zero", test_rational_add_zero},

  {"test_rational_sub_basic", test_rational_sub_basic},
  {"test_rational_sub_givesNegative", test_rational_sub_givesNegative},
  {"test_rational_sub_zero", test_rational_sub_zero},

  {"test_rational_mul_basic", test_rational_mul_basic},
  {"test_rational_mul_negative", test_rational_mul_negative},
  {"test_rational_mul_bothNegative", test_rational_mul_bothNegative},
  {"test_rational_mul_byZero", test_rational_mul_byZero},

  {"test_rational_div_basic", test_rational_div_basic},
  {"test_rational_div_negative", test_rational_div_negative},
  {"test_rational_div_zeroNumerator", test_rational_div_zeroNumerator},
  {"test_rational_div_byZero", test_rational_div_byZero},
};
#define TEST_LEN (int)(sizeof(tests) / sizeof(tests[0]))

int main(void) {
  _alloc = i_failAlloc_new(&_heap, -1); /* never fail */
  run_tests("rational", tests, TEST_LEN);
}
/* END: DRIVER CODE */
