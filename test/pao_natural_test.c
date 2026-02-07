#include "../src/mb_natural.h"
#include "../src/mb_basicTypes.h"
#include "../src/mb_status.h"
#include "../src/mb_stdAlloc.h"
#include <stdio.h>
#include <strings.h>
#include "common.h"

char buffer[DEFAULT_SIZE];

void printNat(mb_Natural n) {
  usize written = mb_natural_snprint(n, buffer, DEFAULT_SIZE);
  if (written == 0) {
    printf("written 0 bytes.\n");
  }
  printf("%.*s", (int)written, buffer);
  printf(" (length: %d, cap: %d)\n", n.len, n.cap);
}

/* BEGIN: testing addDigit*/
bool test_natural_addDigit_0(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural out = a;
  mb_Natural expected = mb_natural_empty();

  u32 A_DIGS[] = {999999999, 999999999};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))

  mb_natural_setVec(MB_stdAlloc, A_DIGS, A_DIGS_LEN, &a);
  
  u32 EXP_DIGS[A_DIGS_LEN+1] = {1, 0, 0};
  mb_natural_setVec(MB_stdAlloc, EXP_DIGS, A_DIGS_LEN+1, &expected);

  mb_status s = mb_natural_addDigit(MB_stdAlloc, &a, 1, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// tests carry
bool test_natural_addDigit_1(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  u32 A_DIGS[] = {999999999, 999999999};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))

  mb_natural_setVec(MB_stdAlloc, A_DIGS, A_DIGS_LEN, &a);
  
  u32 EXP_DIGS[A_DIGS_LEN+1] = {1, 0, 0};
  mb_natural_setVec(MB_stdAlloc, EXP_DIGS, A_DIGS_LEN+1, &expected);

  mb_status s = mb_natural_addDigit(MB_stdAlloc, &a, 1, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// tests 0 as identity
bool test_natural_addDigit_2(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  mb_natural_set(MB_stdAlloc, 0, &a);
  mb_natural_set(MB_stdAlloc, 42, &expected);

  mb_status s = mb_natural_addDigit(MB_stdAlloc, &a, 42, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// tests 0 as identity
bool test_natural_addDigit_3(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  mb_natural_set(MB_stdAlloc, 314159, &a);
  mb_natural_set(MB_stdAlloc, 314159, &expected);

  mb_status s = mb_natural_addDigit(MB_stdAlloc, &a, 0, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// ensures operands are not modified
bool test_natural_addDigit_4(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  mb_natural_set(MB_stdAlloc, 314159, &a);
  mb_natural_set(MB_stdAlloc, 314160, &expected);

  mb_status s = mb_natural_addDigit(MB_stdAlloc, &a, 1, &out);
  checkStatus(s);

  return mb_natural_equalDigit(&a, 314159);
}
/* END: testing addDigit */

/* BEGIN: testing multDigit */

// tests 0 as annihilator
bool test_natural_multDigit_1(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  mb_natural_set(MB_stdAlloc, 314159, &a);
  mb_natural_set(MB_stdAlloc, 0, &expected);

  mb_status s = mb_natural_multDigit(MB_stdAlloc, &a, 0, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// tests 0 as annihilator
bool test_natural_multDigit_2(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  mb_natural_set(MB_stdAlloc, 0, &a);
  mb_natural_set(MB_stdAlloc, 0, &expected);

  mb_status s = mb_natural_multDigit(MB_stdAlloc, &a, 314159, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// tests 1 as identity
bool test_natural_multDigit_3(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  mb_natural_set(MB_stdAlloc, 314159, &a);
  mb_natural_set(MB_stdAlloc, 314159, &expected);

  mb_status s = mb_natural_multDigit(MB_stdAlloc, &a, 1, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// tests 1 as identity
bool test_natural_multDigit_4(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  mb_natural_set(MB_stdAlloc, 1, &a);
  mb_natural_set(MB_stdAlloc, 314159, &expected);

  mb_status s = mb_natural_multDigit(MB_stdAlloc, &a, 314159, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// tests powers of 2
bool test_natural_multDigit_5(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  {
    mb_natural_set(MB_stdAlloc, 1, &a);
    mb_natural_set(MB_stdAlloc, 2, &expected);

    mb_status s = mb_natural_multDigit(MB_stdAlloc, &a, 2, &out);
    checkStatus(s);
    if (!mb_natural_equal(&out, &expected)) {
      return false;
    }
  }

  {
    mb_natural_set(MB_stdAlloc, 2, &a);
    mb_natural_set(MB_stdAlloc, 4, &expected);

    mb_status s = mb_natural_multDigit(MB_stdAlloc, &a, 2, &out);
    checkStatus(s);
    if (!mb_natural_equal(&out, &expected)) {
      return false;
    }
  }

  {
    mb_natural_set(MB_stdAlloc, 4, &a);
    mb_natural_set(MB_stdAlloc, 8, &expected);

    mb_status s = mb_natural_multDigit(MB_stdAlloc, &a, 2, &out);
    checkStatus(s);
    if (!mb_natural_equal(&out, &expected)) {
      return false;
    }
  }
  return true;
}

// tests some other multiplications
bool test_natural_multDigit_6(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  {
    mb_natural_set(MB_stdAlloc, 12, &a);
    mb_natural_set(MB_stdAlloc, 144, &expected);

    mb_status s = mb_natural_multDigit(MB_stdAlloc, &a, 12, &out);
    checkStatus(s);
    if (!mb_natural_equal(&out, &expected)) {
      return false;
    }
  }

  {
    mb_natural_set(MB_stdAlloc, 1111, &a);
    mb_natural_set(MB_stdAlloc, 3702963, &expected);

    mb_status s = mb_natural_multDigit(MB_stdAlloc, &a, 3333, &out);
    checkStatus(s);
    if (!mb_natural_equal(&out, &expected)) {
      return false;
    }
  }

  {
    u32 digits[] = {
      999999998, 000000001
    };
    #define DILEN (sizeof(digits) / sizeof(digits[0]))

    mb_natural_set(MB_stdAlloc, MB_natural_base-1, &a);
    mb_natural_setVec(MB_stdAlloc, digits, DILEN, &expected);

    mb_status s = mb_natural_multDigit(MB_stdAlloc, &a, MB_natural_base-1, &out);
    checkStatus(s);
    if (!mb_natural_equal(&out, &expected)) {
      return false;
    }
  }
  return true;
}

// tests whether operands remain unchanged
bool test_natural_multDigit_7(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  mb_natural_set(MB_stdAlloc, 4, &a);
  mb_natural_set(MB_stdAlloc, 16, &expected);

  mb_status s = mb_natural_multDigit(MB_stdAlloc, &a, 4, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected) && mb_natural_equalDigit(&a, 4);
}

/* END: testing multDigit */

/* BEGIN: testing multBase*/
bool test_natural_multBase_1(void) {
  mb_status s;
  mb_Natural a = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  u32 digits[] = {
    000000001, 000000000
  };
  #define DILEN (sizeof(digits) / sizeof(digits[0]))
  s = mb_natural_set(MB_stdAlloc, 1, &a);
  checkStatus(s);
  s = mb_natural_setVec(MB_stdAlloc, digits, DILEN, &expected);
  checkStatus(s);
  s = mb_natural_multBase(MB_stdAlloc, &a);
  checkStatus(s);

  return mb_natural_equal(&a, &expected);
}

bool test_natural_multBase_2(void) {
  mb_status s;
  mb_Natural a = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  u32 digits[] = {
    900000000, 000000000, 000000000
  };
  #define DILEN (sizeof(digits) / sizeof(digits[0]))
  s = mb_natural_set(MB_stdAlloc, 900000000, &a);
  checkStatus(s);
  s = mb_natural_setVec(MB_stdAlloc, digits, DILEN, &expected);
  checkStatus(s);
  s = mb_natural_multBase(MB_stdAlloc, &a);
  checkStatus(s);
  s = mb_natural_multBase(MB_stdAlloc, &a);
  checkStatus(s);

  return mb_natural_equal(&a, &expected);
}

/* END: testing multBase*/

/* BEGIN: testing snprint */
char test_buffer[DEFAULT_SIZE];

bool test_natural_snprint_0(void) {
  mb_Natural A = mb_natural_empty();
  mb_natural_set(MB_stdAlloc, 1, &A);

  usize written = mb_natural_snprint(A, test_buffer, DEFAULT_SIZE);
  if (written == 0) {
    return false;
  }

  if (strncmp("1", test_buffer, written) != 0) {
    return false;
  }
  return true;
}

bool test_natural_snprint_1(void) {
  mb_Natural A = mb_natural_empty();

  u32 digits[] = {
    1,0,0
  };
  #define DILEN (sizeof(digits) / sizeof(digits[0]))

  mb_natural_setVec(MB_stdAlloc, digits, DILEN, &A);

  usize written = mb_natural_snprint(A, test_buffer, DEFAULT_SIZE);
  if (written == 0) {
    return false;
  }

  if (strncmp("1000000000000000000", test_buffer, written) != 0) {
    return false;
  }
  return true;
}

bool test_natural_snprint_2(void) {
  mb_Natural A = mb_natural_empty();

  mb_natural_set(MB_stdAlloc, 314159, &A);

  usize written = mb_natural_snprint(A, test_buffer, DEFAULT_SIZE);
  if (written == 0) {
    return false;
  }

  if (strncmp("314159", test_buffer, written) != 0) {
    return false;
  }
  return true;
}

bool test_natural_snprint_3(void) {
  mb_Natural A = mb_natural_empty();

  mb_natural_set(MB_stdAlloc, 0, &A);

  usize written = mb_natural_snprint(A, test_buffer, DEFAULT_SIZE);
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
  mb_Natural a = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  u32 A_DIGS[] = {1, 0, 0};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))
  mb_natural_setVec(MB_stdAlloc, A_DIGS, A_DIGS_LEN, &a);
  
  u32 EXP_DIGS[A_DIGS_LEN-1] = {999999999, 999999999};
  mb_natural_setVec(MB_stdAlloc, EXP_DIGS, A_DIGS_LEN-1, &expected);

  mb_status s = mb_natural_distanceDigit(MB_stdAlloc, &a, 1, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

bool test_natural_distanceDigit_2(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  mb_natural_set(MB_stdAlloc, 1, &a);
  mb_natural_set(MB_stdAlloc, 0, &expected);

  mb_status s = mb_natural_distanceDigit(MB_stdAlloc, &a, 1, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

/* test 0 as identity */
bool test_natural_distanceDigit_3(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  mb_natural_set(MB_stdAlloc, 42, &a);
  mb_natural_set(MB_stdAlloc, 42, &expected);

  mb_status s = mb_natural_distanceDigit(MB_stdAlloc, &a, 0, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

/* tests 0 as identity, but now we also test if it is comutative */
bool test_natural_distanceDigit_4(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  mb_natural_set(MB_stdAlloc, 0, &a);
  mb_natural_set(MB_stdAlloc, 42, &expected);

  mb_status s = mb_natural_distanceDigit(MB_stdAlloc, &a, 42, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// ensures operands are not modified
bool test_natural_distanceDigit_5(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  mb_natural_set(MB_stdAlloc, 314159, &a);
  mb_natural_set(MB_stdAlloc, 314158, &expected);

  mb_status s = mb_natural_distanceDigit(MB_stdAlloc, &a, 1, &out);
  checkStatus(s);

  return mb_natural_equalDigit(&a, 314159) && mb_natural_equalDigit(&out, 314158);
}
/* END: testing distanceDigit */

/* BEGIN: testing divDigit */
bool test_natural_divDigit_1(void) {
  mb_Natural A = mb_natural_empty();
  mb_natural_set(MB_stdAlloc, 5, &A);
  u32 B = 3;
  mb_Natural Q = mb_natural_empty();
  u32 R;

  mb_Natural exp_Q = mb_natural_empty();
  mb_natural_set(MB_stdAlloc, 1, &exp_Q);
  u32 exp_R = 2;

  mb_status s = mb_natural_divDigit(MB_stdAlloc, &A, B, &Q, &R);
  checkStatus(s);

  return R == exp_R && mb_natural_equal(&Q, &exp_Q);
}

bool test_natural_divDigit_2(void) {
  mb_Natural A = mb_natural_empty();
  mb_natural_set(MB_stdAlloc, 0, &A);
  u32 B = 11;
  mb_Natural Q = mb_natural_empty();
  u32 R;

  mb_Natural exp_Q = mb_natural_empty();
  mb_natural_set(MB_stdAlloc, 0, &exp_Q);
  u32 exp_R = 0;

  mb_status s = mb_natural_divDigit(MB_stdAlloc, &A, B, &Q, &R);
  checkStatus(s);

  return R == exp_R && mb_natural_equal(&Q, &exp_Q);
}

bool test_natural_divDigit_3(void) {
  mb_Natural A = mb_natural_empty();
  u32 A_DIGS[] = {999999999, 999999999};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))
  mb_natural_setVec(MB_stdAlloc, A_DIGS, A_DIGS_LEN, &A);

  u32 B = 9;
  mb_Natural Q = mb_natural_empty();
  u32 R;

  mb_Natural exp_Q = mb_natural_empty();
  u32 exp_Q_DIGS[] = {111111111, 111111111};
  #define exp_Q_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))
  mb_natural_setVec(MB_stdAlloc, exp_Q_DIGS, exp_Q_DIGS_LEN, &exp_Q);
  u32 exp_R = 0;

  mb_status s = mb_natural_divDigit(MB_stdAlloc, &A, B, &Q, &R);
  checkStatus(s);

  return R == exp_R && mb_natural_equal(&Q, &exp_Q);
}

// basically tests if divDigit respects the division theorem
bool test_natural_divDigit_4(void) {
  mb_Natural A = mb_natural_empty();
  mb_natural_set(MB_stdAlloc, 36, &A);

  u32 B = 1;
  mb_Natural Q = mb_natural_empty();
  u32 R;

  while (B < 36) {
    mb_status s = mb_natural_divDigit(MB_stdAlloc, &A, B, &Q, &R);
    checkStatus(s);

    if (B <= R) {
      return false;
    }
    B++;
  }
  return true;
}

// same thing as the previous test, but with more digits
bool test_natural_divDigit_5(void) {
  mb_Natural A = mb_natural_empty();
  u32 A_DIGS[] = {999999999, 999999999};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))
  mb_natural_setVec(MB_stdAlloc, A_DIGS, A_DIGS_LEN, &A);

  u32 B = 1;
  mb_Natural Q = mb_natural_empty();
  u32 R;

  while (B < 36) {
    mb_status s = mb_natural_divDigit(MB_stdAlloc, &A, B, &Q, &R);
    checkStatus(s);

    if (B <= R) {
      return false;
    }
    B++;
  }
  return true;
}

bool test_natural_divDigit_6(void) {
  mb_Natural A = mb_natural_empty();
  mb_natural_set(MB_stdAlloc, 5, &A);
  u32 B = 0;
  mb_Natural Q = mb_natural_empty();
  u32 R;
  mb_status s = mb_natural_divDigit(MB_stdAlloc, &A, B, &Q, &R);
  return s == MB_status_divisionByZero;
}
/* END: testing divDigit */

/* BEGIN: testing copy */
bool test_natural_copy_1(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_status s;

  s = mb_natural_set(MB_stdAlloc, 2222, &a);
  checkStatus(s);
  
  s = mb_natural_copy(MB_stdAlloc, &a, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &a);
}

bool test_natural_copy_2(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural out = mb_natural_empty();

  u32 A_DIGS[] = {999999999, 999999999};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))
  mb_natural_setVec(MB_stdAlloc, A_DIGS, A_DIGS_LEN, &a);
  
  mb_status s = mb_natural_copy(MB_stdAlloc, &a, &out);
  checkStatus(s);

  return mb_natural_equal(&a, &out);
}
/* END: testing copy*/

/* BEGIN: DRIVER CODE */
Tester tests[] = {
  {"test_natural_snprint_0", test_natural_snprint_0},
  {"test_natural_snprint_1", test_natural_snprint_1},
  {"test_natural_snprint_2", test_natural_snprint_2},
  {"test_natural_snprint_3", test_natural_snprint_3},

  {"test_natural_addDigit_0", test_natural_addDigit_0},
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
  {"test_natural_divDigit_3", test_natural_divDigit_3},
  {"test_natural_divDigit_4", test_natural_divDigit_4},
  {"test_natural_divDigit_5", test_natural_divDigit_5},
  {"test_natural_divDigit_6", test_natural_divDigit_6},

  {"test_natural_copy_1", test_natural_copy_1},
  {"test_natural_copy_2", test_natural_copy_2},
};
#define TEST_LEN (int)(sizeof(tests) / sizeof(tests[0]))

int main(void) {
  run_tests(tests, TEST_LEN);
}
/* END: DRIVER CODE */
