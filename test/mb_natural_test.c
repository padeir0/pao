#include "../src/mb_natural.h"
#include "../src/mb_basicTypes.h"
#include "../src/mb_status.h"
#include "../src/mb_stdAlloc.h"
#include <stdio.h>
#include <strings.h>
#include "common.h"

char buffer[DEFAULT_SIZE];

// TODO: check if numbers are valid after each operation (using the function above)
// TODO: remove the usage of macros (DIGS_LEN) in inner scopes
// TODO: properly free all numbers at the end of each test
// TODO: improve tests by using an allocator that allows you to
//       check for number of allocations performed and leaks
// TODO: write tests for when the allocator fails
// TODO: write tests for snprint when buffer is too small
// TODO: write tests for snprint when buffer is exact fit
// TODO: write tests for snprint when buffer size is 1

#define alloc MB_stdAlloc

bool isAllFree(void) {
  return alloc->info(alloc->heap).used == 0;
}

bool isValidNumber(mb_Natural* n) {
  if (n->len == 0) {
    return true;
  }

  // tests for invalid zero and invalid leading zeroes
  if (n->digits[n->len-1] == 0) {
    return false;
  }

  // invalid digits
  u32 i = 0;
  while (i < n->len) {
    if (n->digits[i] >= MB_natural_base) {
      return false;
    }
    i++;
  }
  return true;
}

void printNat(mb_Natural* n) {
  usize written = mb_natural_snprint(n, buffer, DEFAULT_SIZE);
  if (written == 0) {
    printf("written 0 bytes.\n");
  }
  printf("%.*s", (int)written, buffer);
  printf(" (length: %d, cap: %d)\n", n->len, n->cap);
}

/* BEGIN: testing addDigit*/
bool test_natural_addDigit_0(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  u32 A_DIGS[] = {999999999, 999999999};
  s = mb_natural_setVec(alloc, A_DIGS, 2, &a); checkStatus(s);
  u32 EXP_DIGS[] = {1, 0, 0};
  s = mb_natural_setVec(alloc, EXP_DIGS, 3, &expected); checkStatus(s);

  s = mb_natural_addDigit(alloc, &a, 1, &out); checkStatus(s);
  bool ok = mb_natural_equal(&out, &expected) &&
            isValidNumber(&out);

  mb_natural_free(alloc, a);
  mb_natural_free(alloc, out);
  mb_natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests carry
bool test_natural_addDigit_1(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  u32 A_DIGS[] = {999999999, 999999999};
  s = mb_natural_setVec(alloc, A_DIGS, 2, &a); checkStatus(s);
  u32 EXP_DIGS[3] = {1, 0, 0};
  s = mb_natural_setVec(alloc, EXP_DIGS, 3, &expected); checkStatus(s);

  s = mb_natural_addDigit(alloc, &a, 1, &out); checkStatus(s);
  bool ok = mb_natural_equal(&out, &expected) &&
            isValidNumber(&out);

  mb_natural_free(alloc, a);
  mb_natural_free(alloc, out);
  mb_natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests 0 as identity
bool test_natural_addDigit_2(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 0, &a); checkStatus(s);
  s = mb_natural_set(alloc, 42, &expected); checkStatus(s);

  s = mb_natural_addDigit(alloc, &a, 42, &out); checkStatus(s);
  bool ok = mb_natural_equal(&out, &expected) &&
            isValidNumber(&out);

  mb_natural_free(alloc, a);
  mb_natural_free(alloc, out);
  mb_natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests 0 as identity
bool test_natural_addDigit_3(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 314159, &a); checkStatus(s);
  s = mb_natural_set(alloc, 314159, &expected); checkStatus(s);

  s = mb_natural_addDigit(alloc, &a, 0, &out); checkStatus(s);
  bool ok = mb_natural_equal(&out, &expected) &&
            isValidNumber(&out);

  mb_natural_free(alloc, a);
  mb_natural_free(alloc, out);
  mb_natural_free(alloc, expected);

  return ok && isAllFree();
}

// ensures operands are not modified
bool test_natural_addDigit_4(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 314159, &a); checkStatus(s);
  s = mb_natural_set(alloc, 314160, &expected); checkStatus(s);

  s = mb_natural_addDigit(alloc, &a, 1, &out); checkStatus(s);
  bool ok = mb_natural_equalDigit(&a, 314159) &&
            isValidNumber(&a) && isValidNumber(&out);

  mb_natural_free(alloc, a);
  mb_natural_free(alloc, out);
  mb_natural_free(alloc, expected);

  return ok && isAllFree();
}

// sets garbage to the outparam.
bool test_natural_addDigit_5(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 314159, &a); checkStatus(s);
  s = mb_natural_set(alloc, 999999, &out); checkStatus(s);
  s = mb_natural_set(alloc, 314160, &expected); checkStatus(s);

  s = mb_natural_addDigit(alloc, &a, 1, &out); checkStatus(s);
  bool ok = mb_natural_equalDigit(&out, 314160) &&
            isValidNumber(&out);

  mb_natural_free(alloc, a);
  mb_natural_free(alloc, out);
  mb_natural_free(alloc, expected);

  return ok && isAllFree();
}
/* END: testing addDigit */

/* BEGIN: testing compare */
bool test_natural_compare_1(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural b = mb_natural_new();
  mb_Order out;

  s = mb_natural_set(alloc, 42, &a); checkStatus(s);
  s = mb_natural_set(alloc, 42, &b); checkStatus(s);
  
  out = mb_natural_compare(&a, &b);

  mb_natural_free(alloc, a);
  mb_natural_free(alloc, b);

  return out == MB_order_equal && isAllFree();
}

bool test_natural_compare_2(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural b = mb_natural_new();
  mb_Order out;

  s = mb_natural_set(alloc, 1, &a); checkStatus(s);
  s = mb_natural_set(alloc, 100, &b); checkStatus(s);
  
  out  = mb_natural_compare(&a, &b);

  mb_natural_free(alloc, a);
  mb_natural_free(alloc, b);

  return out == MB_order_less && isAllFree();
}

bool test_natural_compare_3(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural b = mb_natural_new();
  mb_Order out;

  s = mb_natural_set(alloc, 100, &a); checkStatus(s);
  s = mb_natural_set(alloc, 1, &b); checkStatus(s);
  
  out  = mb_natural_compare(&a, &b);

  mb_natural_free(alloc, a);
  mb_natural_free(alloc, b);

  return out == MB_order_greater && isAllFree();
}

bool test_natural_compare_4(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural b = mb_natural_new();
  mb_Order out;

  u32 A_DIGS[] = {1, 0};
  s = mb_natural_setVec(alloc, A_DIGS, 2, &a); checkStatus(s);
  s = mb_natural_set(alloc, MB_natural_base - 1, &b); checkStatus(s);
  
  out = mb_natural_compare(&a, &b);

  mb_natural_free(alloc, a);
  mb_natural_free(alloc, b);

  return out == MB_order_greater && isAllFree();
}

bool test_natural_compare_5(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural b = mb_natural_new();
  mb_Order out;

  u32 A_DIGS[] = {127, 0, 0, 1};
  s = mb_natural_setVec(alloc, A_DIGS, 4, &a); checkStatus(s);
  u32 B_DIGS[] = {127, 0, 0, 0};
  s = mb_natural_setVec(alloc, B_DIGS, 4, &b); checkStatus(s);
  
  out = mb_natural_compare(&a, &b);

  mb_natural_free(alloc, a);
  mb_natural_free(alloc, b);

  return out == MB_order_greater && isAllFree();
}

bool test_natural_compare_6(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural b = mb_natural_new();
  mb_Order out;

  u32 A_DIGS[] = {127, 0, 0, 0};
  s = mb_natural_setVec(alloc, A_DIGS, 4, &a); checkStatus(s);
  u32 B_DIGS[] = {127, 0, 0, 1};
  s = mb_natural_setVec(alloc, B_DIGS, 4, &b); checkStatus(s);
  
  out = mb_natural_compare(&a, &b);

  mb_natural_free(alloc, a);
  mb_natural_free(alloc, b);

  return out == MB_order_less && isAllFree();
}

bool test_natural_compare_7(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural b = mb_natural_new();
  mb_Order out;

  u32 A_DIGS[] = {127, 128, 129, 130};
  s = mb_natural_setVec(alloc, A_DIGS, 4, &a); checkStatus(s);
  u32 B_DIGS[] = {127, 128, 129, 130};
  s = mb_natural_setVec(alloc, B_DIGS, 4, &b); checkStatus(s);
  
  out  = mb_natural_compare(&a, &b);

  mb_natural_free(alloc, a);
  mb_natural_free(alloc, b);

  return out == MB_order_equal && isAllFree();
}
/* END: testing compare */

/* BEGIN: testing compareDigit */
bool test_natural_compareDigit_1(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Order out;

  s = mb_natural_set(alloc, 42, &a); checkStatus(s);
  out = mb_natural_compareDigit(&a, 42);
  mb_natural_free(alloc, a);

  return out == MB_order_equal && isAllFree();
}

bool test_natural_compareDigit_2(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Order out;

  u32 A_DIGS[] = {127, 0, 0, 0};
  s = mb_natural_setVec(alloc, A_DIGS, 4, &a); checkStatus(s);
  out = mb_natural_compareDigit(&a, 1);
  mb_natural_free(alloc, a);

  return out == MB_order_greater && isAllFree();
}

bool test_natural_compareDigit_3(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Order out;

  u32 A_DIGS[] = {1, 0};
  s = mb_natural_setVec(alloc, A_DIGS, 2, &a); checkStatus(s);
  out = mb_natural_compareDigit(&a, MB_natural_base-1);
  mb_natural_free(alloc, a);

  return out == MB_order_greater && isAllFree();
}

bool test_natural_compareDigit_4(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Order out;

  s = mb_natural_set(alloc, 0, &a); checkStatus(s);
  out = mb_natural_compareDigit(&a, 0);
  mb_natural_free(alloc, a);

  return out == MB_order_equal && isAllFree();
}

bool test_natural_compareDigit_5(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Order out;

  s = mb_natural_set(alloc, 0, &a); checkStatus(s);
  out = mb_natural_compareDigit(&a, 1);
  mb_natural_free(alloc, a);

  return out == MB_order_less && isAllFree();
}

bool test_natural_compareDigit_6(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Order out;

  s = mb_natural_set(alloc, 5, &a); checkStatus(s);
  out = mb_natural_compareDigit(&a, 6);
  mb_natural_free(alloc, a);

  return out == MB_order_less && isAllFree();
}

bool test_natural_compareDigit_7(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Order out;

  s = mb_natural_set(alloc, 6, &a); checkStatus(s);
  out = mb_natural_compareDigit(&a, 5);
  mb_natural_free(alloc, a);

  return out == MB_order_greater && isAllFree();
}
/* END: testing compareDigit */

/* BEGIN: testing add */

// tests 0 as identity
bool test_natural_add_0a(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural b = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 42, &a); checkStatus(s);
  s = mb_natural_set(alloc, 0, &b); checkStatus(s);
  s = mb_natural_set(alloc, 42, &expected); checkStatus(s);
  
  s = mb_natural_add(alloc, &a, &b, &out); checkStatus(s);

  bool ok = mb_natural_equal(&out, &expected) &&
            isValidNumber(&out);

  mb_natural_free(alloc, a);
  mb_natural_free(alloc, b);
  mb_natural_free(alloc, out);
  mb_natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests 0 as identity, but places garbage in the out parameter
bool test_natural_add_0b(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural b = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  const u32 GARBAGE = 0xCAFE;
  s = mb_natural_set(alloc, GARBAGE, &out); checkStatus(s);
  s = mb_natural_set(alloc, 42, &a); checkStatus(s);
  s = mb_natural_set(alloc, 0, &b); checkStatus(s);
  s = mb_natural_set(alloc, 42, &expected); checkStatus(s);
  
  s = mb_natural_add(alloc, &a, &b, &out); checkStatus(s);

  bool ok = mb_natural_equal(&out, &expected) &&
            isValidNumber(&out);

  mb_natural_free(alloc, a);
  mb_natural_free(alloc, b);
  mb_natural_free(alloc, out);
  mb_natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests addition of big numbers 
bool test_natural_add_1a(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural b = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, MB_natural_base - 1, &a); checkStatus(s);
  s = mb_natural_set(alloc, MB_natural_base - 1, &b); checkStatus(s);
  
  u32 EXP_DIGS[2] = {1, 999999998};
  s = mb_natural_setVec(alloc, EXP_DIGS, 2, &expected); checkStatus(s);

  s = mb_natural_add(alloc, &a, &b, &out); checkStatus(s);
  bool ok = mb_natural_equal(&out, &expected) &&
            isValidNumber(&out);
  mb_natural_free(alloc, a);
  mb_natural_free(alloc, b);
  mb_natural_free(alloc, out);
  mb_natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests addition of big numbers, but places garbage at the outparam
bool test_natural_add_1b(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural b = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  const u32 GARBAGE = 0xCAFE;
  s = mb_natural_set(alloc, GARBAGE, &out); checkStatus(s);
  s = mb_natural_set(alloc, MB_natural_base - 1, &a); checkStatus(s);
  s = mb_natural_set(alloc, MB_natural_base - 1, &b); checkStatus(s);
  u32 EXP_DIGS[2] = {1, 999999998};
  s = mb_natural_setVec(alloc, EXP_DIGS, 2, &expected); checkStatus(s);

  s = mb_natural_add(alloc, &a, &b, &out); checkStatus(s);
  bool ok = mb_natural_equal(&out, &expected) &&
            isValidNumber(&out);
  mb_natural_free(alloc, a);
  mb_natural_free(alloc, b);
  mb_natural_free(alloc, out);
  mb_natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests commutativity
bool test_natural_add_2(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural b = mb_natural_new();
  mb_Natural out1 = mb_natural_new();
  mb_Natural out2 = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 42, &a); checkStatus(s);
  s = mb_natural_set(alloc, 17, &b); checkStatus(s);
  s = mb_natural_set(alloc, 59, &expected); checkStatus(s);

  s = mb_natural_add(alloc, &a, &b, &out1); checkStatus(s);
  s = mb_natural_add(alloc, &b, &a, &out2); checkStatus(s);

  bool ok = mb_natural_equal(&out1, &expected) &&
            mb_natural_equal(&out2, &expected) &&
            isValidNumber(&out1) &&
            isValidNumber(&out2);

  mb_natural_free(alloc, a);
  mb_natural_free(alloc, b);
  mb_natural_free(alloc, out1);
  mb_natural_free(alloc, out2);
  mb_natural_free(alloc, expected);

  return ok && isAllFree();
}
/* END: testing add */

/* BEGIN: testing multDigit */

// tests 0 as annihilator
bool test_natural_multDigit_1(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 314159, &a); checkStatus(s);
  s = mb_natural_set(alloc, 0, &expected); checkStatus(s);
  s = mb_natural_multDigit(alloc, &a, 0, &out); checkStatus(s);

  bool ok = mb_natural_equal(&out, &expected) &&
            isValidNumber(&out);

  mb_natural_free(alloc, a);
  mb_natural_free(alloc, out);
  mb_natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests 0 as annihilator
bool test_natural_multDigit_2(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 0, &a); checkStatus(s);
  s = mb_natural_set(alloc, 0, &expected); checkStatus(s);

  s = mb_natural_multDigit(alloc, &a, 314159, &out); checkStatus(s);

  bool ok = mb_natural_equal(&out, &expected) &&
            isValidNumber(&out);
  mb_natural_free(alloc, a);
  mb_natural_free(alloc, out);
  mb_natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests 1 as identity
bool test_natural_multDigit_3(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 314159, &a); checkStatus(s);
  s = mb_natural_set(alloc, 314159, &expected); checkStatus(s);

  s = mb_natural_multDigit(alloc, &a, 1, &out); checkStatus(s);

  bool ok = mb_natural_equal(&out, &expected) &&
            isValidNumber(&out);
  mb_natural_free(alloc, a);
  mb_natural_free(alloc, out);
  mb_natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests 1 as identity
bool test_natural_multDigit_4(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 1, &a); checkStatus(s);
  s = mb_natural_set(alloc, 314159, &expected); checkStatus(s);

  s = mb_natural_multDigit(alloc, &a, 314159, &out); checkStatus(s);

  bool ok = mb_natural_equal(&out, &expected) &&
            isValidNumber(&out);
  mb_natural_free(alloc, a);
  mb_natural_free(alloc, out);
  mb_natural_free(alloc, expected);

  return ok && isAllFree();
}

// TODO: continue the refactoring from here onwards

// tests powers of 2
bool test_natural_multDigit_5(void) {
  mb_Natural a = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  {
    mb_natural_set(alloc, 1, &a);
    mb_natural_set(alloc, 2, &expected);

    mb_Status s = mb_natural_multDigit(alloc, &a, 2, &out);
    checkStatus(s);
    if (!mb_natural_equal(&out, &expected)) {
      return false;
    }
  }

  {
    mb_natural_set(alloc, 2, &a);
    mb_natural_set(alloc, 4, &expected);

    mb_Status s = mb_natural_multDigit(alloc, &a, 2, &out);
    checkStatus(s);
    if (!mb_natural_equal(&out, &expected)) {
      return false;
    }
  }

  {
    mb_natural_set(alloc, 4, &a);
    mb_natural_set(alloc, 8, &expected);

    mb_Status s = mb_natural_multDigit(alloc, &a, 2, &out);
    checkStatus(s);
    if (!mb_natural_equal(&out, &expected)) {
      return false;
    }
  }
  return true;
}

// tests some other multiplications
bool test_natural_multDigit_6(void) {
  mb_Natural a = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  {
    mb_natural_set(alloc, 12, &a);
    mb_natural_set(alloc, 144, &expected);

    mb_Status s = mb_natural_multDigit(alloc, &a, 12, &out);
    checkStatus(s);
    if (!mb_natural_equal(&out, &expected)) {
      return false;
    }
  }

  {
    mb_natural_set(alloc, 1111, &a);
    mb_natural_set(alloc, 3702963, &expected);

    mb_Status s = mb_natural_multDigit(alloc, &a, 3333, &out);
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

    mb_natural_set(alloc, MB_natural_base-1, &a);
    mb_natural_setVec(alloc, digits, DILEN, &expected);

    mb_Status s = mb_natural_multDigit(alloc, &a, MB_natural_base-1, &out);
    checkStatus(s);
    if (!mb_natural_equal(&out, &expected)) {
      return false;
    }
  }
  return true;
}

// tests whether operands remain unchanged
bool test_natural_multDigit_7(void) {
  mb_Natural a = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  mb_natural_set(alloc, 4, &a);
  mb_natural_set(alloc, 16, &expected);

  mb_Status s = mb_natural_multDigit(alloc, &a, 4, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected) && mb_natural_equalDigit(&a, 4);
}

/* END: testing multDigit */

/* BEGIN: mult tests */

// tests 0 as annihilator (A = 0)
bool test_natural_mult_1(void) {
  mb_Status s;
  mb_Natural a   = mb_natural_new();
  mb_Natural b   = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 0,      &a);        checkStatus(s);
  s = mb_natural_set(alloc, 314159, &b);        checkStatus(s);
  s = mb_natural_set(alloc, 0,      &expected); checkStatus(s);

  s = mb_natural_mult(alloc, &a, &b, &out); checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// tests 0 as annihilator (B = 0)
bool test_natural_mult_2(void) {
  mb_Status s;
  mb_Natural a   = mb_natural_new();
  mb_Natural b   = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 314159, &a);        checkStatus(s);
  s = mb_natural_set(alloc, 0,      &b);        checkStatus(s);
  s = mb_natural_set(alloc, 0,      &expected); checkStatus(s);

  s = mb_natural_mult(alloc, &a, &b, &out); checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// tests 1 as identity (A = 1)
bool test_natural_mult_3(void) {
  mb_Status s;
  mb_Natural a   = mb_natural_new();
  mb_Natural b   = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 1,      &a);        checkStatus(s);
  s = mb_natural_set(alloc, 314159, &b);        checkStatus(s);
  s = mb_natural_set(alloc, 314159, &expected); checkStatus(s);

  s = mb_natural_mult(alloc, &a, &b, &out); checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// tests 1 as identity (B = 1)
bool test_natural_mult_4(void) {
  mb_Status s;
  mb_Natural a   = mb_natural_new();
  mb_Natural b   = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 314159, &a);        checkStatus(s);
  s = mb_natural_set(alloc, 1,      &b);        checkStatus(s);
  s = mb_natural_set(alloc, 314159, &expected); checkStatus(s);

  s = mb_natural_mult(alloc, &a, &b, &out); checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// tests commutativity
bool test_natural_mult_5(void) {
  mb_Status s;
  mb_Natural a    = mb_natural_new();
  mb_Natural b    = mb_natural_new();
  mb_Natural out1 = mb_natural_new();
  mb_Natural out2 = mb_natural_new();

  s = mb_natural_set(alloc, 12345, &a); checkStatus(s);
  s = mb_natural_set(alloc, 67890, &b); checkStatus(s);

  s = mb_natural_mult(alloc, &a, &b, &out1); checkStatus(s);
  s = mb_natural_mult(alloc, &b, &a, &out2); checkStatus(s);

  return mb_natural_equal(&out1, &out2);
}

// tests a carry that produces a new limb: (BASE-1) * (BASE-1)
bool test_natural_mult_6(void) {
  mb_Status s;
  mb_Natural a   = mb_natural_new();
  mb_Natural b   = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, MB_natural_base - 1, &a); checkStatus(s);
  s = mb_natural_set(alloc, MB_natural_base - 1, &b); checkStatus(s);

  u32 EXP_DIGS[] = {999999998, 000000001};
  s = mb_natural_setVec(alloc, EXP_DIGS, 2, &expected); checkStatus(s);

  s = mb_natural_mult(alloc, &a, &b, &out); checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// tests multi-limb * single-limb: [1, 0] * 2 = [2, 0], ie, BASE * 2 = 2*BASE
bool test_natural_mult_7(void) {
  mb_Status s;
  mb_Natural a   = mb_natural_new();
  mb_Natural b   = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  u32 A_DIGS[] = {1, 0};
  s = mb_natural_setVec(alloc, A_DIGS, 2, &a); checkStatus(s);
  s = mb_natural_set(alloc, 2, &b);             checkStatus(s);

  u32 EXP_DIGS[] = {2, 0};
  s = mb_natural_setVec(alloc, EXP_DIGS, 2, &expected); checkStatus(s);

  s = mb_natural_mult(alloc, &a, &b, &out); checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// tests multi-limb * multi-limb with carry propagation across limbs
// [1, 0] * [1, 0] = [1, 0, 0], ie BASE * BASE = BASE^2
bool test_natural_mult_8(void) {
  mb_Status s;
  mb_Natural a   = mb_natural_new();
  mb_Natural b   = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  u32 AB_DIGS[] = {1, 0};
  s = mb_natural_setVec(alloc, AB_DIGS, 2, &a); checkStatus(s);
  s = mb_natural_setVec(alloc, AB_DIGS, 2, &b); checkStatus(s);

  u32 EXP_DIGS[] = {1, 0, 0};
  s = mb_natural_setVec(alloc, EXP_DIGS, 3, &expected); checkStatus(s);

  s = mb_natural_mult(alloc, &a, &b, &out); checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// tests that operands are not modified
bool test_natural_mult_9(void) {
  mb_Status s;
  mb_Natural a   = mb_natural_new();
  mb_Natural b   = mb_natural_new();
  mb_Natural out = mb_natural_new();

  s = mb_natural_set(alloc, 6, &a); checkStatus(s);
  s = mb_natural_set(alloc, 7, &b); checkStatus(s);

  s = mb_natural_mult(alloc, &a, &b, &out); checkStatus(s);

  return mb_natural_equalDigit(&a, 6) && mb_natural_equalDigit(&b, 7);
}

// tests that garbage in the out-param does not affect the result
bool test_natural_mult_10(void) {
  mb_Status s;
  mb_Natural a   = mb_natural_new();
  mb_Natural b   = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  const u32 GARBAGE = 0xCAFEBABE % MB_natural_base;
  s = mb_natural_set(alloc, GARBAGE, &out);    checkStatus(s);
  s = mb_natural_set(alloc, 111,     &a);      checkStatus(s);
  s = mb_natural_set(alloc, 111,     &b);      checkStatus(s);
  s = mb_natural_set(alloc, 12321,   &expected); checkStatus(s);

  s = mb_natural_mult(alloc, &a, &b, &out); checkStatus(s);

  return mb_natural_equal(&out, &expected);
}
/* END: mult tests */

/* BEGIN: testing multBase*/
bool test_natural_multBase_1(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  u32 digits[] = {
    000000001, 000000000
  };
  #define DILEN (sizeof(digits) / sizeof(digits[0]))
  s = mb_natural_set(alloc, 1, &a);
  checkStatus(s);
  s = mb_natural_setVec(alloc, digits, DILEN, &expected);
  checkStatus(s);
  s = mb_natural_multBase(alloc, &a);
  checkStatus(s);

  return mb_natural_equal(&a, &expected);
}

bool test_natural_multBase_2(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  u32 digits[] = {
    900000000, 000000000, 000000000
  };
  #define DILEN (sizeof(digits) / sizeof(digits[0]))
  s = mb_natural_set(alloc, 900000000, &a);
  checkStatus(s);
  s = mb_natural_setVec(alloc, digits, DILEN, &expected);
  checkStatus(s);
  s = mb_natural_multBase(alloc, &a);
  checkStatus(s);
  s = mb_natural_multBase(alloc, &a);
  checkStatus(s);

  return mb_natural_equal(&a, &expected);
}

/* END: testing multBase*/

/* BEGIN: testing snprint */
char test_buffer[DEFAULT_SIZE];

bool test_natural_snprint_0(void) {
  mb_Natural A = mb_natural_new();
  mb_natural_set(alloc, 1, &A);

  usize written = mb_natural_snprint(&A, test_buffer, DEFAULT_SIZE);
  if (written == 0) {
    return false;
  }

  if (strncmp("1", test_buffer, written) != 0) {
    return false;
  }
  return true;
}

bool test_natural_snprint_1(void) {
  mb_Natural A = mb_natural_new();

  u32 digits[] = {
    1,0,0
  };
  #define DILEN (sizeof(digits) / sizeof(digits[0]))

  mb_natural_setVec(alloc, digits, DILEN, &A);

  usize written = mb_natural_snprint(&A, test_buffer, DEFAULT_SIZE);
  if (written == 0) {
    return false;
  }

  if (strncmp("1000000000000000000", test_buffer, written) != 0) {
    return false;
  }
  return true;
}

bool test_natural_snprint_2(void) {
  mb_Natural A = mb_natural_new();

  mb_natural_set(alloc, 314159, &A);

  usize written = mb_natural_snprint(&A, test_buffer, DEFAULT_SIZE);
  if (written == 0) {
    return false;
  }

  if (strncmp("314159", test_buffer, written) != 0) {
    return false;
  }
  return true;
}

bool test_natural_snprint_3(void) {
  mb_Natural A = mb_natural_new();

  mb_natural_set(alloc, 0, &A);

  usize written = mb_natural_snprint(&A, test_buffer, DEFAULT_SIZE);
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
  mb_Natural a = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  u32 A_DIGS[] = {1, 0, 0};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))
  mb_natural_setVec(alloc, A_DIGS, A_DIGS_LEN, &a);
  
  u32 EXP_DIGS[A_DIGS_LEN-1] = {999999999, 999999999};
  mb_natural_setVec(alloc, EXP_DIGS, A_DIGS_LEN-1, &expected);

  mb_Status s = mb_natural_distanceDigit(alloc, &a, 1, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

bool test_natural_distanceDigit_2(void) {
  mb_Natural a = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  mb_natural_set(alloc, 1, &a);
  mb_natural_set(alloc, 0, &expected);

  mb_Status s = mb_natural_distanceDigit(alloc, &a, 1, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

/* test 0 as identity */
bool test_natural_distanceDigit_3(void) {
  mb_Natural a = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  mb_natural_set(alloc, 42, &a);
  mb_natural_set(alloc, 42, &expected);

  mb_Status s = mb_natural_distanceDigit(alloc, &a, 0, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

/* tests 0 as identity, but now we also test if it is comutative */
bool test_natural_distanceDigit_4(void) {
  mb_Natural a = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  mb_natural_set(alloc, 0, &a);
  mb_natural_set(alloc, 42, &expected);

  mb_Status s = mb_natural_distanceDigit(alloc, &a, 42, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// ensures operands are not modified
bool test_natural_distanceDigit_5(void) {
  mb_Natural a = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  mb_natural_set(alloc, 314159, &a);
  mb_natural_set(alloc, 314158, &expected);

  mb_Status s = mb_natural_distanceDigit(alloc, &a, 1, &out);
  checkStatus(s);

  return mb_natural_equalDigit(&a, 314159) && mb_natural_equalDigit(&out, 314158);
}
/* END: testing distanceDigit */

/* BEGIN: testing distance */

// tests 0 as identity
bool test_natural_distance_0(void) {
  mb_Natural a = mb_natural_new();
  mb_Natural b = mb_natural_new();
  mb_Natural out1 = mb_natural_new();
  mb_Natural out2 = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  u32 A_DIGS[] = {1, 0, 0};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))
  mb_natural_setVec(alloc, A_DIGS, A_DIGS_LEN, &a);

  mb_natural_set(alloc, 0, &b);
  
  u32 EXP_DIGS[A_DIGS_LEN] = {1, 0, 0};
  mb_natural_setVec(alloc, EXP_DIGS, A_DIGS_LEN, &expected);

  mb_Status s = mb_natural_distance(alloc, &a, &b, &out1);
  checkStatus(s);
  s = mb_natural_distance(alloc, &b, &a, &out2);
  checkStatus(s);

  return mb_natural_equal(&out1, &expected) &&
         mb_natural_equal(&out2, &expected);
}

// tests if |a - a| = 0
bool test_natural_distance_1(void) {
  mb_Natural a = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  u32 A_DIGS[] = {1, 0, 0};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))
  mb_natural_setVec(alloc, A_DIGS, A_DIGS_LEN, &a);
  
  mb_natural_set(alloc, 0, &expected);

  mb_Status s = mb_natural_distance(alloc, &a, &a, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// tests commutativity
bool test_natural_distance_2(void) {
  mb_Natural a = mb_natural_new();
  mb_Natural b = mb_natural_new();
  mb_Natural expected = mb_natural_new();
  mb_Natural out1 = mb_natural_new();
  mb_Natural out2 = mb_natural_new();

  mb_natural_set(alloc, 42, &a);
  mb_natural_set(alloc, 17, &b);
  mb_natural_set(alloc, 25, &expected);

  mb_Status s = mb_natural_distance(alloc, &a, &b, &out1);
  checkStatus(s);
  s = mb_natural_distance(alloc, &b, &a, &out2);
  checkStatus(s);

  return mb_natural_equal(&out1, &expected) &&
         mb_natural_equal(&out2, &expected);
}

// tests big numbers
bool test_natural_distance_3(void) {
  mb_Natural a = mb_natural_new();
  mb_Natural b = mb_natural_new();
  mb_Natural out1 = mb_natural_new();
  mb_Natural out2 = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  u32 A_DIGS[] = {1, 0, 42};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))
  mb_natural_setVec(alloc, A_DIGS, A_DIGS_LEN, &a);

  mb_natural_set(alloc, 42, &b);
  
  u32 EXP_DIGS[A_DIGS_LEN] = {1, 0, 0};
  #define EXP_DIGS_LEN (sizeof(EXP_DIGS) / sizeof(EXP_DIGS[0]))
  mb_natural_setVec(alloc, EXP_DIGS, EXP_DIGS_LEN, &expected);

  mb_Status s = mb_natural_distance(alloc, &a, &b, &out1);
  checkStatus(s);
  s = mb_natural_distance(alloc, &b, &a, &out2);
  checkStatus(s);

  return mb_natural_equal(&out1, &expected) &&
         mb_natural_equal(&out2, &expected);
}
/* END: testing distance */

/* BEGIN: testing divDigit */
bool test_natural_divDigit_1(void) {
  mb_Natural A = mb_natural_new();
  mb_natural_set(alloc, 5, &A);
  u32 B = 3;
  mb_Natural Q = mb_natural_new();
  u32 R;

  mb_Natural exp_Q = mb_natural_new();
  mb_natural_set(alloc, 1, &exp_Q);
  u32 exp_R = 2;

  mb_Status s = mb_natural_divDigit(alloc, &A, B, &Q, &R);
  checkStatus(s);

  return R == exp_R && mb_natural_equal(&Q, &exp_Q);
}

bool test_natural_divDigit_2(void) {
  mb_Natural A = mb_natural_new();
  mb_natural_set(alloc, 0, &A);
  u32 B = 11;
  mb_Natural Q = mb_natural_new();
  u32 R;

  mb_Natural exp_Q = mb_natural_new();
  mb_natural_set(alloc, 0, &exp_Q);
  u32 exp_R = 0;

  mb_Status s = mb_natural_divDigit(alloc, &A, B, &Q, &R);
  checkStatus(s);

  return R == exp_R && mb_natural_equal(&Q, &exp_Q);
}

bool test_natural_divDigit_3(void) {
  mb_Natural A = mb_natural_new();
  u32 A_DIGS[] = {999999999, 999999999};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))
  mb_natural_setVec(alloc, A_DIGS, A_DIGS_LEN, &A);

  u32 B = 9;
  mb_Natural Q = mb_natural_new();
  u32 R;

  mb_Natural exp_Q = mb_natural_new();
  u32 exp_Q_DIGS[] = {111111111, 111111111};
  #define exp_Q_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))
  mb_natural_setVec(alloc, exp_Q_DIGS, exp_Q_DIGS_LEN, &exp_Q);
  u32 exp_R = 0;

  mb_Status s = mb_natural_divDigit(alloc, &A, B, &Q, &R);
  checkStatus(s);

  return R == exp_R && mb_natural_equal(&Q, &exp_Q);
}

// basically tests if divDigit respects the division theorem
bool test_natural_divDigit_4(void) {
  mb_Natural A = mb_natural_new();
  mb_natural_set(alloc, 36, &A);

  u32 B = 1;
  mb_Natural Q = mb_natural_new();
  u32 R;

  while (B < 36) {
    mb_Status s = mb_natural_divDigit(alloc, &A, B, &Q, &R);
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
  mb_Natural A = mb_natural_new();
  u32 A_DIGS[] = {999999999, 999999999};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))
  mb_natural_setVec(alloc, A_DIGS, A_DIGS_LEN, &A);

  u32 B = 1;
  mb_Natural Q = mb_natural_new();
  u32 R;

  while (B < 36) {
    mb_Status s = mb_natural_divDigit(alloc, &A, B, &Q, &R);
    checkStatus(s);

    if (B <= R) {
      return false;
    }
    B++;
  }
  return true;
}

bool test_natural_divDigit_6(void) {
  mb_Natural A = mb_natural_new();
  mb_natural_set(alloc, 5, &A);
  u32 B = 0;
  mb_Natural Q = mb_natural_new();
  u32 R;
  mb_Status s = mb_natural_divDigit(alloc, &A, B, &Q, &R);
  return s == MB_status_divisionByZero;
}

// tests 1 as identity
bool test_natural_divDigit_7(void) {
  mb_Natural A = mb_natural_new();
  mb_natural_set(alloc, 42, &A);
  u32 B = 1;
  mb_Natural Q = mb_natural_new();
  u32 R;

  mb_Natural exp_Q = mb_natural_new();
  mb_natural_set(alloc, 42, &exp_Q);
  u32 exp_R = 0;

  mb_Status s = mb_natural_divDigit(alloc, &A, B, &Q, &R);
  checkStatus(s);

  return R == exp_R && mb_natural_equal(&Q, &exp_Q);
}

// tests a/a = 1*a + 0
bool test_natural_divDigit_8(void) {
  mb_Natural A = mb_natural_new();
  mb_natural_set(alloc, 42, &A);
  u32 B = 42;
  mb_Natural Q = mb_natural_new();
  u32 R;

  mb_Natural exp_Q = mb_natural_new();
  mb_natural_set(alloc, 1, &exp_Q);
  u32 exp_R = 0;

  mb_Status s = mb_natural_divDigit(alloc, &A, B, &Q, &R);
  checkStatus(s);

  return R == exp_R && mb_natural_equal(&Q, &exp_Q);
}

/* END: testing divDigit */

/* BEGIN: testing div */
bool test_natural_div_1(void) {
  mb_Status s;
  mb_Natural A = mb_natural_new();
  mb_Natural B = mb_natural_new();
  mb_Natural Q = mb_natural_new();
  mb_Natural R = mb_natural_new();
  mb_Natural scratch = mb_natural_new();
  mb_Natural exp_Q = mb_natural_new();
  mb_Natural exp_R = mb_natural_new();

  s = mb_natural_set(alloc, 5, &A);     checkStatus(s);
  s = mb_natural_set(alloc, 1, &exp_Q); checkStatus(s);
  s = mb_natural_set(alloc, 3, &B);     checkStatus(s);
  s = mb_natural_set(alloc, 2, &exp_R); checkStatus(s);

  s = mb_natural_div(alloc, &scratch, &A, &B, &Q, &R); checkStatus(s);

  return mb_natural_equal(&R, &exp_R) && mb_natural_equal(&Q, &exp_Q);
}

bool test_natural_div_2(void) {
  mb_Status s;
  mb_Natural A = mb_natural_new();
  mb_Natural B = mb_natural_new();
  mb_Natural Q = mb_natural_new();
  mb_Natural R = mb_natural_new();
  mb_Natural scratch = mb_natural_new();
  mb_Natural exp_Q = mb_natural_new();
  mb_Natural exp_R = mb_natural_new();

  s = mb_natural_set(alloc, 0,  &A);    checkStatus(s);
  s = mb_natural_set(alloc, 11, &B);    checkStatus(s);
  s = mb_natural_set(alloc, 0, &exp_Q); checkStatus(s);
  s = mb_natural_set(alloc, 0, &exp_R); checkStatus(s);

  s = mb_natural_div(alloc, &scratch, &A, &B, &Q, &R); checkStatus(s);

  return mb_natural_equal(&R, &exp_R) && mb_natural_equal(&Q, &exp_Q);
}

bool test_natural_div_3(void) {
  mb_Status s;
  mb_Natural A = mb_natural_new();
  mb_Natural B = mb_natural_new();
  mb_Natural Q = mb_natural_new();
  mb_Natural R = mb_natural_new();
  mb_Natural scratch = mb_natural_new();
  mb_Natural exp_Q = mb_natural_new();
  mb_Natural exp_R = mb_natural_new();

  u32 A_DIGS[] = {999999999, 999999999};
  s = mb_natural_setVec(alloc, A_DIGS, 2, &A); checkStatus(s);
  s = mb_natural_set(alloc, 9, &B); checkStatus(s);
  u32 exp_Q_DIGS[] = {111111111, 111111111};
  mb_natural_setVec(alloc, exp_Q_DIGS, 2, &exp_Q);

  s = mb_natural_div(alloc, &scratch, &A, &B, &Q, &R); checkStatus(s);

  return mb_natural_equal(&R, &exp_R) && mb_natural_equal(&Q, &exp_Q);
}

// basically tests if divDigit respects the division theorem
bool test_natural_div_4(void) {
  mb_Status s;
  mb_Natural A = mb_natural_new();
  mb_Natural B = mb_natural_new();
  mb_Natural Q = mb_natural_new();
  mb_Natural R = mb_natural_new();
  mb_Natural scratch = mb_natural_new();

  s = mb_natural_set(alloc, 36, &A); checkStatus(s);
  s = mb_natural_set(alloc, 1, &B); checkStatus(s);

  while (mb_natural_compareDigit(&B, 36) == MB_order_less) {
    s = mb_natural_div(alloc, &scratch, &A, &B, &Q, &R);
    checkStatus(s);

    if (!(mb_natural_compare(&R, &B) == MB_order_less)) {
      return false;
    }
    s = mb_natural_incByDigit(alloc, 1, &B); checkStatus(s);
  }
  return true;
}

// same thing as the previous test, but with more digits
bool test_natural_div_5(void) {
  mb_Status s;
  mb_Natural A = mb_natural_new();
  mb_Natural B = mb_natural_new();
  mb_Natural Q = mb_natural_new();
  mb_Natural R = mb_natural_new();
  mb_Natural scratch = mb_natural_new();

  u32 A_DIGS[] = {999999999, 999999999};
  s = mb_natural_setVec(alloc, A_DIGS, 2, &A); checkStatus(s);
  s = mb_natural_set(alloc, 1, &B); checkStatus(s);
  s = mb_natural_set(alloc, 0, &R); checkStatus(s);

  while (mb_natural_compareDigit(&B, MB_natural_base) == MB_order_less) {
    mb_Status s = mb_natural_div(alloc, &scratch, &A, &B, &Q, &R);
    checkStatus(s);

    if (!(mb_natural_compare(&R, &B) == MB_order_less)) {
      return false;
    }
    s = mb_natural_incByDigit(alloc, MB_natural_base/10, &B); checkStatus(s);
  }
  return true;
}

bool test_natural_div_6(void) {
  mb_Status s;
  mb_Natural A = mb_natural_new();
  mb_Natural B = mb_natural_new();
  mb_Natural Q = mb_natural_new();
  mb_Natural R = mb_natural_new();
  mb_Natural scratch = mb_natural_new();

  s = mb_natural_set(alloc, 5, &A); checkStatus(s);
  s = mb_natural_set(alloc, 0, &B); checkStatus(s);

  s = mb_natural_div(alloc, &scratch, &A, &B, &Q, &R);
  return s == MB_status_divisionByZero;
}

// tests 1 as identity
bool test_natural_div_7(void) {
  mb_Status s;
  mb_Natural A = mb_natural_new();
  mb_Natural B = mb_natural_new();
  mb_Natural Q = mb_natural_new();
  mb_Natural R = mb_natural_new();
  mb_Natural scratch = mb_natural_new();
  mb_Natural exp_Q = mb_natural_new();
  mb_Natural exp_R = mb_natural_new();

  s = mb_natural_set(alloc, 42, &A);     checkStatus(s);
  s = mb_natural_set(alloc, 1, &B);      checkStatus(s);
  s = mb_natural_set(alloc, 42, &exp_Q); checkStatus(s);
  s = mb_natural_set(alloc, 0, &exp_R); checkStatus(s);

  s = mb_natural_div(alloc, &scratch, &A, &B, &Q, &R); checkStatus(s);

  return mb_natural_equal(&R, &exp_R) && mb_natural_equal(&Q, &exp_Q);
}

// tests a/a = 1*a + 0
bool test_natural_div_8(void) {
  mb_Status s;
  mb_Natural A = mb_natural_new();
  mb_Natural B = mb_natural_new();
  mb_Natural Q = mb_natural_new();
  mb_Natural R = mb_natural_new();
  mb_Natural scratch = mb_natural_new();
  mb_Natural exp_Q = mb_natural_new();
  mb_Natural exp_R = mb_natural_new();

  s = mb_natural_set(alloc, 42, &A);    checkStatus(s);
  s = mb_natural_set(alloc, 42, &B);    checkStatus(s);
  s = mb_natural_set(alloc, 1, &exp_Q); checkStatus(s);
  s = mb_natural_set(alloc, 0, &exp_R); checkStatus(s);

  s = mb_natural_div(alloc, &scratch, &A, &B, &Q, &R); checkStatus(s);

  return mb_natural_equal(&R, &exp_R) && mb_natural_equal(&Q, &exp_Q);
}
/* END: testing div */

/* BEGIN: testing copy */
bool test_natural_copy_1(void) {
  mb_Natural a = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Status s;

  s = mb_natural_set(alloc, 2222, &a);
  checkStatus(s);
  
  s = mb_natural_copy(alloc, &a, &out);
  checkStatus(s);

  return mb_natural_equal(&out, &a);
}

bool test_natural_copy_2(void) {
  mb_Natural a = mb_natural_new();
  mb_Natural out = mb_natural_new();

  u32 A_DIGS[] = {999999999, 999999999};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))
  mb_natural_setVec(alloc, A_DIGS, A_DIGS_LEN, &a);
  
  mb_Status s = mb_natural_copy(alloc, &a, &out);
  checkStatus(s);

  return mb_natural_equal(&a, &out);
}

// must work even if has garbage in the out param
bool test_natural_copy_3(void) {
  mb_Status s;
  mb_Natural a = mb_natural_new();
  mb_Natural out = mb_natural_new();

  u32 A_DIGS[] = {999999999, 999999999};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))
  s = mb_natural_setVec(alloc, A_DIGS, A_DIGS_LEN, &a); checkStatus(s);
  s = mb_natural_set(alloc, 123456, &out); checkStatus(s);

  s = mb_natural_copy(alloc, &a, &out); checkStatus(s);

  return mb_natural_equal(&a, &out);
}
/* END: testing copy*/

/* BEGIN: grow/shrink tests */
// addDigit/distanceDigit
bool test_natural_growShrink_1(void) {
  mb_Natural A = mb_natural_new();
  u32 B = MB_natural_base-1;
  mb_Natural C = mb_natural_new();

  mb_Status st;
  st = mb_natural_set(alloc, 0, &A); checkStatus(st);
  st = mb_natural_set(alloc, 0, &C); checkStatus(st);;

  int i = 0;
  while (i < 100) {
    st = mb_natural_addDigit(alloc, &A, B, &C); checkStatus(st);
    st = mb_natural_copy(alloc, &C, &A); checkStatus(st);
    i++;
  }

  while (0 < i) {
    st = mb_natural_distanceDigit(alloc, &A, B, &C); checkStatus(st);
    st = mb_natural_copy(alloc, &C, &A); checkStatus(st);
    i--;
  }

  return mb_natural_isZero(&A);
}

// add/distance
bool test_natural_growShrink_2(void) {
  mb_Natural A = mb_natural_new();
  mb_Natural B = mb_natural_new();
  mb_Natural C = mb_natural_new();

  mb_Status st;
  st = mb_natural_set(alloc, 0, &A); checkStatus(st);
  st = mb_natural_set(alloc, MB_natural_base-1, &B); checkStatus(st);
  st = mb_natural_set(alloc, 0, &C); checkStatus(st);;

  int i = 0;
  while (i < 100) {
    st = mb_natural_add(alloc, &A, &B, &C); checkStatus(st);
    st = mb_natural_copy(alloc, &C, &A); checkStatus(st);
    i++;
  }

  while (0 < i) {
    st = mb_natural_distance(alloc, &A, &B, &C); checkStatus(st);
    st = mb_natural_copy(alloc, &C, &A); checkStatus(st);
    i--;
  }

  return mb_natural_isZero(&A);
}

// multDigit/divDigit
bool test_natural_growShrink_3(void) {
  mb_Natural A = mb_natural_new();
  u32 B = 8;
  mb_Natural C = mb_natural_new();
  u32 R = 0;
  const int maxIter = 16;

  mb_Status st;
  st = mb_natural_set(alloc, 1, &A); checkStatus(st);
  st = mb_natural_set(alloc, 1, &C); checkStatus(st);;

  int i = 0;
  while (i < maxIter) {
    st = mb_natural_multDigit(alloc, &A, B, &C); checkStatus(st);
    st = mb_natural_copy(alloc, &C, &A); checkStatus(st);
    i++;
  }

  while (0 < i) {
    st = mb_natural_divDigit(alloc, &A, B, &C, &R); checkStatus(st);
    st = mb_natural_copy(alloc, &C, &A); checkStatus(st);
    if (R != 0) {
      return false;
    }
    i--;
  }

  return mb_natural_equalDigit(&A, 1);
}

// incByDigit/decrByDigit
bool test_natural_growShrink_4(void) {
  u32 B = MB_natural_base-1;
  mb_Natural C = mb_natural_new();

  mb_Status st;
  st = mb_natural_set(alloc, 0, &C); checkStatus(st);

  int i = 0;
  while (i < 100) {
    st = mb_natural_incByDigit(alloc, B, &C); checkStatus(st);
    i++;
  }

  while (0 < i) {
    st = mb_natural_decrByDigit(alloc, B, &C); checkStatus(st);
    i--;
  }

  return mb_natural_isZero(&C);
}

// incBy/decrBy
bool test_natural_growShrink_5(void) {
  mb_Natural B = mb_natural_new();
  mb_Natural C = mb_natural_new();

  mb_Status st;
  st = mb_natural_set(alloc, MB_natural_base-1, &B); checkStatus(st);
  st = mb_natural_set(alloc, 0, &C); checkStatus(st);

  int i = 0;
  while (i < 100) {
    st = mb_natural_incBy(alloc, &B, &C); checkStatus(st);
    i++;
  }

  while (0 < i) {
    st = mb_natural_decrBy(alloc, &B, &C); checkStatus(st);
    i--;
  }

  return mb_natural_isZero(&C);
}

// multDigit/divDigit
bool test_natural_growShrink_6(void) {
  mb_Natural A = mb_natural_new();
  mb_Natural B = mb_natural_new();
  mb_Natural C = mb_natural_new();
  mb_Natural R = mb_natural_new();
  mb_Natural scratch = mb_natural_new();
  const int maxIter = 16;

  mb_Status st;
  st = mb_natural_set(alloc, 1, &A); checkStatus(st);
  st = mb_natural_set(alloc, 8, &B); checkStatus(st);
  st = mb_natural_set(alloc, 1, &C); checkStatus(st);;
  st = mb_natural_set(alloc, 0, &R); checkStatus(st);;

  int i = 0;
  while (i < maxIter) {
    st = mb_natural_mult(alloc, &A, &B, &C); checkStatus(st);
    st = mb_natural_copy(alloc, &C, &A); checkStatus(st);
    i++;
  }

  while (0 < i) {
    st = mb_natural_div(alloc, &scratch, &A, &B, &C, &R); checkStatus(st);
    st = mb_natural_copy(alloc, &C, &A); checkStatus(st);
    if (!mb_natural_isZero(&R)) {
      return false;
    }
    i--;
  }

  return mb_natural_equalDigit(&A, 1);
}
/* END: grow/shrink tests */

/* BEGIN: incByDigit tests*/

// tests 0 as identity
bool test_natural_incByDigit_0(void) {
  mb_Status s;
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 42, &out); checkStatus(s);
  s = mb_natural_set(alloc, 42, &expected); checkStatus(s);
  s = mb_natural_incByDigit(alloc, 0, &out); checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

bool test_natural_incByDigit_1(void) {
  mb_Status s;
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 42, &out); checkStatus(s);
  s = mb_natural_set(alloc, 84, &expected); checkStatus(s);
  s = mb_natural_incByDigit(alloc, 42, &out); checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

bool test_natural_incByDigit_2(void) {
  mb_Status s;
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  u32 A_DIGS[2] = {999999999, 999999999};
  s = mb_natural_setVec(alloc, A_DIGS, 2, &out); checkStatus(s);
  
  u32 EXP_DIGS[3] = {1, 0, 0};
  s = mb_natural_setVec(alloc, EXP_DIGS, 3, &expected); checkStatus(s);

  s = mb_natural_incByDigit(alloc, 1, &out); checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// tests 0 as identity (in out)
bool test_natural_incByDigit_3(void) {
  mb_Status s;
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 0, &out); checkStatus(s);
  s = mb_natural_set(alloc, 42, &expected); checkStatus(s);
  s = mb_natural_incByDigit(alloc, 42, &out); checkStatus(s);

  return mb_natural_equal(&out, &expected);
}
/* END: incByDigit tests*/

/* BEGIN: incBy tests*/

// tests 0 as identity
bool test_natural_incBy_0(void) {
  mb_Status s;
  mb_Natural out = mb_natural_new();
  mb_Natural b = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 42, &out); checkStatus(s);
  s = mb_natural_set(alloc, 0,  &b); checkStatus(s);
  s = mb_natural_set(alloc, 42, &expected); checkStatus(s);

  s = mb_natural_incBy(alloc, &b, &out); checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

bool test_natural_incBy_1(void) {
  mb_Status s;
  mb_Natural out = mb_natural_new();
  mb_Natural b = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 42, &out); checkStatus(s);
  s = mb_natural_set(alloc, 42, &b); checkStatus(s);
  s = mb_natural_set(alloc, 84, &expected); checkStatus(s);

  s = mb_natural_incBy(alloc, &b, &out); checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

bool test_natural_incBy_2(void) {
  mb_Status s;
  mb_Natural out = mb_natural_new();
  mb_Natural b = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  u32 A_DIGS[2] = {999999999, 999999999};
  s = mb_natural_setVec(alloc, A_DIGS, 2, &out); checkStatus(s);

  u32 B_DIGS[2] = {999999999, 999999999};
  s = mb_natural_setVec(alloc, B_DIGS, 2, &b); checkStatus(s);

  u32 EXP_DIGS[3] = {1, 999999999, 999999998};
  s = mb_natural_setVec(alloc, EXP_DIGS, 3, &expected); checkStatus(s);

  s = mb_natural_incBy(alloc, &b, &out); checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

// tests 0 as identity (in out)
bool test_natural_incBy_3(void) {
  mb_Status s;
  mb_Natural out = mb_natural_new();
  mb_Natural b   = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 0, &out); checkStatus(s);
  s = mb_natural_set(alloc, 42, &b); checkStatus(s);
  s = mb_natural_set(alloc, 42, &expected); checkStatus(s);
  s = mb_natural_incBy(alloc, &b, &out); checkStatus(s);

  return mb_natural_equal(&out, &expected);
}
/* END: incBy tests*/

/* BEGIN: decrByDigit tests*/
// tests carry
bool test_natural_decrByDigit_1(void) {
  mb_Status s;
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  u32 OUT_DIGS[] = {1, 0, 0};
  s = mb_natural_setVec(alloc, OUT_DIGS, 3, &out); checkStatus(s);
  
  u32 EXP_DIGS[2] = {999999999, 999999999};
  s = mb_natural_setVec(alloc, EXP_DIGS, 2, &expected); checkStatus(s);

  s = mb_natural_decrByDigit(alloc, 1, &out); checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

bool test_natural_decrByDigit_2(void) {
  mb_Status s;
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 1, &out); checkStatus(s);
  s = mb_natural_set(alloc, 0, &expected); checkStatus(s);

  s = mb_natural_decrByDigit(alloc, 1, &out); checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

/* test 0 as identity */
bool test_natural_decrByDigit_3(void) {
  mb_Status s;
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 42, &out); checkStatus(s);
  s = mb_natural_set(alloc, 42, &expected); checkStatus(s);
  s = mb_natural_decrByDigit(alloc, 0, &out); checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

/* test 0 - 0 = 0 */
bool test_natural_decrByDigit_4(void) {
  mb_Status s;
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 0, &out); checkStatus(s);
  s = mb_natural_set(alloc, 0, &expected); checkStatus(s);
  s = mb_natural_decrByDigit(alloc, 0, &out); checkStatus(s);

  return mb_natural_equal(&out, &expected);
}
/* END: decrByDigit tests*/

/* BEGIN: decrBy tests*/
// tests carry
bool test_natural_decrBy_1(void) {
  mb_Status s;
  mb_Natural b = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  u32 OUT_DIGS[] = {1, 0, 0};
  s = mb_natural_setVec(alloc, OUT_DIGS, 3, &out); checkStatus(s);
  
  u32 EXP_DIGS[2] = {999999999, 999999999};
  s = mb_natural_setVec(alloc, EXP_DIGS, 2, &expected); checkStatus(s);

  s = mb_natural_set(alloc, 1, &b); checkStatus(s);

  s = mb_natural_decrBy(alloc, &b, &out); checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

bool test_natural_decrBy_2(void) {
  mb_Status s;
  mb_Natural b = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 1, &b); checkStatus(s);
  s = mb_natural_set(alloc, 1, &out); checkStatus(s);
  s = mb_natural_set(alloc, 0, &expected); checkStatus(s);

  s = mb_natural_decrBy(alloc, &b, &out); checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

/* test 0 as identity */
bool test_natural_decrBy_3(void) {
  mb_Status s;
  mb_Natural b = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 42, &out); checkStatus(s);
  s = mb_natural_set(alloc, 42, &expected); checkStatus(s);
  s = mb_natural_set(alloc, 0,  &b); checkStatus(s);

  s = mb_natural_decrBy(alloc, &b, &out); checkStatus(s);

  return mb_natural_equal(&out, &expected);
}

/* test 0 - 0 = 0 */
bool test_natural_decrBy_4(void) {
  mb_Status s;
  mb_Natural b = mb_natural_new();
  mb_Natural out = mb_natural_new();
  mb_Natural expected = mb_natural_new();

  s = mb_natural_set(alloc, 0, &out); checkStatus(s);
  s = mb_natural_set(alloc, 0, &b); checkStatus(s);
  s = mb_natural_set(alloc, 0, &expected); checkStatus(s);
  s = mb_natural_decrBy(alloc, &b, &out); checkStatus(s);

  return mb_natural_equal(&out, &expected);
}
/* END: decrBy tests*/

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
  {"test_natural_addDigit_5", test_natural_addDigit_5},

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

  {"test_natural_mult_1",  test_natural_mult_1},
  {"test_natural_mult_2",  test_natural_mult_2},
  {"test_natural_mult_3",  test_natural_mult_3},
  {"test_natural_mult_4",  test_natural_mult_4},
  {"test_natural_mult_5",  test_natural_mult_5},
  {"test_natural_mult_6",  test_natural_mult_6},
  {"test_natural_mult_7",  test_natural_mult_7},
  {"test_natural_mult_8",  test_natural_mult_8},
  {"test_natural_mult_9",  test_natural_mult_9},
  {"test_natural_mult_10", test_natural_mult_10},

  {"test_natural_multBase_1", test_natural_multBase_1},
  {"test_natural_multBase_2", test_natural_multBase_2},

  {"test_natural_divDigit_1", test_natural_divDigit_1},
  {"test_natural_divDigit_2", test_natural_divDigit_2},
  {"test_natural_divDigit_3", test_natural_divDigit_3},
  {"test_natural_divDigit_4", test_natural_divDigit_4},
  {"test_natural_divDigit_5", test_natural_divDigit_5},
  {"test_natural_divDigit_6", test_natural_divDigit_6},
  {"test_natural_divDigit_7", test_natural_divDigit_7},
  {"test_natural_divDigit_8", test_natural_divDigit_8},

  {"test_natural_div_1", test_natural_div_1},
  {"test_natural_div_2", test_natural_div_2},
  {"test_natural_div_3", test_natural_div_3},
  {"test_natural_div_4", test_natural_div_4},
  {"test_natural_div_5", test_natural_div_5},
  {"test_natural_div_6", test_natural_div_6},
  {"test_natural_div_7", test_natural_div_7},
  {"test_natural_div_8", test_natural_div_8},

  {"test_natural_copy_1", test_natural_copy_1},
  {"test_natural_copy_2", test_natural_copy_2},
  {"test_natural_copy_3", test_natural_copy_3},

  {"test_natural_add_0a", test_natural_add_0a},
  {"test_natural_add_0b", test_natural_add_0b},
  {"test_natural_add_1a", test_natural_add_1a},
  {"test_natural_add_1b", test_natural_add_1b},
  {"test_natural_add_2",  test_natural_add_2},

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

  {"test_natural_compareDigit_1", test_natural_compareDigit_1},
  {"test_natural_compareDigit_2", test_natural_compareDigit_2},
  {"test_natural_compareDigit_3", test_natural_compareDigit_3},
  {"test_natural_compareDigit_4", test_natural_compareDigit_4},
  {"test_natural_compareDigit_5", test_natural_compareDigit_5},
  {"test_natural_compareDigit_6", test_natural_compareDigit_6},
  {"test_natural_compareDigit_7", test_natural_compareDigit_7},

  {"test_natural_growShrink_1", test_natural_growShrink_1},
  {"test_natural_growShrink_2", test_natural_growShrink_2},
  {"test_natural_growShrink_3", test_natural_growShrink_3},
  {"test_natural_growShrink_4", test_natural_growShrink_4},
  {"test_natural_growShrink_5", test_natural_growShrink_5},
  {"test_natural_growShrink_6", test_natural_growShrink_6},

  {"test_natural_incByDigit_0", test_natural_incByDigit_0},
  {"test_natural_incByDigit_1", test_natural_incByDigit_1},
  {"test_natural_incByDigit_2", test_natural_incByDigit_2},
  {"test_natural_incByDigit_3", test_natural_incByDigit_3},

  {"test_natural_incBy_0", test_natural_incBy_0},
  {"test_natural_incBy_1", test_natural_incBy_1},
  {"test_natural_incBy_2", test_natural_incBy_2},
  {"test_natural_incBy_3", test_natural_incBy_3},

  {"test_natural_decrByDigit_1", test_natural_decrByDigit_1},
  {"test_natural_decrByDigit_2", test_natural_decrByDigit_2},
  {"test_natural_decrByDigit_3", test_natural_decrByDigit_3},
  {"test_natural_decrByDigit_4", test_natural_decrByDigit_4},

  {"test_natural_decrBy_1", test_natural_decrBy_1},
  {"test_natural_decrBy_2", test_natural_decrBy_2},
  {"test_natural_decrBy_3", test_natural_decrBy_3},
  {"test_natural_decrBy_4", test_natural_decrBy_4},
};
#define TEST_LEN (int)(sizeof(tests) / sizeof(tests[0]))

int main(void) {
  run_tests(tests, TEST_LEN);
}
/* END: DRIVER CODE */
