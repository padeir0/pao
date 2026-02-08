#include "../src/mb_natural.h"
#include "../src/mb_basicTypes.h"
#include "../src/mb_status.h"
#include "../src/mb_stdAlloc.h"
#include <stdio.h>
#include <strings.h>
#include "common.h"

char buffer[DEFAULT_SIZE];

// TODO: write tests for when the allocator fails
// TODO: write grow/shrink tests with addDigit and distanceDigit
// TODO: write grow/shrink tests with add and distance
// TODO: write grow/shrink tests with multDigit and divDigit
// TODO: write tests for snprint when buffer is too small
// TODO: write tests for snprint when buffer is exact fit
// TODO: write tests for snprint when buffer size is 1
// TODO: remove the usage of macros (DIGS_LEN) in inner scopes
// TODO: write a checker to verify if all numbers abides to the invariantes:
//           len == 0 represents zero
//           or len > 0 and:
//              digits[len - 1] != 0
//              all digits are < MB_natural_base
// TODO: write tests where both operands are 0
// TODO: write tests to assert failure when aliasing rules are violated
// TODO: write tests for divDigit: a/a = 1
// TODO: write tests for divDigit: a/1 = a
// TODO: improve tests by using an allocator that allows you to
//       check for number of allocations performed and leaks

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
  mb_Natural out = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  u32 A_DIGS[] = {999999999, 999999999};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))

  mb_natural_setVec(MB_stdAlloc, A_DIGS, A_DIGS_LEN, &a);
  
  u32 EXP_DIGS[A_DIGS_LEN+1] = {1, 0, 0};
  mb_natural_setVec(MB_stdAlloc, EXP_DIGS, A_DIGS_LEN+1, &expected);

  mb_Status s = mb_natural_addDigit(MB_stdAlloc, &a, 1, &out);
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

  mb_Status s = mb_natural_addDigit(MB_stdAlloc, &a, 1, &out);
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

  mb_Status s = mb_natural_addDigit(MB_stdAlloc, &a, 42, &out);
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

  mb_Status s = mb_natural_addDigit(MB_stdAlloc, &a, 0, &out);
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

  mb_Status s = mb_natural_addDigit(MB_stdAlloc, &a, 1, &out);
  checkStatus(s);

  return mb_natural_equalDigit(&a, 314159);
}
/* END: testing addDigit */

/* BEGIN: testing compare */
bool test_natural_compare_1(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural b = mb_natural_empty();
  mb_Order out;

  mb_natural_set(MB_stdAlloc, 42, &a);
  mb_natural_set(MB_stdAlloc, 42, &b);
  
  out  = mb_natural_compare(&a, &b);

  return out == MB_order_equal;
}

bool test_natural_compare_2(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural b = mb_natural_empty();
  mb_Order out;

  mb_natural_set(MB_stdAlloc, 1, &a);
  mb_natural_set(MB_stdAlloc, 100, &b);
  
  out  = mb_natural_compare(&a, &b);

  return out == MB_order_less;
}

bool test_natural_compare_3(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural b = mb_natural_empty();
  mb_Order out;

  mb_natural_set(MB_stdAlloc, 100, &a);
  mb_natural_set(MB_stdAlloc, 1, &b);
  
  out  = mb_natural_compare(&a, &b);

  return out == MB_order_greater;
}

bool test_natural_compare_4(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural b = mb_natural_empty();
  mb_Order out;

  u32 A_DIGS[] = {1, 0};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))
  mb_natural_setVec(MB_stdAlloc, A_DIGS, A_DIGS_LEN, &a);

  mb_natural_set(MB_stdAlloc, MB_natural_base - 1, &b);
  
  out  = mb_natural_compare(&a, &b);

  return out == MB_order_greater;
}

bool test_natural_compare_5(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural b = mb_natural_empty();
  mb_Order out;

  u32 A_DIGS[] = {127, 0, 0, 1};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))
  mb_natural_setVec(MB_stdAlloc, A_DIGS, A_DIGS_LEN, &a);

  u32 B_DIGS[] = {127, 0, 0, 0};
  #define B_DIGS_LEN (sizeof(B_DIGS) / sizeof(B_DIGS[0]))
  mb_natural_setVec(MB_stdAlloc, B_DIGS, B_DIGS_LEN, &b);
  
  out  = mb_natural_compare(&a, &b);

  return out == MB_order_greater;
}

bool test_natural_compare_6(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural b = mb_natural_empty();
  mb_Order out;

  u32 A_DIGS[] = {127, 0, 0, 0};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))
  mb_natural_setVec(MB_stdAlloc, A_DIGS, A_DIGS_LEN, &a);

  u32 B_DIGS[] = {127, 0, 0, 1};
  #define B_DIGS_LEN (sizeof(B_DIGS) / sizeof(B_DIGS[0]))
  mb_natural_setVec(MB_stdAlloc, B_DIGS, B_DIGS_LEN, &b);
  
  out  = mb_natural_compare(&a, &b);

  return out == MB_order_less;
}

bool test_natural_compare_7(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural b = mb_natural_empty();
  mb_Order out;

  u32 A_DIGS[] = {127, 128, 129, 130};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))
  mb_natural_setVec(MB_stdAlloc, A_DIGS, A_DIGS_LEN, &a);

  u32 B_DIGS[] = {127, 128, 129, 130};
  #define B_DIGS_LEN (sizeof(B_DIGS) / sizeof(B_DIGS[0]))
  mb_natural_setVec(MB_stdAlloc, B_DIGS, B_DIGS_LEN, &b);
  
  out  = mb_natural_compare(&a, &b);

  return out == MB_order_equal;
}
/* END: testing compare */

/* BEGIN: testing add */

// tests 0 as identity
bool test_natural_add_0a(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural b = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  mb_natural_set(MB_stdAlloc, 42, &a);
  mb_natural_set(MB_stdAlloc, 0, &b);
  mb_natural_set(MB_stdAlloc, 42, &expected);
  
  mb_Status s = mb_natural_add(MB_stdAlloc, &a, &b, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// tests 0 as identity, but places garbage in the out parameter
bool test_natural_add_0b(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural b = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  const u32 GARBAGE = 0xCAFE;
  mb_natural_set(MB_stdAlloc, GARBAGE, &out);

  mb_natural_set(MB_stdAlloc, 42, &a);
  mb_natural_set(MB_stdAlloc, 0, &b);
  mb_natural_set(MB_stdAlloc, 42, &expected);
  
  mb_Status s = mb_natural_add(MB_stdAlloc, &a, &b, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// tests addition of big numbers 
bool test_natural_add_1a(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural b = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  mb_natural_set(MB_stdAlloc, MB_natural_base - 1, &a);
  mb_natural_set(MB_stdAlloc, MB_natural_base - 1, &b);
  
  u32 EXP_DIGS[2] = {1, 999999998};
  mb_natural_setVec(MB_stdAlloc, EXP_DIGS, 2, &expected);

  mb_Status s = mb_natural_add(MB_stdAlloc, &a, &b, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// tests addition of big numbers, but places garbage at the outparam
bool test_natural_add_1b(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural b = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  const u32 GARBAGE = 0xCAFE;
  mb_natural_set(MB_stdAlloc, GARBAGE, &out);
  
  mb_natural_set(MB_stdAlloc, MB_natural_base - 1, &a);
  mb_natural_set(MB_stdAlloc, MB_natural_base - 1, &b);
  
  u32 EXP_DIGS[2] = {1, 999999998};
  mb_natural_setVec(MB_stdAlloc, EXP_DIGS, 2, &expected);

  mb_Status s = mb_natural_add(MB_stdAlloc, &a, &b, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// tests commutativity
bool test_natural_add_2(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural b = mb_natural_empty();
  mb_Natural out1 = mb_natural_empty();
  mb_Natural out2 = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  mb_natural_set(MB_stdAlloc, 42, &a);
  mb_natural_set(MB_stdAlloc, 17, &b);
  mb_natural_set(MB_stdAlloc, 59, &expected);

  mb_Status s = mb_natural_add(MB_stdAlloc, &a, &b, &out1);
  checkStatus(s);
  s = mb_natural_add(MB_stdAlloc, &b, &a, &out2);
  checkStatus(s);

  return mb_natural_equal(&out1, &expected) &&
         mb_natural_equal(&out2, &expected);
}
/* END: testing add */

/* BEGIN: testing multDigit */

// tests 0 as annihilator
bool test_natural_multDigit_1(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  mb_natural_set(MB_stdAlloc, 314159, &a);
  mb_natural_set(MB_stdAlloc, 0, &expected);

  mb_Status s = mb_natural_multDigit(MB_stdAlloc, &a, 0, &out);
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

  mb_Status s = mb_natural_multDigit(MB_stdAlloc, &a, 314159, &out);
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

  mb_Status s = mb_natural_multDigit(MB_stdAlloc, &a, 1, &out);
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

  mb_Status s = mb_natural_multDigit(MB_stdAlloc, &a, 314159, &out);
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

    mb_Status s = mb_natural_multDigit(MB_stdAlloc, &a, 2, &out);
    checkStatus(s);
    if (!mb_natural_equal(&out, &expected)) {
      return false;
    }
  }

  {
    mb_natural_set(MB_stdAlloc, 2, &a);
    mb_natural_set(MB_stdAlloc, 4, &expected);

    mb_Status s = mb_natural_multDigit(MB_stdAlloc, &a, 2, &out);
    checkStatus(s);
    if (!mb_natural_equal(&out, &expected)) {
      return false;
    }
  }

  {
    mb_natural_set(MB_stdAlloc, 4, &a);
    mb_natural_set(MB_stdAlloc, 8, &expected);

    mb_Status s = mb_natural_multDigit(MB_stdAlloc, &a, 2, &out);
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

    mb_Status s = mb_natural_multDigit(MB_stdAlloc, &a, 12, &out);
    checkStatus(s);
    if (!mb_natural_equal(&out, &expected)) {
      return false;
    }
  }

  {
    mb_natural_set(MB_stdAlloc, 1111, &a);
    mb_natural_set(MB_stdAlloc, 3702963, &expected);

    mb_Status s = mb_natural_multDigit(MB_stdAlloc, &a, 3333, &out);
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

    mb_Status s = mb_natural_multDigit(MB_stdAlloc, &a, MB_natural_base-1, &out);
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

  mb_Status s = mb_natural_multDigit(MB_stdAlloc, &a, 4, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected) && mb_natural_equalDigit(&a, 4);
}

/* END: testing multDigit */

/* BEGIN: testing multBase*/
bool test_natural_multBase_1(void) {
  mb_Status s;
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
  mb_Status s;
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

  mb_Status s = mb_natural_distanceDigit(MB_stdAlloc, &a, 1, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

bool test_natural_distanceDigit_2(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  mb_natural_set(MB_stdAlloc, 1, &a);
  mb_natural_set(MB_stdAlloc, 0, &expected);

  mb_Status s = mb_natural_distanceDigit(MB_stdAlloc, &a, 1, &out);
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

  mb_Status s = mb_natural_distanceDigit(MB_stdAlloc, &a, 0, &out);
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

  mb_Status s = mb_natural_distanceDigit(MB_stdAlloc, &a, 42, &out);
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

  mb_Status s = mb_natural_distanceDigit(MB_stdAlloc, &a, 1, &out);
  checkStatus(s);

  return mb_natural_equalDigit(&a, 314159) && mb_natural_equalDigit(&out, 314158);
}
/* END: testing distanceDigit */

/* BEGIN: testing distance */

// tests 0 as identity
bool test_natural_distance_0(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural b = mb_natural_empty();
  mb_Natural out1 = mb_natural_empty();
  mb_Natural out2 = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  u32 A_DIGS[] = {1, 0, 0};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))
  mb_natural_setVec(MB_stdAlloc, A_DIGS, A_DIGS_LEN, &a);

  mb_natural_set(MB_stdAlloc, 0, &b);
  
  u32 EXP_DIGS[A_DIGS_LEN] = {1, 0, 0};
  mb_natural_setVec(MB_stdAlloc, EXP_DIGS, A_DIGS_LEN, &expected);

  mb_Status s = mb_natural_distance(MB_stdAlloc, &a, &b, &out1);
  checkStatus(s);
  s = mb_natural_distance(MB_stdAlloc, &b, &a, &out2);
  checkStatus(s);

  return mb_natural_equal(&out1, &expected) &&
         mb_natural_equal(&out2, &expected);
}

// tests if |a - a| = 0
bool test_natural_distance_1(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  u32 A_DIGS[] = {1, 0, 0};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))
  mb_natural_setVec(MB_stdAlloc, A_DIGS, A_DIGS_LEN, &a);
  
  mb_natural_set(MB_stdAlloc, 0, &expected);

  mb_Status s = mb_natural_distance(MB_stdAlloc, &a, &a, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// tests commutativity
bool test_natural_distance_2(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural b = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();
  mb_Natural out1 = mb_natural_empty();
  mb_Natural out2 = mb_natural_empty();

  mb_natural_set(MB_stdAlloc, 42, &a);
  mb_natural_set(MB_stdAlloc, 17, &b);
  mb_natural_set(MB_stdAlloc, 25, &expected);

  mb_Status s = mb_natural_distance(MB_stdAlloc, &a, &b, &out1);
  checkStatus(s);
  s = mb_natural_distance(MB_stdAlloc, &b, &a, &out2);
  checkStatus(s);

  return mb_natural_equal(&out1, &expected) &&
         mb_natural_equal(&out2, &expected);
}

// tests big numbers
bool test_natural_distance_3(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural b = mb_natural_empty();
  mb_Natural out1 = mb_natural_empty();
  mb_Natural out2 = mb_natural_empty();
  mb_Natural expected = mb_natural_empty();

  u32 A_DIGS[] = {1, 0, 42};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))
  mb_natural_setVec(MB_stdAlloc, A_DIGS, A_DIGS_LEN, &a);

  mb_natural_set(MB_stdAlloc, 42, &b);
  
  u32 EXP_DIGS[A_DIGS_LEN] = {1, 0, 0};
  #define EXP_DIGS_LEN (sizeof(EXP_DIGS) / sizeof(EXP_DIGS[0]))
  mb_natural_setVec(MB_stdAlloc, EXP_DIGS, EXP_DIGS_LEN, &expected);

  mb_Status s = mb_natural_distance(MB_stdAlloc, &a, &b, &out1);
  checkStatus(s);
  s = mb_natural_distance(MB_stdAlloc, &b, &a, &out2);
  checkStatus(s);

  return mb_natural_equal(&out1, &expected) &&
         mb_natural_equal(&out2, &expected);
}
/* END: testing distance */

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

  mb_Status s = mb_natural_divDigit(MB_stdAlloc, &A, B, &Q, &R);
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

  mb_Status s = mb_natural_divDigit(MB_stdAlloc, &A, B, &Q, &R);
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

  mb_Status s = mb_natural_divDigit(MB_stdAlloc, &A, B, &Q, &R);
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
    mb_Status s = mb_natural_divDigit(MB_stdAlloc, &A, B, &Q, &R);
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
    mb_Status s = mb_natural_divDigit(MB_stdAlloc, &A, B, &Q, &R);
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
  mb_Status s = mb_natural_divDigit(MB_stdAlloc, &A, B, &Q, &R);
  return s == MB_status_divisionByZero;
}
/* END: testing divDigit */

/* BEGIN: testing copy */
bool test_natural_copy_1(void) {
  mb_Natural a = mb_natural_empty();
  mb_Natural out = mb_natural_empty();
  mb_Status s;

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
  
  mb_Status s = mb_natural_copy(MB_stdAlloc, &a, &out);
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

  {"test_natural_add_0a", test_natural_add_0a},
  {"test_natural_add_0b", test_natural_add_0b},
  {"test_natural_add_1a", test_natural_add_1a},
  {"test_natural_add_1b", test_natural_add_1b},
  {"test_natural_add_2", test_natural_add_2},

  {"test_natural_compare_1", test_natural_compare_1},
  {"test_natural_compare_2", test_natural_compare_2},
  {"test_natural_compare_3", test_natural_compare_3},
  {"test_natural_compare_4", test_natural_compare_4},
  {"test_natural_compare_5", test_natural_compare_5},
  {"test_natural_compare_6", test_natural_compare_6},
  {"test_natural_compare_7", test_natural_compare_7},

  {"test_natural_distance_0", test_natural_distance_0},
  {"test_natural_distance_1", test_natural_distance_1},
  {"test_natural_distance_2", test_natural_distance_2},
  {"test_natural_distance_3", test_natural_distance_3},
};
#define TEST_LEN (int)(sizeof(tests) / sizeof(tests[0]))

int main(void) {
  run_tests(tests, TEST_LEN);
}
/* END: DRIVER CODE */
