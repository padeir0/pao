#include "../src/pao_natural.h"
#include "../src/pao_basicTypes.h"
#include "../src/pao_status.h"
#include "../src/pao_stdAlloc.h"
#include <stdio.h>
#include <strings.h>
#include "common.h"

char buffer[DEFAULT_SIZE];

void printNat(pao_Natural n) {
  usize written = pao_natural_snprint(n, buffer, DEFAULT_SIZE);
  if (written == 0) {
    printf("nothing printed :(\n");
    abort();
  }
  printf("%.*s", (int)written, buffer);
  printf(" (length: %d, cap: %d)\n", n.len, n.cap);
}

/* BEGIN: testing addDigit*/
// TODO: write test for when `a` == `out`
// tests carry
bool test_natural_addDigit_1(void) {
  pao_Natural a = pao_natural_empty();
  pao_Natural out = pao_natural_empty();
  pao_Natural expected = pao_natural_empty();

  u32 A_DIGS[] = {999999999, 999999999};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))

  pao_natural_setVec(PAO_stdAlloc, &a, A_DIGS, A_DIGS_LEN);
  
  u32 EXP_DIGS[A_DIGS_LEN+1] = {1, 0, 0};
  pao_natural_setVec(PAO_stdAlloc, &expected, EXP_DIGS, A_DIGS_LEN+1);

  pao_status s = pao_natural_addDigit(PAO_stdAlloc, &a, 1, &out);
  checkStatus(s);

  return pao_natural_equal(&out, &expected);
}

// tests 0 as identity
bool test_natural_addDigit_2(void) {
  pao_Natural a = pao_natural_empty();
  pao_Natural out = pao_natural_empty();
  pao_Natural expected = pao_natural_empty();

  pao_natural_set(PAO_stdAlloc, &a, 0);
  pao_natural_set(PAO_stdAlloc, &expected, 42);

  pao_status s = pao_natural_addDigit(PAO_stdAlloc, &a, 42, &out);
  checkStatus(s);

  return pao_natural_equal(&out, &expected);
}

// tests 0 as identity
bool test_natural_addDigit_3(void) {
  pao_Natural a = pao_natural_empty();
  pao_Natural out = pao_natural_empty();
  pao_Natural expected = pao_natural_empty();

  pao_natural_set(PAO_stdAlloc, &a, 314159);
  pao_natural_set(PAO_stdAlloc, &expected, 314159);

  pao_status s = pao_natural_addDigit(PAO_stdAlloc, &a, 0, &out);
  checkStatus(s);

  return pao_natural_equal(&out, &expected);
}

// ensures operands are not modified
bool test_natural_addDigit_4(void) {
  pao_Natural a = pao_natural_empty();
  pao_Natural out = pao_natural_empty();
  pao_Natural expected = pao_natural_empty();

  pao_natural_set(PAO_stdAlloc, &a, 314159);
  pao_natural_set(PAO_stdAlloc, &expected, 314160);

  pao_status s = pao_natural_addDigit(PAO_stdAlloc, &a, 1, &out);
  checkStatus(s);

  return pao_natural_equalDigit(&a, 314159);
}
/* END: testing addDigit */

/* BEGIN: testing multDigit */

// tests 0 as annihilator
bool test_natural_multDigit_1(void) {
  pao_Natural a = pao_natural_empty();
  pao_Natural out = pao_natural_empty();
  pao_Natural expected = pao_natural_empty();

  pao_natural_set(PAO_stdAlloc, &a, 314159);
  pao_natural_set(PAO_stdAlloc, &expected, 0);

  pao_status s = pao_natural_multDigit(PAO_stdAlloc, &a, 0, &out);
  checkStatus(s);

  return pao_natural_equal(&out, &expected);
}

// tests 0 as annihilator
bool test_natural_multDigit_2(void) {
  pao_Natural a = pao_natural_empty();
  pao_Natural out = pao_natural_empty();
  pao_Natural expected = pao_natural_empty();

  pao_natural_set(PAO_stdAlloc, &a, 0);
  pao_natural_set(PAO_stdAlloc, &expected, 0);

  pao_status s = pao_natural_multDigit(PAO_stdAlloc, &a, 314159, &out);
  checkStatus(s);

  return pao_natural_equal(&out, &expected);
}

// tests 1 as identity
bool test_natural_multDigit_3(void) {
  pao_Natural a = pao_natural_empty();
  pao_Natural out = pao_natural_empty();
  pao_Natural expected = pao_natural_empty();

  pao_natural_set(PAO_stdAlloc, &a, 314159);
  pao_natural_set(PAO_stdAlloc, &expected, 314159);

  pao_status s = pao_natural_multDigit(PAO_stdAlloc, &a, 1, &out);
  checkStatus(s);

  return pao_natural_equal(&out, &expected);
}

// tests 1 as identity
bool test_natural_multDigit_4(void) {
  pao_Natural a = pao_natural_empty();
  pao_Natural out = pao_natural_empty();
  pao_Natural expected = pao_natural_empty();

  pao_natural_set(PAO_stdAlloc, &a, 1);
  pao_natural_set(PAO_stdAlloc, &expected, 314159);

  pao_status s = pao_natural_multDigit(PAO_stdAlloc, &a, 314159, &out);
  checkStatus(s);

  return pao_natural_equal(&out, &expected);
}

// tests powers of 2
bool test_natural_multDigit_5(void) {
  pao_Natural a = pao_natural_empty();
  pao_Natural out = pao_natural_empty();
  pao_Natural expected = pao_natural_empty();

  {
    pao_natural_set(PAO_stdAlloc, &a, 1);
    pao_natural_set(PAO_stdAlloc, &expected, 2);

    pao_status s = pao_natural_multDigit(PAO_stdAlloc, &a, 2, &out);
    checkStatus(s);
    if (!pao_natural_equal(&out, &expected)) {
      return false;
    }
  }

  {
    pao_natural_set(PAO_stdAlloc, &a, 2);
    pao_natural_set(PAO_stdAlloc, &expected, 4);

    pao_status s = pao_natural_multDigit(PAO_stdAlloc, &a, 2, &out);
    checkStatus(s);
    if (!pao_natural_equal(&out, &expected)) {
      return false;
    }
  }

  {
    pao_natural_set(PAO_stdAlloc, &a, 4);
    pao_natural_set(PAO_stdAlloc, &expected, 8);

    pao_status s = pao_natural_multDigit(PAO_stdAlloc, &a, 2, &out);
    checkStatus(s);
    if (!pao_natural_equal(&out, &expected)) {
      return false;
    }
  }
  return true;
}

// tests some other multiplications
bool test_natural_multDigit_6(void) {
  pao_Natural a = pao_natural_empty();
  pao_Natural out = pao_natural_empty();
  pao_Natural expected = pao_natural_empty();

  {
    pao_natural_set(PAO_stdAlloc, &a, 12);
    pao_natural_set(PAO_stdAlloc, &expected, 144);

    pao_status s = pao_natural_multDigit(PAO_stdAlloc, &a, 12, &out);
    checkStatus(s);
    if (!pao_natural_equal(&out, &expected)) {
      return false;
    }
  }

  {
    pao_natural_set(PAO_stdAlloc, &a, 1111);
    pao_natural_set(PAO_stdAlloc, &expected, 3702963);

    pao_status s = pao_natural_multDigit(PAO_stdAlloc, &a, 3333, &out);
    checkStatus(s);
    if (!pao_natural_equal(&out, &expected)) {
      return false;
    }
  }

  {
    u32 digits[] = {
      999999998, 000000001
    };
    #define DILEN (sizeof(digits) / sizeof(digits[0]))

    pao_natural_set(PAO_stdAlloc, &a, PAO_natural_base-1);
    pao_natural_setVec(PAO_stdAlloc, &expected, digits, DILEN);

    pao_status s = pao_natural_multDigit(PAO_stdAlloc, &a, PAO_natural_base-1, &out);
    checkStatus(s);
    if (!pao_natural_equal(&out, &expected)) {
      return false;
    }
  }
  return true;
}

// tests whether operands remain unchanged
bool test_natural_multDigit_7(void) {
  pao_Natural a = pao_natural_empty();
  pao_Natural out = pao_natural_empty();
  pao_Natural expected = pao_natural_empty();

  pao_natural_set(PAO_stdAlloc, &a, 4);
  pao_natural_set(PAO_stdAlloc, &expected, 16);

  pao_status s = pao_natural_multDigit(PAO_stdAlloc, &a, 4, &out);
  checkStatus(s);

  return pao_natural_equal(&out, &expected) && pao_natural_equalDigit(&a, 4);
}

/* END: testing multDigit */

/* BEGIN: testing multBase*/
bool test_natural_multBase_1(void) {
  pao_status s;
  pao_Natural a = pao_natural_empty();
  pao_Natural expected = pao_natural_empty();

  u32 digits[] = {
    000000001, 000000000
  };
  #define DILEN (sizeof(digits) / sizeof(digits[0]))
  s = pao_natural_set(PAO_stdAlloc, &a, 1);
  checkStatus(s);
  s = pao_natural_setVec(PAO_stdAlloc, &expected, digits, DILEN);
  checkStatus(s);
  s = pao_natural_multBase(PAO_stdAlloc, &a);
  checkStatus(s);

  return pao_natural_equal(&a, &expected);
}

bool test_natural_multBase_2(void) {
  pao_status s;
  pao_Natural a = pao_natural_empty();
  pao_Natural expected = pao_natural_empty();

  u32 digits[] = {
    900000000, 000000000, 000000000
  };
  #define DILEN (sizeof(digits) / sizeof(digits[0]))
  s = pao_natural_set(PAO_stdAlloc, &a, 900000000);
  checkStatus(s);
  s = pao_natural_setVec(PAO_stdAlloc, &expected, digits, DILEN);
  checkStatus(s);
  s = pao_natural_multBase(PAO_stdAlloc, &a);
  checkStatus(s);
  s = pao_natural_multBase(PAO_stdAlloc, &a);
  checkStatus(s);

  return pao_natural_equal(&a, &expected);
}

/* END: testing multBase*/

/* BEGIN: testing snprint */
char test_buffer[DEFAULT_SIZE];

bool test_natural_snprint_0(void) {
  pao_Natural A = pao_natural_empty();
  pao_natural_set(PAO_stdAlloc, &A, 1);

  usize written = pao_natural_snprint(A, test_buffer, DEFAULT_SIZE);
  if (written == 0) {
    return false;
  }

  if (strncmp("1", test_buffer, written) != 0) {
    return false;
  }
  return true;
}

bool test_natural_snprint_1(void) {
  pao_Natural A = pao_natural_empty();

  u32 digits[] = {
    1,0,0
  };
  #define DILEN (sizeof(digits) / sizeof(digits[0]))

  pao_natural_setVec(PAO_stdAlloc, &A, digits, DILEN);

  usize written = pao_natural_snprint(A, test_buffer, DEFAULT_SIZE);
  if (written == 0) {
    return false;
  }

  if (strncmp("1000000000000000000", test_buffer, written) != 0) {
    return false;
  }
  return true;
}

bool test_natural_snprint_2(void) {
  pao_Natural A = pao_natural_empty();

  pao_natural_set(PAO_stdAlloc, &A, 314159);

  usize written = pao_natural_snprint(A, test_buffer, DEFAULT_SIZE);
  if (written == 0) {
    return false;
  }

  if (strncmp("314159", test_buffer, written) != 0) {
    return false;
  }
  return true;
}

bool test_natural_snprint_3(void) {
  pao_Natural A = pao_natural_empty();

  pao_natural_set(PAO_stdAlloc, &A, 0);

  usize written = pao_natural_snprint(A, test_buffer, DEFAULT_SIZE);
  if (written == 0) {
    return false;
  }

  if (strncmp("0", test_buffer, written) != 0) {
    return false;
  }
  return true;
}
/* END: testing snprint */

/* BEGIN: testing distanceDigit */
// tests carry
bool test_natural_distanceDigit_1(void) {
  pao_Natural a = pao_natural_empty();
  pao_Natural out = pao_natural_empty();
  pao_Natural expected = pao_natural_empty();

  u32 A_DIGS[] = {1, 0, 0};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))
  pao_natural_setVec(PAO_stdAlloc, &a, A_DIGS, A_DIGS_LEN);
  
  u32 EXP_DIGS[A_DIGS_LEN-1] = {999999999, 999999999};
  pao_natural_setVec(PAO_stdAlloc, &expected, EXP_DIGS, A_DIGS_LEN-1);

  pao_status s = pao_natural_distanceDigit(PAO_stdAlloc, &a, 1, &out);
  checkStatus(s);

  return pao_natural_equal(&out, &expected);
}

bool test_natural_distanceDigit_2(void) {
  pao_Natural a = pao_natural_empty();
  pao_Natural out = pao_natural_empty();
  pao_Natural expected = pao_natural_empty();

  pao_natural_set(PAO_stdAlloc, &a, 1);
  pao_natural_set(PAO_stdAlloc, &expected, 0);

  pao_status s = pao_natural_distanceDigit(PAO_stdAlloc, &a, 1, &out);
  checkStatus(s);

  return pao_natural_equal(&out, &expected);
}

/* test 0 as identity */
bool test_natural_distanceDigit_3(void) {
  pao_Natural a = pao_natural_empty();
  pao_Natural out = pao_natural_empty();
  pao_Natural expected = pao_natural_empty();

  pao_natural_set(PAO_stdAlloc, &a, 42);
  pao_natural_set(PAO_stdAlloc, &expected, 42);

  pao_status s = pao_natural_distanceDigit(PAO_stdAlloc, &a, 0, &out);
  checkStatus(s);

  return pao_natural_equal(&out, &expected);
}

/* tests 0 as identity, but now we also test if it is comutative */
bool test_natural_distanceDigit_4(void) {
  pao_Natural a = pao_natural_empty();
  pao_Natural out = pao_natural_empty();
  pao_Natural expected = pao_natural_empty();

  pao_natural_set(PAO_stdAlloc, &a, 0);
  pao_natural_set(PAO_stdAlloc, &expected, 42);

  pao_status s = pao_natural_distanceDigit(PAO_stdAlloc, &a, 42, &out);
  checkStatus(s);

  return pao_natural_equal(&out, &expected);
}

// ensures operands are not modified
bool test_natural_distanceDigit_5(void) {
  pao_Natural a = pao_natural_empty();
  pao_Natural out = pao_natural_empty();
  pao_Natural expected = pao_natural_empty();

  pao_natural_set(PAO_stdAlloc, &a, 314159);
  pao_natural_set(PAO_stdAlloc, &expected, 314158);

  pao_status s = pao_natural_distanceDigit(PAO_stdAlloc, &a, 1, &out);
  checkStatus(s);

  return pao_natural_equalDigit(&a, 314159) && pao_natural_equalDigit(&out, 314158);
}
/* END: testing distanceDigit */

/**/

/* BEGIN: testing divDigit */
bool test_natural_divDigit_1(void) {
  pao_Natural A = pao_natural_empty();
  pao_natural_set(PAO_stdAlloc, &A, 5);
  u32 B = 3;
  pao_Natural Q = pao_natural_empty();
  u32 R;

  pao_Natural exp_Q = pao_natural_empty();
  pao_natural_set(PAO_stdAlloc, &exp_Q, 1);
  u32 exp_R = 2;

  pao_status s = pao_natural_divDigit(PAO_stdAlloc, &A, B, &Q, &R);
  checkStatus(s);

  return R != exp_R || pao_natural_equal(&Q, &exp_Q);
}

bool test_natural_divDigit_2(void) {
  pao_Natural A = pao_natural_empty();
  pao_natural_set(PAO_stdAlloc, &A, 0);
  u32 B = 11;
  pao_Natural Q = pao_natural_empty();
  u32 R;

  pao_Natural exp_Q = pao_natural_empty();
  pao_natural_set(PAO_stdAlloc, &exp_Q, 0);
  u32 exp_R = 0;

  pao_status s = pao_natural_divDigit(PAO_stdAlloc, &A, B, &Q, &R);
  checkStatus(s);

  return R != exp_R || pao_natural_equal(&Q, &exp_Q);
}

bool test_natural_divDigit_3(void) {
  pao_Natural A = pao_natural_empty();
  u32 A_DIGS[] = {999999999, 999999999};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))
  pao_natural_setVec(PAO_stdAlloc, &A, A_DIGS, A_DIGS_LEN);

  u32 B = 9;
  pao_Natural Q = pao_natural_empty();
  u32 R;

  pao_Natural exp_Q = pao_natural_empty();
  u32 exp_Q_DIGS[] = {111111111, 111111111};
  #define exp_Q_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))
  pao_natural_setVec(PAO_stdAlloc, &exp_Q, exp_Q_DIGS, exp_Q_DIGS_LEN);
  u32 exp_R = 0;

  pao_status s = pao_natural_divDigit(PAO_stdAlloc, &A, B, &Q, &R);
  checkStatus(s);

  return R != exp_R || pao_natural_equal(&Q, &exp_Q);
}
/* END: testing divDigit */

/* BEGIN: DRIVER CODE */
Tester tests[] = {
  {"test_natural_snprint_0", test_natural_snprint_0},
  {"test_natural_snprint_1", test_natural_snprint_1},
  {"test_natural_snprint_2", test_natural_snprint_2},
  {"test_natural_snprint_3", test_natural_snprint_3},

  {"test_natural_addDigit_1", test_natural_addDigit_1},
  {"test_natural_addDigit_2", test_natural_addDigit_2},
  {"test_natural_addDigit_3", test_natural_addDigit_3},
  {"test_natural_addDigit_4", test_natural_addDigit_4},

  {"test_natural_distanceDigit_1", test_natural_distanceDigit_1},
  {"test_natural_distanceDigit_2", test_natural_distanceDigit_2},
  {"test_natural_distanceDigit_3", test_natural_distanceDigit_3},
  {"test_natural_distanceDigit_4", test_natural_distanceDigit_4},
  {"test_natural_distanceDigit_5", test_natural_distanceDigit_5},

  {"test_natural_multDigit_1", test_natural_multDigit_1},
  {"test_natural_multDigit_2", test_natural_multDigit_2},
  {"test_natural_multDigit_3", test_natural_multDigit_3},
  {"test_natural_multDigit_4", test_natural_multDigit_4},
  {"test_natural_multDigit_5", test_natural_multDigit_5},
  {"test_natural_multDigit_6", test_natural_multDigit_6},
  {"test_natural_multDigit_7", test_natural_multDigit_7},

  {"test_natural_multBase_1", test_natural_multBase_1},
  {"test_natural_multBase_2", test_natural_multBase_2},

  {"test_natural_divDigit_1", test_natural_divDigit_1},
  {"test_natural_divDigit_2", test_natural_divDigit_2},
  {"test_natural_divDigit_3", test_natural_divDigit_3}
};
#define TEST_LEN (int)(sizeof(tests) / sizeof(tests[0]))

int main(void) {
  run_tests(tests, TEST_LEN);
}
/* END: DRIVER CODE */
