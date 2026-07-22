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
};
#define TEST_LEN (int)(sizeof(tests) / sizeof(tests[0]))

int main(void) {
  _alloc = i_failAlloc_new(&_heap, -1); /* never fail */
  run_tests("rational", tests, TEST_LEN);
}
/* END: DRIVER CODE */
