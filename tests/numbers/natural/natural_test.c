#include "../../../lib/numbers/natural.h"
#include "../../../lib/basicTypes.h"
#include "../../../lib/status.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "../../common.h"

char buffer[DEFAULT_SIZE];

bool isEmptyNat(Natural n) {
  return n.cap == 0 && n.len == 0 && n.digits == NULL;
}

/* backed by FLAlloc (see common.h) instead of stdAlloc, so that
 * info().used actually tracks live allocations: isAllFree() below
 * is a real leak check, not a constant true.
 */
i_FailAllocHeap _heap;
IAllocator _alloc;
#define alloc (&_alloc)

bool isAllFree(void) {
  return alloc->info(alloc->heap).used == 0;
}

bool isValidNatural(Natural* n) {
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
    if (n->digits[i] >= natural_BASE) {
      return false;
    }
    i++;
  }
  return true;
}

void printNat(Natural* n) {
  usize written = natural_snprint(n, buffer, DEFAULT_SIZE);
  if (written == 0) {
    printf("written 0 bytes.\n");
  }
  printf("%.*s", (int)written, buffer);
  printf(" (length: %d, cap: %d)\n", n->len, n->cap);
}

/* BEGIN: testing addDigit*/
bool test_natural_addDigit_0(void) {
  Status s;
  Natural a = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  u32 A_DIGS[] = {999999999, 999999999};
  s = natural_setVec(alloc, A_DIGS, 2, &a); checkStatus(s);
  u32 EXP_DIGS[] = {1, 0, 0};
  s = natural_setVec(alloc, EXP_DIGS, 3, &expected); checkStatus(s);

  s = natural_addDigit(alloc, &a, 1, &out); checkStatus(s);
  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);

  natural_free(alloc, a);
  natural_free(alloc, out);
  natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests carry
bool test_natural_addDigit_1(void) {
  Status s;
  Natural a = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  u32 A_DIGS[] = {999999999, 999999998};
  s = natural_setVec(alloc, A_DIGS, 2, &a); checkStatus(s);
  u32 EXP_DIGS[3] = {999999999, 999999999};
  s = natural_setVec(alloc, EXP_DIGS, 2, &expected); checkStatus(s);

  s = natural_addDigit(alloc, &a, 1, &out); checkStatus(s);
  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);

  natural_free(alloc, a);
  natural_free(alloc, out);
  natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests 0 as identity
bool test_natural_addDigit_2(void) {
  Status s;
  Natural a = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  s = natural_set(alloc, 0, &a); checkStatus(s);
  s = natural_set(alloc, 42, &expected); checkStatus(s);

  s = natural_addDigit(alloc, &a, 42, &out); checkStatus(s);
  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);

  natural_free(alloc, a);
  natural_free(alloc, out);
  natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests 0 as identity
bool test_natural_addDigit_3(void) {
  Status s;
  Natural a = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  s = natural_set(alloc, 314159, &a); checkStatus(s);
  s = natural_set(alloc, 314159, &expected); checkStatus(s);

  s = natural_addDigit(alloc, &a, 0, &out); checkStatus(s);
  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);

  natural_free(alloc, a);
  natural_free(alloc, out);
  natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests aliased parameters.
bool test_natural_addDigit_4a(void) {
  Status s;
  Natural out = natural_new();
  Natural expected = natural_new();

  u32 out_digs[] = {999999999, 999999999};
  s = natural_setVec(alloc, out_digs, 2, &out); checkStatus(s);
  u32 exp_digs[] = {1, 0, 0};
  s = natural_setVec(alloc, exp_digs, 3, &expected); checkStatus(s);

  s = natural_addDigit(alloc, &out, 1, &out); checkStatus(s);
  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);


  natural_free(alloc, out);
  natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests aliased parameters.
bool test_natural_addDigit_4b(void) {
  Status s;
  Natural out = natural_new();

  s = natural_set(alloc, 314159, &out); checkStatus(s);
  s = natural_addDigit(alloc, &out, 1, &out); checkStatus(s);
  bool ok = natural_equalDigit(&out, 314160) &&
            isValidNatural(&out);

  natural_free(alloc, out);

  return ok && isAllFree();
}

// sets garbage to the outparam.
bool test_natural_addDigit_5(void) {
  Status s;
  Natural a = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  s = natural_set(alloc, 314159, &a); checkStatus(s);
  s = natural_set(alloc, 999999, &out); checkStatus(s);
  s = natural_set(alloc, 314160, &expected); checkStatus(s);

  s = natural_addDigit(alloc, &a, 1, &out); checkStatus(s);
  bool ok = natural_equalDigit(&out, 314160) &&
            isValidNatural(&out);

  natural_free(alloc, a);
  natural_free(alloc, out);
  natural_free(alloc, expected);

  return ok && isAllFree();
}
/* END: testing addDigit */

/* BEGIN: testing compare */
bool test_natural_compare_1(void) {
  Status s;
  Natural a = natural_new();
  Natural b = natural_new();
  Order out;

  s = natural_set(alloc, 42, &a); checkStatus(s);
  s = natural_set(alloc, 42, &b); checkStatus(s);
  
  out = natural_compare(&a, &b);

  natural_free(alloc, a);
  natural_free(alloc, b);

  return out == order_EQUAL && isAllFree();
}

bool test_natural_compare_2(void) {
  Status s;
  Natural a = natural_new();
  Natural b = natural_new();
  Order out;

  s = natural_set(alloc, 1, &a); checkStatus(s);
  s = natural_set(alloc, 100, &b); checkStatus(s);
  
  out  = natural_compare(&a, &b);

  natural_free(alloc, a);
  natural_free(alloc, b);

  return out == order_LESS && isAllFree();
}

bool test_natural_compare_3(void) {
  Status s;
  Natural a = natural_new();
  Natural b = natural_new();
  Order out;

  s = natural_set(alloc, 100, &a); checkStatus(s);
  s = natural_set(alloc, 1, &b); checkStatus(s);
  
  out  = natural_compare(&a, &b);

  natural_free(alloc, a);
  natural_free(alloc, b);

  return out == order_GREATER && isAllFree();
}

bool test_natural_compare_4(void) {
  Status s;
  Natural a = natural_new();
  Natural b = natural_new();
  Order out;

  u32 A_DIGS[] = {1, 0};
  s = natural_setVec(alloc, A_DIGS, 2, &a); checkStatus(s);
  s = natural_set(alloc, natural_BASE - 1, &b); checkStatus(s);
  
  out = natural_compare(&a, &b);

  natural_free(alloc, a);
  natural_free(alloc, b);

  return out == order_GREATER && isAllFree();
}

bool test_natural_compare_5(void) {
  Status s;
  Natural a = natural_new();
  Natural b = natural_new();
  Order out;

  u32 A_DIGS[] = {127, 0, 0, 1};
  s = natural_setVec(alloc, A_DIGS, 4, &a); checkStatus(s);
  u32 B_DIGS[] = {127, 0, 0, 0};
  s = natural_setVec(alloc, B_DIGS, 4, &b); checkStatus(s);
  
  out = natural_compare(&a, &b);

  natural_free(alloc, a);
  natural_free(alloc, b);

  return out == order_GREATER && isAllFree();
}

bool test_natural_compare_6(void) {
  Status s;
  Natural a = natural_new();
  Natural b = natural_new();
  Order out;

  u32 A_DIGS[] = {127, 0, 0, 0};
  s = natural_setVec(alloc, A_DIGS, 4, &a); checkStatus(s);
  u32 B_DIGS[] = {127, 0, 0, 1};
  s = natural_setVec(alloc, B_DIGS, 4, &b); checkStatus(s);
  
  out = natural_compare(&a, &b);

  natural_free(alloc, a);
  natural_free(alloc, b);

  return out == order_LESS && isAllFree();
}

bool test_natural_compare_7(void) {
  Status s;
  Natural a = natural_new();
  Natural b = natural_new();
  Order out;

  u32 A_DIGS[] = {127, 128, 129, 130};
  s = natural_setVec(alloc, A_DIGS, 4, &a); checkStatus(s);
  u32 B_DIGS[] = {127, 128, 129, 130};
  s = natural_setVec(alloc, B_DIGS, 4, &b); checkStatus(s);
  
  out  = natural_compare(&a, &b);

  natural_free(alloc, a);
  natural_free(alloc, b);

  return out == order_EQUAL && isAllFree();
}
/* END: testing compare */

/* BEGIN: testing compareDigit */
bool test_natural_compareDigit_1(void) {
  Status s;
  Natural a = natural_new();
  Order out;

  s = natural_set(alloc, 42, &a); checkStatus(s);
  out = natural_compareDigit(&a, 42);
  natural_free(alloc, a);

  return out == order_EQUAL && isAllFree();
}

bool test_natural_compareDigit_2(void) {
  Status s;
  Natural a = natural_new();
  Order out;

  u32 A_DIGS[] = {127, 0, 0, 0};
  s = natural_setVec(alloc, A_DIGS, 4, &a); checkStatus(s);
  out = natural_compareDigit(&a, 1);
  natural_free(alloc, a);

  return out == order_GREATER && isAllFree();
}

bool test_natural_compareDigit_3(void) {
  Status s;
  Natural a = natural_new();
  Order out;

  u32 A_DIGS[] = {1, 0};
  s = natural_setVec(alloc, A_DIGS, 2, &a); checkStatus(s);
  out = natural_compareDigit(&a, natural_BASE-1);
  natural_free(alloc, a);

  return out == order_GREATER && isAllFree();
}

bool test_natural_compareDigit_4(void) {
  Status s;
  Natural a = natural_new();
  Order out;

  s = natural_set(alloc, 0, &a); checkStatus(s);
  out = natural_compareDigit(&a, 0);
  natural_free(alloc, a);

  return out == order_EQUAL && isAllFree();
}

bool test_natural_compareDigit_5(void) {
  Status s;
  Natural a = natural_new();
  Order out;

  s = natural_set(alloc, 0, &a); checkStatus(s);
  out = natural_compareDigit(&a, 1);
  natural_free(alloc, a);

  return out == order_LESS && isAllFree();
}

bool test_natural_compareDigit_6(void) {
  Status s;
  Natural a = natural_new();
  Order out;

  s = natural_set(alloc, 5, &a); checkStatus(s);
  out = natural_compareDigit(&a, 6);
  natural_free(alloc, a);

  return out == order_LESS && isAllFree();
}

bool test_natural_compareDigit_7(void) {
  Status s;
  Natural a = natural_new();
  Order out;

  s = natural_set(alloc, 6, &a); checkStatus(s);
  out = natural_compareDigit(&a, 5);
  natural_free(alloc, a);

  return out == order_GREATER && isAllFree();
}
/* END: testing compareDigit */

/* BEGIN: testing add */

// tests 0 as identity
bool test_natural_add_0a(void) {
  Status s;
  Natural a = natural_new();
  Natural b = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  s = natural_set(alloc, 42, &a); checkStatus(s);
  s = natural_set(alloc, 0, &b); checkStatus(s);
  s = natural_set(alloc, 42, &expected); checkStatus(s);
  
  s = natural_add(alloc, &a, &b, &out); checkStatus(s);

  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);

  natural_free(alloc, a);
  natural_free(alloc, b);
  natural_free(alloc, out);
  natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests 0 as identity, but places garbage in the out parameter
bool test_natural_add_0b(void) {
  Status s;
  Natural a = natural_new();
  Natural b = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  const u32 GARBAGE = 0xCAFE;
  s = natural_set(alloc, GARBAGE, &out); checkStatus(s);
  s = natural_set(alloc, 42, &a); checkStatus(s);
  s = natural_set(alloc, 0, &b); checkStatus(s);
  s = natural_set(alloc, 42, &expected); checkStatus(s);
  
  s = natural_add(alloc, &a, &b, &out); checkStatus(s);

  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);

  natural_free(alloc, a);
  natural_free(alloc, b);
  natural_free(alloc, out);
  natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests addition of big numbers 
bool test_natural_add_1a(void) {
  Status s;
  Natural a = natural_new();
  Natural b = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  s = natural_set(alloc, natural_BASE - 1, &a); checkStatus(s);
  s = natural_set(alloc, natural_BASE - 1, &b); checkStatus(s);
  
  u32 EXP_DIGS[2] = {1, 999999998};
  s = natural_setVec(alloc, EXP_DIGS, 2, &expected); checkStatus(s);

  s = natural_add(alloc, &a, &b, &out); checkStatus(s);
  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);
  natural_free(alloc, a);
  natural_free(alloc, b);
  natural_free(alloc, out);
  natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests addition of big numbers, but places garbage at the outparam
bool test_natural_add_1b(void) {
  Status s;
  Natural a = natural_new();
  Natural b = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  const u32 GARBAGE = 0xCAFE;
  s = natural_set(alloc, GARBAGE, &out); checkStatus(s);
  s = natural_set(alloc, natural_BASE - 1, &a); checkStatus(s);
  s = natural_set(alloc, natural_BASE - 1, &b); checkStatus(s);
  u32 EXP_DIGS[2] = {1, 999999998};
  s = natural_setVec(alloc, EXP_DIGS, 2, &expected); checkStatus(s);

  s = natural_add(alloc, &a, &b, &out); checkStatus(s);
  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);
  natural_free(alloc, a);
  natural_free(alloc, b);
  natural_free(alloc, out);
  natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests commutativity
bool test_natural_add_2(void) {
  Status s;
  Natural a = natural_new();
  Natural b = natural_new();
  Natural out1 = natural_new();
  Natural out2 = natural_new();
  Natural expected = natural_new();

  s = natural_set(alloc, 42, &a); checkStatus(s);
  s = natural_set(alloc, 17, &b); checkStatus(s);
  s = natural_set(alloc, 59, &expected); checkStatus(s);

  s = natural_add(alloc, &a, &b, &out1); checkStatus(s);
  s = natural_add(alloc, &b, &a, &out2); checkStatus(s);

  bool ok = natural_equal(&out1, &expected) &&
            natural_equal(&out2, &expected) &&
            isValidNatural(&out1) &&
            isValidNatural(&out2);

  natural_free(alloc, a);
  natural_free(alloc, b);
  natural_free(alloc, out1);
  natural_free(alloc, out2);
  natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests 2 aliased parameters
bool test_natural_add_3a(void) {
  Status s;
  Natural b = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  s = natural_set(alloc, natural_BASE - 1, &out); checkStatus(s);
  s = natural_set(alloc, natural_BASE - 1, &b); checkStatus(s);
  
  u32 EXP_DIGS[2] = {1, 999999998};
  s = natural_setVec(alloc, EXP_DIGS, 2, &expected); checkStatus(s);

  s = natural_add(alloc, &out, &b, &out); checkStatus(s);
  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);
  natural_free(alloc, b);
  natural_free(alloc, out);
  natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests 3 aliased parameters
bool test_natural_add_3b(void) {
  Status s;
  Natural out = natural_new();
  Natural expected = natural_new();

  s = natural_set(alloc, natural_BASE - 1, &out); checkStatus(s);
  
  u32 EXP_DIGS[2] = {1, 999999998};
  s = natural_setVec(alloc, EXP_DIGS, 2, &expected); checkStatus(s);

  s = natural_add(alloc, &out, &out, &out); checkStatus(s);
  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);
  natural_free(alloc, out);
  natural_free(alloc, expected);

  return ok && isAllFree();
}
/* END: testing add */

/* BEGIN: testing multDigit */

// tests 0 as annihilator
bool test_natural_multDigit_1(void) {
  Status s;
  Natural a = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  s = natural_set(alloc, 314159, &a); checkStatus(s);
  s = natural_set(alloc, 0, &expected); checkStatus(s);
  s = natural_multDigit(alloc, &a, 0, &out); checkStatus(s);

  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);

  natural_free(alloc, a);
  natural_free(alloc, out);
  natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests 0 as annihilator
bool test_natural_multDigit_2(void) {
  Status s;
  Natural a = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  s = natural_set(alloc, 0, &a); checkStatus(s);
  s = natural_set(alloc, 0, &expected); checkStatus(s);

  s = natural_multDigit(alloc, &a, 314159, &out); checkStatus(s);

  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);
  natural_free(alloc, a);
  natural_free(alloc, out);
  natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests 1 as identity
bool test_natural_multDigit_3(void) {
  Status s;
  Natural a = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  s = natural_set(alloc, 314159, &a); checkStatus(s);
  s = natural_set(alloc, 314159, &expected); checkStatus(s);

  s = natural_multDigit(alloc, &a, 1, &out); checkStatus(s);

  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);
  natural_free(alloc, a);
  natural_free(alloc, out);
  natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests 1 as identity
bool test_natural_multDigit_4(void) {
  Status s;
  Natural a = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  s = natural_set(alloc, 1, &a); checkStatus(s);
  s = natural_set(alloc, 314159, &expected); checkStatus(s);

  s = natural_multDigit(alloc, &a, 314159, &out); checkStatus(s);

  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);
  natural_free(alloc, a);
  natural_free(alloc, out);
  natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests powers of 2
bool test_natural_multDigit_5(void) {
  Status s;
  Natural a = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  {
    s = natural_set(alloc, 1, &a); checkStatus(s);
    s = natural_set(alloc, 2, &expected); checkStatus(s);
    s = natural_multDigit(alloc, &a, 2, &out); checkStatus(s);

    if (!natural_equal(&out, &expected) || !isValidNatural(&out)) {
      natural_free(alloc, a);
      natural_free(alloc, out);
      natural_free(alloc, expected);
      return false && isAllFree();
    }
  }

  {
    s = natural_set(alloc, 2, &a); checkStatus(s);
    s = natural_set(alloc, 4, &expected); checkStatus(s);
    s = natural_multDigit(alloc, &a, 2, &out); checkStatus(s);

    if (!natural_equal(&out, &expected) || !isValidNatural(&out)) {
      natural_free(alloc, a);
      natural_free(alloc, out);
      natural_free(alloc, expected);
      return false && isAllFree();
    }
  }

  {
    s = natural_set(alloc, 4, &a); checkStatus(s);
    s = natural_set(alloc, 8, &expected); checkStatus(s);
    s = natural_multDigit(alloc, &a, 2, &out); checkStatus(s);

    if (!natural_equal(&out, &expected) || !isValidNatural(&out)) {
      natural_free(alloc, a);
      natural_free(alloc, out);
      natural_free(alloc, expected);
      return false && isAllFree();
    }
  }

  natural_free(alloc, a);
  natural_free(alloc, out);
  natural_free(alloc, expected);
  return true && isAllFree();
}

// tests some other multiplications
bool test_natural_multDigit_6(void) {
  Status s;
  Natural a = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  {
    s = natural_set(alloc, 12, &a); checkStatus(s);
    s = natural_set(alloc, 144, &expected); checkStatus(s);
    s = natural_multDigit(alloc, &a, 12, &out); checkStatus(s);

    if (!natural_equal(&out, &expected) || !isValidNatural(&out)) {
      natural_free(alloc, a);
      natural_free(alloc, out);
      natural_free(alloc, expected);
      return false;
    }
  }

  {
    s = natural_set(alloc, 1111, &a); checkStatus(s);
    s = natural_set(alloc, 3702963, &expected); checkStatus(s);
    s = natural_multDigit(alloc, &a, 3333, &out); checkStatus(s);

    if (!natural_equal(&out, &expected) || !isValidNatural(&out)) {
      natural_free(alloc, a);
      natural_free(alloc, out);
      natural_free(alloc, expected);
      return false;
    }
  }

  {
    u32 digits[] = {
      999999998, 000000001
    };
    s = natural_set(alloc, natural_BASE-1, &a); checkStatus(s);
    s = natural_setVec(alloc, digits, 2, &expected); checkStatus(s);
    s = natural_multDigit(alloc, &a, natural_BASE-1, &out); checkStatus(s);

    if (!natural_equal(&out, &expected) || !isValidNatural(&out)) {
      natural_free(alloc, a);
      natural_free(alloc, out);
      natural_free(alloc, expected);
      return false;
    }
  }
  natural_free(alloc, a);
  natural_free(alloc, out);
  natural_free(alloc, expected);
  return true;
}

// tests whether operands remain unchanged
bool test_natural_multDigit_7(void) {
  Status s;
  Natural a = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  s = natural_set(alloc, 4, &a); checkStatus(s);
  s = natural_set(alloc, 16, &expected); checkStatus(s);
  s = natural_multDigit(alloc, &a, 4, &out); checkStatus(s);

  bool ok = natural_equal(&out, &expected) &&
            natural_equalDigit(&a, 4) &&
            isValidNatural(&out);

  natural_free(alloc, a);
  natural_free(alloc, out);
  natural_free(alloc, expected);
  return ok && isAllFree();
}

/* END: testing multDigit */

/* BEGIN: mult tests */

// tests 0 as annihilator (A = 0)
bool test_natural_mult_1(void) {
  Status s;
  Natural a   = natural_new();
  Natural b   = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  s = natural_set(alloc, 0,      &a);        checkStatus(s);
  s = natural_set(alloc, 314159, &b);        checkStatus(s);
  s = natural_set(alloc, 0,      &expected); checkStatus(s);

  s = natural_mult(alloc, &a, &b, &out); checkStatus(s);

  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);

  natural_free(alloc, a);
  natural_free(alloc, b);
  natural_free(alloc, out);
  natural_free(alloc, expected);
  return ok && isAllFree();
}

// tests 0 as annihilator (B = 0)
bool test_natural_mult_2(void) {
  Status s;
  Natural a   = natural_new();
  Natural b   = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  s = natural_set(alloc, 314159, &a);        checkStatus(s);
  s = natural_set(alloc, 0,      &b);        checkStatus(s);
  s = natural_set(alloc, 0,      &expected); checkStatus(s);

  s = natural_mult(alloc, &a, &b, &out); checkStatus(s);

  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);
  
  natural_free(alloc, a);
  natural_free(alloc, b);
  natural_free(alloc, out);
  natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests 1 as identity (A = 1)
bool test_natural_mult_3(void) {
  Status s;
  Natural a   = natural_new();
  Natural b   = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  s = natural_set(alloc, 1,      &a);        checkStatus(s);
  s = natural_set(alloc, 314159, &b);        checkStatus(s);
  s = natural_set(alloc, 314159, &expected); checkStatus(s);

  s = natural_mult(alloc, &a, &b, &out); checkStatus(s);

  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);
  natural_free(alloc, a);
  natural_free(alloc, b);
  natural_free(alloc, out);
  natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests 1 as identity (B = 1)
bool test_natural_mult_4(void) {
  Status s;
  Natural a   = natural_new();
  Natural b   = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  s = natural_set(alloc, 314159, &a);        checkStatus(s);
  s = natural_set(alloc, 1,      &b);        checkStatus(s);
  s = natural_set(alloc, 314159, &expected); checkStatus(s);

  s = natural_mult(alloc, &a, &b, &out); checkStatus(s);

  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);
  natural_free(alloc, a);
  natural_free(alloc, b);
  natural_free(alloc, out);
  natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests commutativity
bool test_natural_mult_5(void) {
  Status s;
  Natural a    = natural_new();
  Natural b    = natural_new();
  Natural out1 = natural_new();
  Natural out2 = natural_new();

  s = natural_set(alloc, 12345, &a); checkStatus(s);
  s = natural_set(alloc, 67890, &b); checkStatus(s);

  s = natural_mult(alloc, &a, &b, &out1); checkStatus(s);
  s = natural_mult(alloc, &b, &a, &out2); checkStatus(s);

  bool ok = natural_equal(&out1, &out2) &&
            isValidNatural(&out1) &&
            isValidNatural(&out2);
  natural_free(alloc, a);
  natural_free(alloc, b);
  natural_free(alloc, out1);
  natural_free(alloc, out2);

  return ok && isAllFree();
}

// tests a carry that produces a new limb: (BASE-1) * (BASE-1)
bool test_natural_mult_6(void) {
  Status s;
  Natural a   = natural_new();
  Natural b   = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  s = natural_set(alloc, natural_BASE - 1, &a); checkStatus(s);
  s = natural_set(alloc, natural_BASE - 1, &b); checkStatus(s);

  u32 EXP_DIGS[] = {999999998, 000000001};
  s = natural_setVec(alloc, EXP_DIGS, 2, &expected); checkStatus(s);

  s = natural_mult(alloc, &a, &b, &out); checkStatus(s);

  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);

  natural_free(alloc, a);
  natural_free(alloc, b);
  natural_free(alloc, out);
  natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests multi-limb * single-limb: [1, 0] * 2 = [2, 0], ie, BASE * 2 = 2*BASE
bool test_natural_mult_7(void) {
  Status s;
  Natural a   = natural_new();
  Natural b   = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  u32 A_DIGS[] = {1, 0};
  s = natural_setVec(alloc, A_DIGS, 2, &a); checkStatus(s);
  s = natural_set(alloc, 2, &b);            checkStatus(s);

  u32 EXP_DIGS[] = {2, 0};
  s = natural_setVec(alloc, EXP_DIGS, 2, &expected); checkStatus(s);

  s = natural_mult(alloc, &a, &b, &out); checkStatus(s);

  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);
  natural_free(alloc, a);
  natural_free(alloc, b);
  natural_free(alloc, out);
  natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests multi-limb * multi-limb with carry propagation across limbs
// [1, 0] * [1, 0] = [1, 0, 0], ie BASE * BASE = BASE^2
bool test_natural_mult_8(void) {
  Status s;
  Natural a   = natural_new();
  Natural b   = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  u32 AB_DIGS[] = {1, 0};
  s = natural_setVec(alloc, AB_DIGS, 2, &a); checkStatus(s);
  s = natural_setVec(alloc, AB_DIGS, 2, &b); checkStatus(s);

  u32 EXP_DIGS[] = {1, 0, 0};
  s = natural_setVec(alloc, EXP_DIGS, 3, &expected); checkStatus(s);

  s = natural_mult(alloc, &a, &b, &out); checkStatus(s);

  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);
  natural_free(alloc, a);
  natural_free(alloc, b);
  natural_free(alloc, out);
  natural_free(alloc, expected);

  return ok && isAllFree();
}

// tests that operands are not modified
bool test_natural_mult_9(void) {
  Status s;
  Natural a   = natural_new();
  Natural b   = natural_new();
  Natural out = natural_new();

  s = natural_set(alloc, 6, &a); checkStatus(s);
  s = natural_set(alloc, 7, &b); checkStatus(s);

  s = natural_mult(alloc, &a, &b, &out); checkStatus(s);

  bool ok = natural_equalDigit(&a, 6) &&
            natural_equalDigit(&b, 7) &&
            isValidNatural(&a) && isValidNatural(&b);
  natural_free(alloc, a);
  natural_free(alloc, b);
  natural_free(alloc, out);
  return ok && isAllFree();
}

// tests that garbage in the out-param does not affect the result
bool test_natural_mult_10(void) {
  Status s;
  Natural a   = natural_new();
  Natural b   = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  const u32 GARBAGE = 0xCAFEBABE % natural_BASE;
  s = natural_set(alloc, GARBAGE, &out);    checkStatus(s);
  s = natural_set(alloc, 111,     &a);      checkStatus(s);
  s = natural_set(alloc, 111,     &b);      checkStatus(s);
  s = natural_set(alloc, 12321,   &expected); checkStatus(s);

  s = natural_mult(alloc, &a, &b, &out); checkStatus(s);

  bool ok = natural_equal(&out, &expected) &&
          isValidNatural(&out);
  natural_free(alloc, a);
  natural_free(alloc, b);
  natural_free(alloc, out);
  natural_free(alloc, expected);
  return ok && isAllFree();
}
/* END: mult tests */

/* BEGIN: testing multBase*/
bool test_natural_multBase_1(void) {
  Status s;
  Natural a = natural_new();
  Natural expected = natural_new();

  u32 digits[] = {
    000000001, 000000000
  };
  s = natural_set(alloc, 1, &a); checkStatus(s);
  s = natural_setVec(alloc, digits, 2, &expected); checkStatus(s);
  s = natural_multBase(alloc, &a); checkStatus(s);

  bool ok = natural_equal(&a, &expected) &&
            isValidNatural(&a);
  natural_free(alloc, a);
  natural_free(alloc, expected);
  return ok && isAllFree();
}

bool test_natural_multBase_2(void) {
  Status s;
  Natural a = natural_new();
  Natural expected = natural_new();

  u32 digits[] = {
    900000000, 000000000, 000000000
  };
  s = natural_set(alloc, 900000000, &a); checkStatus(s);
  s = natural_setVec(alloc, digits, 3, &expected); checkStatus(s);
  s = natural_multBase(alloc, &a); checkStatus(s);
  s = natural_multBase(alloc, &a); checkStatus(s);

  bool ok = natural_equal(&a, &expected) &&
            isValidNatural(&a);
  natural_free(alloc, a);
  natural_free(alloc, expected);
  return ok && isAllFree();
}

/* END: testing multBase*/

/* BEGIN: testing snprint */
char test_buffer[DEFAULT_SIZE];

bool test_natural_snprint_0(void) {
  Status s;
  Natural A = natural_new();
  s = natural_set(alloc, 1, &A); checkStatus(s);

  usize written = natural_snprint(&A, test_buffer, DEFAULT_SIZE);
  if (written == 0) {
    natural_free(alloc, A);
    return false;
  }

  if (strncmp("1", test_buffer, written) != 0) {
    natural_free(alloc, A);
    return false;
  }
  natural_free(alloc, A);
  return true;
}

bool test_natural_snprint_1(void) {
  Status s;
  Natural A = natural_new();

  u32 digits[] = {
    1,0,0
  };

  s = natural_setVec(alloc, digits, 3, &A); checkStatus(s);

  usize written = natural_snprint(&A, test_buffer, DEFAULT_SIZE);
  if (written == 0) {
    natural_free(alloc, A);
    return false;
  }

  if (strncmp("1000000000000000000", test_buffer, written) != 0) {
    natural_free(alloc, A);
    return false;
  }
  natural_free(alloc, A);
  return true;
}

bool test_natural_snprint_2(void) {
  Status s;
  Natural A = natural_new();

  s = natural_set(alloc, 314159, &A); checkStatus(s);

  usize written = natural_snprint(&A, test_buffer, DEFAULT_SIZE);
  if (written == 0) {
    natural_free(alloc, A);
    return false;
  }

  if (strncmp("314159", test_buffer, written) != 0) {
    natural_free(alloc, A);
    return false;
  }
  natural_free(alloc, A);
  return true;
}

bool test_natural_snprint_3(void) {
  Status s;
  Natural A = natural_new();

  s = natural_set(alloc, 0, &A); checkStatus(s);

  usize written = natural_snprint(&A, test_buffer, DEFAULT_SIZE);
  if (written == 0) {
    natural_free(alloc, A);
    return false;
  }

  if (strncmp("0", test_buffer, written) != 0) {
    natural_free(alloc, A);
    return false;
  }
  natural_free(alloc, A);
  return true;
}

// buffSize == 0 must always fail, and must never touch the buffer.
bool test_natural_snprint_tooSmall_0(void) {
  Status s;
  Natural A = natural_new();
  s = natural_set(alloc, 42, &A); checkStatus(s);

  test_buffer[0] = 'X';
  usize written = natural_snprint(&A, test_buffer, 0);
  bool ok = written == 0 && test_buffer[0] == 'X';

  natural_free(alloc, A);
  return ok;
}

// a buffer far too small for a multi-limb number must fail without
// writing anything into the buffer (per the "fully writes or returns
// 0" contract).
bool test_natural_snprint_tooSmall_1(void) {
  Status s;
  Natural A = natural_new();
  u32 digits[] = {1, 2, 3}; // 3 limbs, neededBytes = 27
  s = natural_setVec(alloc, digits, 3, &A); checkStatus(s);

  test_buffer[0] = 'X';
  usize written = natural_snprint(&A, test_buffer, 5);
  bool ok = written == 0 && test_buffer[0] == 'X';

  natural_free(alloc, A);
  return ok;
}

// buffSize == 1: only zero fits in a single byte.
bool test_natural_snprint_bufferSize1_0(void) {
  Status s;
  Natural A = natural_new();
  s = natural_set(alloc, 0, &A); checkStatus(s);

  usize written = natural_snprint(&A, test_buffer, 1);
  bool ok = written == 1 && test_buffer[0] == '0';

  natural_free(alloc, A);
  return ok;
}

/* END: testing snprint */

/* BEGIN: testing printingSize */
// zero needs exactly one byte ("0")
bool test_natural_printingSize_zero(void) {
  Status s;
  Natural A = natural_new();
  s = natural_set(alloc, 0, &A); checkStatus(s);

  bool ok = natural_printingSize(&A) == 1;

  natural_free(alloc, A);
  return ok && isAllFree();
}

// single-limb number: printingSize must match the digit count exactly
bool test_natural_printingSize_singleLimb(void) {
  Status s;
  Natural A = natural_new();
  s = natural_set(alloc, 314159, &A); checkStatus(s);

  bool ok = natural_printingSize(&A) == 6; /* strlen("314159") */

  natural_free(alloc, A);
  return ok && isAllFree();
}

// multi-limb number: only the most-significant limb may be printed
// without leading zeroes, every other limb contributes exactly
// natural_DIGITSPERINT bytes.
bool test_natural_printingSize_multiLimb(void) {
  Status s;
  Natural A = natural_new();
  u32 digits[] = {1, 0, 0}; /* MSD -> LSD: 1 followed by two all-zero limbs */
  s = natural_setVec(alloc, digits, 3, &A); checkStatus(s);

  /* "1" followed by 2*9 zeroes, ie strlen("1000000000000000000") */
  bool ok = natural_printingSize(&A) == 1 + 2*natural_DIGITSPERINT;

  natural_free(alloc, A);
  return ok && isAllFree();
}

// printingSize must always agree with how many bytes snprint actually
// writes, given a buffer large enough to never truncate.
bool test_natural_printingSize_matchesSnprint(void) {
  Status s;
  Natural A = natural_new();
  u32 digits[] = {42, 7, 123456789};
  s = natural_setVec(alloc, digits, 3, &A); checkStatus(s);

  usize written = natural_snprint(&A, test_buffer, DEFAULT_SIZE);
  bool ok = written > 0 && natural_printingSize(&A) == written;

  natural_free(alloc, A);
  return ok && isAllFree();
}
/* END: testing printingSize */

/* BEGIN: testing distanceDigit */
// tests carry
bool test_natural_distanceDigit_1(void) {
  Status s;
  Natural a = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  u32 A_DIGS[] = {1, 0, 0};
  s = natural_setVec(alloc, A_DIGS, 3, &a); checkStatus(s);
  u32 EXP_DIGS[2] = {999999999, 999999999};
  s = natural_setVec(alloc, EXP_DIGS, 2, &expected); checkStatus(s);
  s = natural_distanceDigit(alloc, &a, 1, &out); checkStatus(s);

  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);

  natural_free(alloc, a);
  natural_free(alloc, out);
  natural_free(alloc, expected);
  return ok && isAllFree();
}

bool test_natural_distanceDigit_2(void) {
  Status s;
  Natural a = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  s = natural_set(alloc, 1, &a); checkStatus(s);
  s = natural_set(alloc, 0, &expected); checkStatus(s);
  s = natural_distanceDigit(alloc, &a, 1, &out); checkStatus(s);

  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);
  natural_free(alloc, a);
  natural_free(alloc, out);
  natural_free(alloc, expected);
  return ok && isAllFree();
}

/* test 0 as identity */
bool test_natural_distanceDigit_3(void) {
  Status s;
  Natural a = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  s = natural_set(alloc, 42, &a); checkStatus(s);
  s = natural_set(alloc, 42, &expected); checkStatus(s);
  s = natural_distanceDigit(alloc, &a, 0, &out); checkStatus(s);

  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);
  natural_free(alloc, a);
  natural_free(alloc, out);
  natural_free(alloc, expected);
  return ok && isAllFree();
}

/* tests 0 as identity, but now we also test if it is comutative */
bool test_natural_distanceDigit_4(void) {
  Status s;
  Natural a = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  s = natural_set(alloc, 0, &a); checkStatus(s);
  s = natural_set(alloc, 42, &expected); checkStatus(s);

  s = natural_distanceDigit(alloc, &a, 42, &out); checkStatus(s);

  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);
  natural_free(alloc, a);
  natural_free(alloc, out);
  natural_free(alloc, expected);
  return ok && isAllFree();
}

// tests aliased parameters
bool test_natural_distanceDigit_5a(void) {
  Status s;
  Natural out = natural_new();

  s = natural_set(alloc, 314159, &out); checkStatus(s);
  s = natural_distanceDigit(alloc, &out, 1, &out); checkStatus(s);

  bool ok = natural_equalDigit(&out, 314158) &&
            isValidNatural(&out);
  natural_free(alloc, out);
  return ok && isAllFree();
}

bool test_natural_distanceDigit_5b(void) {
  Status s;
  Natural out = natural_new();
  Natural expected = natural_new();

  u32 out_digs[] = {1, 0, 0};
  s = natural_setVec(alloc, out_digs, 3, &out); checkStatus(s);
  u32 exp_digs[] = {999999999, 999999999};
  s = natural_setVec(alloc, exp_digs, 2, &expected); checkStatus(s);

  s = natural_distanceDigit(alloc, &out, 1, &out); checkStatus(s);

  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);
  natural_free(alloc, out);
  natural_free(alloc, expected);
  return ok && isAllFree();
}
/* END: testing distanceDigit */

/* BEGIN: testing distance */

// tests 0 as identity
bool test_natural_distance_0(void) {
  Status s;
  Natural a = natural_new();
  Natural b = natural_new();
  Natural out1 = natural_new();
  Natural out2 = natural_new();
  Natural expected = natural_new();

  u32 A_DIGS[] = {1, 0, 0};
  s = natural_setVec(alloc, A_DIGS, 3, &a); checkStatus(s);

  s = natural_set(alloc, 0, &b); checkStatus(s);
  
  u32 EXP_DIGS[3] = {1, 0, 0};
  s = natural_setVec(alloc, EXP_DIGS, 3, &expected); checkStatus(s);

  s = natural_distance(alloc, &a, &b, &out1); checkStatus(s);
  s = natural_distance(alloc, &b, &a, &out2); checkStatus(s);

  bool ok = natural_equal(&out1, &expected) &&
            natural_equal(&out2, &expected) &&
            isValidNatural(&out1) &&
            isValidNatural(&out2);
  natural_free(alloc, a);
  natural_free(alloc, b);
  natural_free(alloc, out1);
  natural_free(alloc, out2);
  natural_free(alloc, expected);
  return ok && isAllFree();
}

// tests if |a - a| = 0
bool test_natural_distance_1(void) {
  Status s;
  Natural a = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  u32 A_DIGS[] = {1, 0, 0};
  s = natural_setVec(alloc, A_DIGS, 3, &a); checkStatus(s);
  s = natural_set(alloc, 0, &expected); checkStatus(s);
  s = natural_distance(alloc, &a, &a, &out); checkStatus(s);

  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);
  natural_free(alloc, a);
  natural_free(alloc, out);
  natural_free(alloc, expected);
  return ok && isAllFree();
}

// tests commutativity
bool test_natural_distance_2(void) {
  Status s;
  Natural a = natural_new();
  Natural b = natural_new();
  Natural expected = natural_new();
  Natural out1 = natural_new();
  Natural out2 = natural_new();

  s = natural_set(alloc, 42, &a); checkStatus(s);
  s = natural_set(alloc, 17, &b); checkStatus(s);
  s = natural_set(alloc, 25, &expected); checkStatus(s);

  s = natural_distance(alloc, &a, &b, &out1); checkStatus(s);
  s = natural_distance(alloc, &b, &a, &out2); checkStatus(s);

  bool ok = natural_equal(&out1, &expected) &&
            natural_equal(&out2, &expected) &&
            isValidNatural(&out1) &&
            isValidNatural(&out2);

  natural_free(alloc, a);
  natural_free(alloc, b);
  natural_free(alloc, out1);
  natural_free(alloc, out2);
  natural_free(alloc, expected);
  return ok && isAllFree();
}

// tests big numbers
bool test_natural_distance_3(void) {
  Status s;
  Natural a = natural_new();
  Natural b = natural_new();
  Natural out1 = natural_new();
  Natural out2 = natural_new();
  Natural expected = natural_new();

  u32 A_DIGS[3] = {1, 0, 42};
  s = natural_setVec(alloc, A_DIGS, 3, &a); checkStatus(s);
  s = natural_set(alloc, 42, &b); checkStatus(s);
  
  u32 EXP_DIGS[3] = {1, 0, 0};
  s = natural_setVec(alloc, EXP_DIGS, 3, &expected); checkStatus(s);

  s = natural_distance(alloc, &a, &b, &out1); checkStatus(s);
  s = natural_distance(alloc, &b, &a, &out2); checkStatus(s);

  bool ok = natural_equal(&out1, &expected) &&
            natural_equal(&out2, &expected) &&
            isValidNatural(&out1) &&
            isValidNatural(&out2);
  natural_free(alloc, a);
  natural_free(alloc, b);
  natural_free(alloc, out1);
  natural_free(alloc, out2);
  natural_free(alloc, expected);
  return ok && isAllFree();
}

// tests 2 aliased parameters
bool test_natural_distance_4a(void) {
  Status s;
  Natural b = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  u32 A_DIGS[3] = {1, 0, 42};
  s = natural_setVec(alloc, A_DIGS, 3, &out); checkStatus(s);
  s = natural_set(alloc, 42, &b); checkStatus(s);
  
  u32 EXP_DIGS[3] = {1, 0, 0};
  s = natural_setVec(alloc, EXP_DIGS, 3, &expected); checkStatus(s);

  s = natural_distance(alloc, &out, &b, &out); checkStatus(s);

  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);
  natural_free(alloc, b);
  natural_free(alloc, out);
  natural_free(alloc, expected);
  return ok && isAllFree();
}
// tests 2 aliased parameters
bool test_natural_distance_4b(void) {
  Status s;
  Natural b = natural_new();
  Natural out = natural_new();
  Natural expected = natural_new();

  u32 A_DIGS[3] = {1, 0, 42};
  s = natural_setVec(alloc, A_DIGS, 3, &out); checkStatus(s);
  s = natural_set(alloc, 42, &b); checkStatus(s);
  
  u32 EXP_DIGS[3] = {1, 0, 0};
  s = natural_setVec(alloc, EXP_DIGS, 3, &expected); checkStatus(s);

  s = natural_distance(alloc, &b, &out, &out); checkStatus(s);

  bool ok = natural_equal(&out, &expected) &&
            isValidNatural(&out);
  natural_free(alloc, b);
  natural_free(alloc, out);
  natural_free(alloc, expected);
  return ok && isAllFree();
}
// tests 3 aliased parameters
bool test_natural_distance_4c(void) {
  Status s;
  Natural out = natural_new();

  u32 A_DIGS[3] = {1, 0, 42};
  s = natural_setVec(alloc, A_DIGS, 3, &out); checkStatus(s);
  s = natural_distance(alloc, &out, &out, &out); checkStatus(s);

  bool ok = natural_isZero(&out) &&
            isValidNatural(&out);
  natural_free(alloc, out);
  return ok && isAllFree();
}
/* END: testing distance */

/* BEGIN: testing divDigit */
bool test_natural_divDigit_1(void) {
  Status s;
  Natural A = natural_new();
  u32 B = 3;
  Natural Q = natural_new();
  u32 R;

  Natural exp_Q = natural_new();
  u32 exp_R = 2;

  s = natural_set(alloc, 5, &A); checkStatus(s);
  s = natural_set(alloc, 1, &exp_Q); checkStatus(s);
  s = natural_divDigit(alloc, &A, B, &Q, &R); checkStatus(s);

  bool ok = R == exp_R && natural_equal(&Q, &exp_Q) &&
            isValidNatural(&Q);
  natural_free(alloc, A);
  natural_free(alloc, Q);
  natural_free(alloc, exp_Q);
  return ok && isAllFree();
}

bool test_natural_divDigit_2(void) {
  Status s;
  Natural A = natural_new();
  u32 B = 11;
  Natural Q = natural_new();
  u32 R;
  Natural exp_Q = natural_new();
  u32 exp_R = 0;

  s = natural_set(alloc, 0, &A); checkStatus(s);
  s = natural_set(alloc, 0, &exp_Q); checkStatus(s);
  s = natural_divDigit(alloc, &A, B, &Q, &R); checkStatus(s);

  bool ok = R == exp_R && natural_equal(&Q, &exp_Q) &&
            isValidNatural(&Q);
  natural_free(alloc, A);
  natural_free(alloc, Q);
  natural_free(alloc, exp_Q);
  return ok && isAllFree();
}

bool test_natural_divDigit_3(void) {
  Status s;
  Natural A = natural_new();
  u32 B = 9;
  Natural Q = natural_new();
  u32 R;
  Natural exp_Q = natural_new();
  u32 exp_R = 0;

  u32 A_DIGS[] = {999999999, 999999999};
  s = natural_setVec(alloc, A_DIGS, 2, &A); checkStatus(s);
  u32 exp_Q_DIGS[] = {111111111, 111111111};
  s = natural_setVec(alloc, exp_Q_DIGS, 2, &exp_Q); checkStatus(s);

  s = natural_divDigit(alloc, &A, B, &Q, &R); checkStatus(s);

  bool ok = R == exp_R && natural_equal(&Q, &exp_Q) &&
            isValidNatural(&Q);
  natural_free(alloc, A);
  natural_free(alloc, Q);
  natural_free(alloc, exp_Q);
  return ok && isAllFree();
}

// basically tests if divDigit respects the division theorem
bool test_natural_divDigit_4(void) {
  Status s;
  Natural A = natural_new();
  u32 B = 1;
  Natural Q = natural_new();
  u32 R;

  s = natural_set(alloc, 36, &A); checkStatus(s);

  while (B < 36) {
    s = natural_divDigit(alloc, &A, B, &Q, &R); checkStatus(s);

    if (B <= R) {
      natural_free(alloc, A);
      natural_free(alloc, Q);
      return false;
    }
    B++;
  }
  natural_free(alloc, A);
  natural_free(alloc, Q);
  return isAllFree();
}

// same thing as the previous test, but with more digits
bool test_natural_divDigit_5(void) {
  Status s;
  Natural A = natural_new();
  u32 B = 1;
  Natural Q = natural_new();
  u32 R;

  u32 A_DIGS[] = {999999999, 999999999};
  s = natural_setVec(alloc, A_DIGS, 2, &A); checkStatus(s);

  while (B < 36) {
    s = natural_divDigit(alloc, &A, B, &Q, &R); checkStatus(s);

    if (B <= R) {
      natural_free(alloc, A);
      natural_free(alloc, Q);
      return false;
    }
    B++;
  }
  natural_free(alloc, A);
  natural_free(alloc, Q);
  return isAllFree();
}

bool test_natural_divDigit_6(void) {
  Status s;
  Natural A = natural_new();
  u32 B = 0;
  Natural Q = natural_new();
  u32 R;
  s = natural_set(alloc, 5, &A); checkStatus(s);
  s = natural_divDigit(alloc, &A, B, &Q, &R);
  natural_free(alloc, A);
  natural_free(alloc, Q);
  return s == status_DIVISIONBYZERO && isAllFree();
}

// tests 1 as identity
bool test_natural_divDigit_7(void) {
  Status s;
  Natural A = natural_new();
  u32 B = 1;
  Natural Q = natural_new();
  u32 R;
  Natural exp_Q = natural_new();
  u32 exp_R = 0;

  s = natural_set(alloc, 42, &A); checkStatus(s);
  s = natural_set(alloc, 42, &exp_Q); checkStatus(s);

  s = natural_divDigit(alloc, &A, B, &Q, &R); checkStatus(s);

  bool ok = R == exp_R && natural_equal(&Q, &exp_Q) &&
            isValidNatural(&Q);
  natural_free(alloc, A);
  natural_free(alloc, Q);
  natural_free(alloc, exp_Q);
  return ok && isAllFree();
}

// tests a/a = 1*a + 0
bool test_natural_divDigit_8(void) {
  Status s;
  Natural A = natural_new();
  u32 B = 42;
  Natural Q = natural_new();
  u32 R;
  Natural exp_Q = natural_new();
  u32 exp_R = 0;

  s = natural_set(alloc, 42, &A); checkStatus(s);
  s = natural_set(alloc, 1, &exp_Q); checkStatus(s);

  s = natural_divDigit(alloc, &A, B, &Q, &R); checkStatus(s);

  bool ok = R == exp_R && natural_equal(&Q, &exp_Q) &&
            isValidNatural(&Q);
  natural_free(alloc, A);
  natural_free(alloc, Q);
  natural_free(alloc, exp_Q);
  return ok && isAllFree();
}

/* END: testing divDigit */

/* BEGIN: testing div */
bool test_natural_div_1(void) {
  Status s;
  Natural A = natural_new();
  Natural B = natural_new();
  Natural Q = natural_new();
  Natural R = natural_new();
  Natural scratch = natural_new();
  Natural exp_Q = natural_new();
  Natural exp_R = natural_new();

  s = natural_set(alloc, 5, &A);     checkStatus(s);
  s = natural_set(alloc, 1, &exp_Q); checkStatus(s);
  s = natural_set(alloc, 3, &B);     checkStatus(s);
  s = natural_set(alloc, 2, &exp_R); checkStatus(s);

  s = natural_div(alloc, &scratch, &A, &B, &Q, &R); checkStatus(s);

  bool ok = natural_equal(&R, &exp_R) &&
            natural_equal(&Q, &exp_Q) &&
            isValidNatural(&Q) &&
            isValidNatural(&R);
  natural_free(alloc, A);
  natural_free(alloc, B);
  natural_free(alloc, Q);
  natural_free(alloc, R);
  natural_free(alloc, scratch);
  natural_free(alloc, exp_Q);
  natural_free(alloc, exp_R);
  return ok && isAllFree();
}

bool test_natural_div_2(void) {
  Status s;
  Natural A = natural_new();
  Natural B = natural_new();
  Natural Q = natural_new();
  Natural R = natural_new();
  Natural scratch = natural_new();
  Natural exp_Q = natural_new();
  Natural exp_R = natural_new();

  s = natural_set(alloc, 0,  &A);    checkStatus(s);
  s = natural_set(alloc, 11, &B);    checkStatus(s);
  s = natural_set(alloc, 0, &exp_Q); checkStatus(s);
  s = natural_set(alloc, 0, &exp_R); checkStatus(s);

  s = natural_div(alloc, &scratch, &A, &B, &Q, &R); checkStatus(s);

  bool ok = natural_equal(&R, &exp_R) &&
            natural_equal(&Q, &exp_Q) &&
            isValidNatural(&R) && 
            isValidNatural(&Q);
  natural_free(alloc, A);
  natural_free(alloc, B);
  natural_free(alloc, Q);
  natural_free(alloc, R);
  natural_free(alloc, scratch);
  natural_free(alloc, exp_Q);
  natural_free(alloc, exp_R);
  return ok && isAllFree();
}

bool test_natural_div_3(void) {
  Status s;
  Natural A = natural_new();
  Natural B = natural_new();
  Natural Q = natural_new();
  Natural R = natural_new();
  Natural scratch = natural_new();
  Natural exp_Q = natural_new();
  Natural exp_R = natural_new();

  u32 A_DIGS[] = {999999999, 999999999};
  s = natural_setVec(alloc, A_DIGS, 2, &A); checkStatus(s);
  s = natural_set(alloc, 9, &B); checkStatus(s);
  u32 exp_Q_DIGS[] = {111111111, 111111111};
  s = natural_setVec(alloc, exp_Q_DIGS, 2, &exp_Q); checkStatus(s);

  s = natural_div(alloc, &scratch, &A, &B, &Q, &R); checkStatus(s);

  bool ok = natural_equal(&R, &exp_R) &&
            natural_equal(&Q, &exp_Q) &&
            isValidNatural(&R) && 
            isValidNatural(&Q);
  natural_free(alloc, A);
  natural_free(alloc, B);
  natural_free(alloc, Q);
  natural_free(alloc, R);
  natural_free(alloc, scratch);
  natural_free(alloc, exp_Q);
  natural_free(alloc, exp_R);
  return ok && isAllFree();
}

// basically tests if divDigit respects the division theorem
bool test_natural_div_4(void) {
  Status s;
  Natural A = natural_new();
  Natural B = natural_new();
  Natural Q = natural_new();
  Natural R = natural_new();
  Natural scratch = natural_new();

  s = natural_set(alloc, 36, &A); checkStatus(s);
  s = natural_set(alloc, 1, &B); checkStatus(s);

  while (natural_compareDigit(&B, 36) == order_LESS) {
    s = natural_div(alloc, &scratch, &A, &B, &Q, &R);
    checkStatus(s);

    if (!(natural_compare(&R, &B) == order_LESS)) {
      natural_free(alloc, A);
      natural_free(alloc, B);
      natural_free(alloc, Q);
      natural_free(alloc, R);
      natural_free(alloc, scratch);
      return false;
    }
    s = natural_addDigit(alloc, &B, 1, &B); checkStatus(s);
  }
  natural_free(alloc, A);
  natural_free(alloc, B);
  natural_free(alloc, Q);
  natural_free(alloc, R);
  natural_free(alloc, scratch);
  return true && isAllFree();
}

// same thing as the previous test, but with more digits
bool test_natural_div_5(void) {
  Status s;
  Natural A = natural_new();
  Natural B = natural_new();
  Natural Q = natural_new();
  Natural R = natural_new();
  Natural scratch = natural_new();

  u32 A_DIGS[] = {999999999, 999999999};
  s = natural_setVec(alloc, A_DIGS, 2, &A); checkStatus(s);
  s = natural_set(alloc, 1, &B); checkStatus(s);
  s = natural_set(alloc, 0, &R); checkStatus(s);

  while (natural_compareDigit(&B, natural_BASE) == order_LESS) {
    s = natural_div(alloc, &scratch, &A, &B, &Q, &R); checkStatus(s);

    if (!(natural_compare(&R, &B) == order_LESS)) {
      natural_free(alloc, A);
      natural_free(alloc, B);
      natural_free(alloc, Q);
      natural_free(alloc, R);
      natural_free(alloc, scratch);
      return false;
    }
    s = natural_addDigit(alloc, &B, natural_BASE/10, &B); checkStatus(s);
  }
  natural_free(alloc, A);
  natural_free(alloc, B);
  natural_free(alloc, Q);
  natural_free(alloc, R);
  natural_free(alloc, scratch);
  return true && isAllFree();
}

bool test_natural_div_6(void) {
  Status s;
  Natural A = natural_new();
  Natural B = natural_new();
  Natural Q = natural_new();
  Natural R = natural_new();
  Natural scratch = natural_new();

  s = natural_set(alloc, 5, &A); checkStatus(s);
  s = natural_set(alloc, 0, &B); checkStatus(s);

  s = natural_div(alloc, &scratch, &A, &B, &Q, &R);
  natural_free(alloc, A);
  natural_free(alloc, B);
  natural_free(alloc, Q);
  natural_free(alloc, R);
  natural_free(alloc, scratch);
  return s == status_DIVISIONBYZERO && isAllFree();
}

// tests 1 as identity
bool test_natural_div_7(void) {
  Status s;
  Natural A = natural_new();
  Natural B = natural_new();
  Natural Q = natural_new();
  Natural R = natural_new();
  Natural scratch = natural_new();
  Natural exp_Q = natural_new();
  Natural exp_R = natural_new();

  s = natural_set(alloc, 42, &A);     checkStatus(s);
  s = natural_set(alloc, 1, &B);      checkStatus(s);
  s = natural_set(alloc, 42, &exp_Q); checkStatus(s);
  s = natural_set(alloc, 0, &exp_R);  checkStatus(s);

  s = natural_div(alloc, &scratch, &A, &B, &Q, &R); checkStatus(s);

  bool ok = natural_equal(&R, &exp_R) &&
            natural_equal(&Q, &exp_Q) && 
            isValidNatural(&R) &&
            isValidNatural(&Q);
  natural_free(alloc, A);
  natural_free(alloc, B);
  natural_free(alloc, Q);
  natural_free(alloc, R);
  natural_free(alloc, scratch);
  natural_free(alloc, exp_Q);
  natural_free(alloc, exp_R);
  return ok && isAllFree();
}

// tests a/a = 1*a + 0
bool test_natural_div_8(void) {
  Status s;
  Natural A = natural_new();
  Natural B = natural_new();
  Natural Q = natural_new();
  Natural R = natural_new();
  Natural scratch = natural_new();
  Natural exp_Q = natural_new();
  Natural exp_R = natural_new();

  s = natural_set(alloc, 42, &A);    checkStatus(s);
  s = natural_set(alloc, 42, &B);    checkStatus(s);
  s = natural_set(alloc, 1, &exp_Q); checkStatus(s);
  s = natural_set(alloc, 0, &exp_R); checkStatus(s);

  s = natural_div(alloc, &scratch, &A, &B, &Q, &R); checkStatus(s);

  bool ok = natural_equal(&R, &exp_R) &&
            natural_equal(&Q, &exp_Q) &&
            isValidNatural(&R) &&
            isValidNatural(&Q);
  natural_free(alloc, A);
  natural_free(alloc, B);
  natural_free(alloc, Q);
  natural_free(alloc, R);
  natural_free(alloc, scratch);
  natural_free(alloc, exp_Q);
  natural_free(alloc, exp_R);
  return ok && isAllFree();
}
/* END: testing div */
/* BEGIN: Algorithm D specific tests */

/* Forces the D6 add-back step.
   Classical construction from Knuth TAOCP 4.3.1:
   Choose B with MSD = BASE/2 (minimum post-normalization) so that the
   quotient estimate can overshoot. We use:
     B = [BASE-1, BASE/2]  (2 digits, LSD-first: digits[0]=BASE-1, digits[1]=BASE/2)
   and pick A so that A = B * (BASE-1) + (B-1), forcing qhat to overestimate.
   We verify correctness via the division theorem: A == Q*B + R and R < B.
*/
bool test_natural_div_addback(void) {
  Status s;
  Natural A  = natural_new();
  Natural B  = natural_new();
  Natural Q  = natural_new();
  Natural R  = natural_new();
  Natural scratch = natural_new();
  Natural tmp = natural_new();
  Natural lhs = natural_new();

  /* B = BASE/2 * BASE + (BASE-1)  i.e. digits[1]=BASE/2, digits[0]=BASE-1 */
  u32 B_digs[] = {natural_BASE - 1, natural_BASE / 2};
  s = natural_setVec(alloc, B_digs, 2, &B); checkStatus(s);

  /* A = B * (BASE-1) + (B-1).
     We build A = B*(BASE-1) using mult, then add B-1 using distance/add. */
  Natural coeff = natural_new();
  s = natural_set(alloc, natural_BASE - 1, &coeff); checkStatus(s);
  s = natural_mult(alloc, &B, &coeff, &A); checkStatus(s);
  natural_free(alloc, coeff);

  /* add (B - 1) to A: add B then subtract 1 */
  Natural Bminus1 = natural_new();
  s = natural_copy(alloc, &B, &Bminus1); checkStatus(s);
  /* Bminus1 = B - 1 via distanceDigit (B > 0 so |B-0|-1... easier: addDigit -1) */
  /* We do: tmp = B; tmp -= 1 */
  u32 one_digs[] = {1};
  Natural one = natural_new();
  s = natural_setVec(alloc, one_digs, 1, &one); checkStatus(s);
  s = natural_distance(alloc, &Bminus1, &one, &Bminus1); checkStatus(s);
  natural_free(alloc, one);
  s = natural_add(alloc, &A, &Bminus1, &A); checkStatus(s);
  natural_free(alloc, Bminus1);

  s = natural_div(alloc, &scratch, &A, &B, &Q, &R); checkStatus(s);

  /* verify division theorem: A == Q*B + R */
  s = natural_mult(alloc, &Q, &B, &tmp); checkStatus(s);
  s = natural_add(alloc, &tmp, &R, &lhs); checkStatus(s);
  bool theorem = natural_equal(&lhs, &A);

  /* verify 0 <= R < B */
  bool rLessB = natural_compare(&R, &B) == order_LESS;

  bool ok = theorem && rLessB && isValidNatural(&Q) && isValidNatural(&R);

  natural_free(alloc, A);
  natural_free(alloc, B);
  natural_free(alloc, Q);
  natural_free(alloc, R);
  natural_free(alloc, scratch);
  natural_free(alloc, tmp);
  natural_free(alloc, lhs);
  return ok && isAllFree();
}

/* Single-digit divisor fast path: exercises the B.len==1 delegation. */
bool test_natural_div_single_digit_divisor(void) {
  Status s;
  Natural A  = natural_new();
  Natural B  = natural_new();
  Natural Q1 = natural_new();
  Natural R1 = natural_new();
  Natural Q2 = natural_new();
  Natural scratch = natural_new();
  u32 r2 = 0;

  /* A = 999999999 * BASE^2 + 123456789 * BASE + 7 (3 digits) */
  u32 A_digs[] = {7, 123456789, 999999999};
  s = natural_setVec(alloc, A_digs, 3, &A); checkStatus(s);
  s = natural_set(alloc, 3, &B); checkStatus(s);

  s = natural_div(alloc, &scratch, &A, &B, &Q1, &R1); checkStatus(s);
  s = natural_divDigit(alloc, &A, 3, &Q2, &r2); checkStatus(s);

  Natural R2 = natural_new();
  s = natural_set(alloc, r2, &R2); checkStatus(s);

  bool ok = natural_equal(&Q1, &Q2) &&
            natural_equal(&R1, &R2) &&
            isValidNatural(&Q1) &&
            isValidNatural(&R1);

  natural_free(alloc, A);
  natural_free(alloc, B);
  natural_free(alloc, Q1);
  natural_free(alloc, R1);
  natural_free(alloc, Q2);
  natural_free(alloc, R2);
  natural_free(alloc, scratch);
  return ok && isAllFree();
}

/* Equal-length operands: m == 0, quotient is a single digit. */
bool test_natural_div_equal_length(void) {
  Status s;
  Natural A  = natural_new();
  Natural B  = natural_new();
  Natural Q  = natural_new();
  Natural R  = natural_new();
  Natural scratch = natural_new();
  Natural expQ = natural_new();
  Natural expR = natural_new();

  /* A = [1, 7] MSD-first => 1*BASE + 7 (setVec receives MSD first)
     B = [1, 3] MSD-first => 1*BASE + 3
     A / B = 1 remainder (BASE+7) - (BASE+3) = 4 */
  u32 A_digs[] = {1, 7};
  u32 B_digs[] = {1, 3};
  s = natural_setVec(alloc, A_digs, 2, &A); checkStatus(s);
  s = natural_setVec(alloc, B_digs, 2, &B); checkStatus(s);
  s = natural_set(alloc, 1, &expQ); checkStatus(s);
  s = natural_set(alloc, 4, &expR); checkStatus(s);

  s = natural_div(alloc, &scratch, &A, &B, &Q, &R); checkStatus(s);

  bool ok = natural_equal(&Q, &expQ) &&
            natural_equal(&R, &expR) &&
            isValidNatural(&Q) &&
            isValidNatural(&R);

  natural_free(alloc, A);
  natural_free(alloc, B);
  natural_free(alloc, Q);
  natural_free(alloc, R);
  natural_free(alloc, scratch);
  natural_free(alloc, expQ);
  natural_free(alloc, expR);
  return ok && isAllFree();
}

/* Large quotient: A has 5 digits, B has 2 — produces a 4-digit quotient. */
bool test_natural_div_large_quotient(void) {
  Status s;
  Natural A  = natural_new();
  Natural B  = natural_new();
  Natural Q  = natural_new();
  Natural R  = natural_new();
  Natural scratch = natural_new();
  Natural tmp = natural_new();
  Natural lhs = natural_new();

  /* A = BASE^4 * 2 + BASE^3 * 3 + BASE^2 * 5 + BASE * 7 + 11 */
  u32 A_digs[] = {11, 7, 5, 3, 2};
  s = natural_setVec(alloc, A_digs, 5, &A); checkStatus(s);
  /* B = BASE + 1 */
  u32 B_digs[] = {1, 1};
  s = natural_setVec(alloc, B_digs, 2, &B); checkStatus(s);

  s = natural_div(alloc, &scratch, &A, &B, &Q, &R); checkStatus(s);

  /* verify: A == Q*B + R and R < B */
  s = natural_mult(alloc, &Q, &B, &tmp); checkStatus(s);
  s = natural_add(alloc, &tmp, &R, &lhs); checkStatus(s);
  bool theorem = natural_equal(&lhs, &A);
  bool rLessB  = natural_compare(&R, &B) == order_LESS;
  bool qLen    = Q.len == 4; /* 5-digit / 2-digit => 4-digit quotient */

  bool ok = theorem && rLessB && qLen && isValidNatural(&Q) && isValidNatural(&R);
  natural_free(alloc, A);
  natural_free(alloc, B);
  natural_free(alloc, Q);
  natural_free(alloc, R);
  natural_free(alloc, scratch);
  natural_free(alloc, tmp);
  natural_free(alloc, lhs);
  return ok && isAllFree();
}

/* Maximum digit values: all digits at BASE-1 = 999999999. */
bool test_natural_div_max_digits(void) {
  Status s;
  Natural A  = natural_new();
  Natural B  = natural_new();
  Natural Q  = natural_new();
  Natural R  = natural_new();
  Natural scratch = natural_new();
  Natural tmp = natural_new();
  Natural lhs = natural_new();

  /* A = [MAX, MAX, MAX, MAX] = BASE^4 - 1 */
  u32 MAX = natural_BASE - 1;
  u32 A_digs[] = {MAX, MAX, MAX, MAX};
  /* B = [MAX, MAX] = BASE^2 - 1 */
  u32 B_digs[] = {MAX, MAX};
  s = natural_setVec(alloc, A_digs, 4, &A); checkStatus(s);
  s = natural_setVec(alloc, B_digs, 2, &B); checkStatus(s);

  s = natural_div(alloc, &scratch, &A, &B, &Q, &R); checkStatus(s);

  s = natural_mult(alloc, &Q, &B, &tmp); checkStatus(s);
  s = natural_add(alloc, &tmp, &R, &lhs); checkStatus(s);
  bool theorem = natural_equal(&lhs, &A);
  bool rLessB  = natural_compare(&R, &B) == order_LESS;

  bool ok = theorem && rLessB && isValidNatural(&Q) && isValidNatural(&R);
  natural_free(alloc, A);
  natural_free(alloc, B);
  natural_free(alloc, Q);
  natural_free(alloc, R);
  natural_free(alloc, scratch);
  natural_free(alloc, tmp);
  natural_free(alloc, lhs);
  return ok && isAllFree();
}

/* Property test: for several multi-digit pairs verify A == Q*B + R and R < B. */
bool test_natural_div_theorem_multidigit(void) {
  Status s;
  Natural A  = natural_new();
  Natural B  = natural_new();
  Natural Q  = natural_new();
  Natural R  = natural_new();
  Natural scratch = natural_new();
  Natural tmp = natural_new();
  Natural lhs = natural_new();

  /* table of (A digits MSD-first, A len, B digits MSD-first, B len) */
  struct { u32 a[5]; int alen; u32 b[3]; int blen; } cases[] = {
    /* 3-digit / 2-digit */
    {{500000001, 999999999, 123456789}, 3, {999999998, 500000001}, 2},
    /* 4-digit / 2-digit */
    {{1, 0, 0, 1}, 4, {999999999, 999999999}, 2},
    /* 4-digit / 3-digit */
    {{7, 3, 5, 2}, 4, {6, 1, 2}, 3},
    /* 5-digit / 2-digit, small divisor */
    {{1, 1, 1, 1, 1}, 5, {1, 2}, 2},
    /* equal length: 3-digit / 3-digit */
    {{999999998, 500000000, 1}, 3, {999999999, 500000000, 1}, 3},
  };
  int ncases = (int)(sizeof(cases) / sizeof(cases[0]));

  bool ok = true;
  int i = 0;
  while (i < ncases && ok) {
    natural_set(alloc, 0, &A); /* reset without free — just clear len */
    natural_set(alloc, 0, &B);
    natural_set(alloc, 0, &Q);
    natural_set(alloc, 0, &R);
    s = natural_setVec(alloc, cases[i].a, cases[i].alen, &A); checkStatus(s);
    s = natural_setVec(alloc, cases[i].b, cases[i].blen, &B); checkStatus(s);
    s = natural_div(alloc, &scratch, &A, &B, &Q, &R); checkStatus(s);

    natural_set(alloc, 0, &tmp);
    natural_set(alloc, 0, &lhs);
    s = natural_mult(alloc, &Q, &B, &tmp); checkStatus(s);
    s = natural_add(alloc, &tmp, &R, &lhs); checkStatus(s);

    bool theorem = natural_equal(&lhs, &A);
    bool rLessB  = natural_compare(&R, &B) == order_LESS;
    ok = ok && theorem && rLessB && isValidNatural(&Q) && isValidNatural(&R);
    i++;
  }

  natural_free(alloc, A);
  natural_free(alloc, B);
  natural_free(alloc, Q);
  natural_free(alloc, R);
  natural_free(alloc, scratch);
  natural_free(alloc, tmp);
  natural_free(alloc, lhs);
  return ok && isAllFree();
}
/* END: Algorithm D specific tests */

/* BEGIN: testing gcd */
static bool i_naturalTest_gcdCheck(u32 a, u32 b, u32 expected) {
  Status s;
  Natural A = natural_new(), B = natural_new(), out = natural_new();
  Natural scr_a = natural_new(), scr_b = natural_new();
  Natural scr_div = natural_new(), scr_q = natural_new();

  s = natural_set(alloc, a, &A); checkStatus(s);
  s = natural_set(alloc, b, &B); checkStatus(s);

  s = natural_gcd(alloc, &A, &B, &out, &scr_a, &scr_b, &scr_div, &scr_q); checkStatus(s);

  bool ok = natural_equalDigit(&out, expected) && isValidNatural(&out);

  natural_free(alloc, A);
  natural_free(alloc, B);
  natural_free(alloc, out);
  natural_free(alloc, scr_a);
  natural_free(alloc, scr_b);
  natural_free(alloc, scr_div);
  natural_free(alloc, scr_q);
  return ok && isAllFree();
}

// classic Euclid example: gcd(1071, 462) = 21
bool test_natural_gcd_1(void) {
  return i_naturalTest_gcdCheck(1071, 462, 21);
}

// a multiple of b: gcd(48, 18) = 6
bool test_natural_gcd_2(void) {
  return i_naturalTest_gcdCheck(48, 18, 6);
}

// gcd(0, n) = n
bool test_natural_gcd_zeroA(void) {
  return i_naturalTest_gcdCheck(0, 5, 5);
}

// gcd(n, 0) = n
bool test_natural_gcd_zeroB(void) {
  return i_naturalTest_gcdCheck(5, 0, 5);
}

// gcd(0, 0) = 0
bool test_natural_gcd_bothZero(void) {
  return i_naturalTest_gcdCheck(0, 0, 0);
}

// coprime numbers: gcd(17, 13) = 1
bool test_natural_gcd_coprime(void) {
  return i_naturalTest_gcdCheck(17, 13, 1);
}

// gcd(n, n) = n
bool test_natural_gcd_equal(void) {
  return i_naturalTest_gcdCheck(314159, 314159, 314159);
}
/* END: testing gcd */

/* BEGIN: testing copy */
bool test_natural_copy_1(void) {
  Natural a = natural_new();
  Natural out = natural_new();
  Status s;

  s = natural_set(alloc, 2222, &a); checkStatus(s);
  s = natural_copy(alloc, &a, &out); checkStatus(s);
  bool ok = natural_equal(&out, &a) &&
            isValidNatural(&a);

  natural_free(alloc, a);
  natural_free(alloc, out);

  return ok && isAllFree();
}

bool test_natural_copy_2(void) {
  Status s;
  Natural a = natural_new();
  Natural out = natural_new();

  u32 A_DIGS[] = {999999999, 999999999};
  s = natural_setVec(alloc, A_DIGS, 2, &a); checkStatus(s);
  s = natural_copy(alloc, &a, &out); checkStatus(s);

  bool ok = natural_equal(&a, &out) &&
            isValidNatural(&out);
  natural_free(alloc, a);
  natural_free(alloc, out);

  return ok && isAllFree();
}

// must work even if has garbage in the out param
bool test_natural_copy_3(void) {
  Status s;
  Natural a = natural_new();
  Natural out = natural_new();

  u32 A_DIGS[] = {999999999, 999999999};
  s = natural_setVec(alloc, A_DIGS, 2, &a); checkStatus(s);
  s = natural_set(alloc, 123456, &out); checkStatus(s);

  s = natural_copy(alloc, &a, &out); checkStatus(s);
  bool ok = natural_equal(&a, &out) &&
            isValidNatural(&out);
  natural_free(alloc, a);
  natural_free(alloc, out);
  return ok && isAllFree();
}
/* END: testing copy*/

/* BEGIN: grow/shrink tests */
// addDigit/distanceDigit
bool test_natural_growShrink_1(void) {
  Status s;
  Natural A = natural_new();
  u32 B = natural_BASE-1;
  Natural C = natural_new();

  s = natural_set(alloc, 0, &A); checkStatus(s);
  s = natural_set(alloc, 0, &C); checkStatus(s);;

  int i = 0;
  while (i < 100) {
    s = natural_addDigit(alloc, &A, B, &C); checkStatus(s);
    s = natural_copy(alloc, &C, &A); checkStatus(s);
    i++;
  }

  while (0 < i) {
    s = natural_distanceDigit(alloc, &A, B, &C); checkStatus(s);
    s = natural_copy(alloc, &C, &A); checkStatus(s);
    i--;
  }

  bool ok = natural_isZero(&A) &&
            isValidNatural(&A);
  natural_free(alloc, A);
  natural_free(alloc, C);
  return ok && isAllFree();
}

// add/distance
bool test_natural_growShrink_2(void) {
  Status s;
  Natural A = natural_new();
  Natural B = natural_new();
  Natural C = natural_new();

  s = natural_set(alloc, 0, &A); checkStatus(s);
  s = natural_set(alloc, natural_BASE-1, &B); checkStatus(s);
  s = natural_set(alloc, 0, &C); checkStatus(s);;

  int i = 0;
  while (i < 100) {
    s = natural_add(alloc, &A, &B, &C); checkStatus(s);
    s = natural_copy(alloc, &C, &A); checkStatus(s);
    i++;
  }

  while (0 < i) {
    s = natural_distance(alloc, &A, &B, &C); checkStatus(s);
    s = natural_copy(alloc, &C, &A); checkStatus(s);
    i--;
  }

  bool ok = natural_isZero(&A) && isValidNatural(&A);
  natural_free(alloc, A);
  natural_free(alloc, B);
  natural_free(alloc, C);
  return ok && isAllFree();
}

// multDigit/divDigit
bool test_natural_growShrink_3(void) {
  Status s;
  Natural A = natural_new();
  u32 B = 8;
  Natural C = natural_new();
  u32 R = 0;
  const int maxIter = 16;

  s = natural_set(alloc, 1, &A); checkStatus(s);
  s = natural_set(alloc, 1, &C); checkStatus(s);;

  int i = 0;
  while (i < maxIter) {
    s = natural_multDigit(alloc, &A, B, &C); checkStatus(s);
    s = natural_copy(alloc, &C, &A); checkStatus(s);
    i++;
  }

  while (0 < i) {
    s = natural_divDigit(alloc, &A, B, &C, &R); checkStatus(s);
    s = natural_copy(alloc, &C, &A); checkStatus(s);
    if (R != 0) {
      return false;
    }
    i--;
  }

  bool ok = natural_equalDigit(&A, 1) && isValidNatural(&A);
  natural_free(alloc, A);
  natural_free(alloc, C);
  return ok && isAllFree();
}

// aliased addDigit/distanceDigit
bool test_natural_growShrink_4(void) {
  Status s;
  u32 B = natural_BASE-1;
  Natural C = natural_new();

  s = natural_set(alloc, 0, &C); checkStatus(s);

  int i = 0;
  while (i < 100) {
    s = natural_addDigit(alloc, &C, B, &C); checkStatus(s);
    i++;
  }

  while (0 < i) {
    s = natural_distanceDigit(alloc, &C, B, &C); checkStatus(s);
    i--;
  }

  bool ok = natural_isZero(&C) && isValidNatural(&C);
  natural_free(alloc, C);
  return ok && isAllFree();
}

// aliased add/distance
bool test_natural_growShrink_5(void) {
  Status s;
  Natural B = natural_new();
  Natural C = natural_new();

  s = natural_set(alloc, natural_BASE-1, &B); checkStatus(s);
  s = natural_set(alloc, 0, &C); checkStatus(s);

  int i = 0;
  while (i < 100) {
    s = natural_add(alloc, &C, &B, &C); checkStatus(s);
    i++;
  }

  while (0 < i) {
    s = natural_distance(alloc, &C, &B, &C); checkStatus(s);
    i--;
  }

  bool ok = natural_isZero(&C) && isValidNatural(&C);
  natural_free(alloc, B);
  natural_free(alloc, C);
  return ok && isAllFree();
}

// mult/div
bool test_natural_growShrink_6(void) {
  Status s;
  Natural A = natural_new();
  Natural B = natural_new();
  Natural C = natural_new();
  Natural R = natural_new();
  Natural scratch = natural_new();
  const int maxIter = 16;

  s = natural_set(alloc, 1, &A); checkStatus(s);
  s = natural_set(alloc, 8, &B); checkStatus(s);
  s = natural_set(alloc, 1, &C); checkStatus(s);;
  s = natural_set(alloc, 0, &R); checkStatus(s);;

  int i = 0;
  while (i < maxIter) {
    s = natural_mult(alloc, &A, &B, &C); checkStatus(s);
    s = natural_copy(alloc, &C, &A); checkStatus(s);
    i++;
  }

  while (0 < i) {
    s = natural_div(alloc, &scratch, &A, &B, &C, &R); checkStatus(s);
    s = natural_copy(alloc, &C, &A); checkStatus(s);
    if (!natural_isZero(&R)) {
      natural_free(alloc, A);
      natural_free(alloc, B);
      natural_free(alloc, C);
      natural_free(alloc, R);
      natural_free(alloc, scratch);
      return false;
    }
    i--;
  }

  bool ok = natural_equalDigit(&A, 1) && isValidNatural(&A);
  natural_free(alloc, A);
  natural_free(alloc, B);
  natural_free(alloc, C);
  natural_free(alloc, R);
  natural_free(alloc, scratch);
  return ok && isAllFree();
}
/* END: grow/shrink tests */

/* BEGIN: testing allocator failure */
// fails on the very first allocation `natural_set` needs to make.
bool test_natural_alloc_fails_set(void) {
  i_FailAllocHeap heap;
  IAllocator fa = i_failAlloc_new(&heap, 0);
  Natural out = natural_new();

  Status s = natural_set(&fa, 42, &out);
  bool ok = s == status_OUTOFMEMORY && isEmptyNat(out);

  natural_free(&fa, out); // safe: digits is still NULL
  return ok;
}

// fails on the first allocation `natural_setVec` needs, before any
// digit has been pushed.
bool test_natural_alloc_fails_setVec_firstAlloc(void) {
  i_FailAllocHeap heap;
  IAllocator fa = i_failAlloc_new(&heap, 0);
  Natural out = natural_new();
  u32 digits[] = {1, 2, 3};

  Status s = natural_setVec(&fa, digits, 3, &out);
  bool ok = s == status_OUTOFMEMORY && isEmptyNat(out);

  natural_free(&fa, out);
  return ok;
}

// allows the initial cap-4 allocation to succeed, but fails the grow
// that's needed to fit a 5th digit. Checks that a failed grow leaves
// the previously-allocated buffer, its length and its contents intact
// rather than corrupting or leaking it.
bool test_natural_alloc_fails_setVec_growAlloc(void) {
  i_FailAllocHeap heap;
  IAllocator fa = i_failAlloc_new(&heap, 1);
  Natural out = natural_new();
  u32 digits[] = {5, 4, 3, 2, 1}; // MSD -> LSD

  Status s = natural_setVec(&fa, digits, 5, &out);
  u32 expected[] = {1, 2, 3, 4}; // digits pushed before the grow failed (LSD-first)

  bool ok = s == status_OUTOFMEMORY &&
            out.cap == natural_MINNATVEC &&
            out.len == 4 &&
            out.digits != NULL &&
            memcmp(out.digits, expected, 4*sizeof(u32)) == 0;

  natural_free(&fa, out);
  return ok;
}

// `out` starts with no capacity, so `natural_copy` must allocate;
// fails that allocation and checks `out` is left untouched.
bool test_natural_alloc_fails_copy(void) {
  i_FailAllocHeap heap;
  IAllocator fa = i_failAlloc_new(&heap, -1); // never fail, for setup
  Natural a = natural_new();
  Natural out = natural_new();

  Status s = natural_set(&fa, 314159, &a); checkStatus(s);

  heap.allocsUntilFail = 0; // now make the next allocation fail
  s = natural_copy(&fa, &a, &out);
  bool ok = s == status_OUTOFMEMORY && isEmptyNat(out);

  natural_free(&fa, a);
  natural_free(&fa, out);
  return ok;
}

// `out` starts empty, so `natural_add` must allocate on its very
// first digit; fails that allocation and checks `out` is left untouched.
bool test_natural_alloc_fails_add(void) {
  i_FailAllocHeap heap;
  IAllocator fa = i_failAlloc_new(&heap, -1); // never fail, for setup
  Natural a = natural_new();
  Natural b = natural_new();
  Natural out = natural_new();

  Status s = natural_set(&fa, 1, &a); checkStatus(s);
  s = natural_set(&fa, 1, &b); checkStatus(s);

  heap.allocsUntilFail = 0; // now make the next allocation fail
  s = natural_add(&fa, &a, &b, &out);
  bool ok = s == status_OUTOFMEMORY && isEmptyNat(out);

  natural_free(&fa, a);
  natural_free(&fa, b);
  natural_free(&fa, out);
  return ok;
}
/* END: testing allocator failure */

/* BEGIN: DRIVER CODE */
Tester tests[] = {
  {"test_natural_snprint_0", test_natural_snprint_0},
  {"test_natural_snprint_1", test_natural_snprint_1},
  {"test_natural_snprint_2", test_natural_snprint_2},
  {"test_natural_snprint_3", test_natural_snprint_3},
  {"test_natural_snprint_bufferSize1_0", test_natural_snprint_bufferSize1_0},

  {"test_natural_printingSize_zero", test_natural_printingSize_zero},
  {"test_natural_printingSize_singleLimb", test_natural_printingSize_singleLimb},
  {"test_natural_printingSize_multiLimb", test_natural_printingSize_multiLimb},
  {"test_natural_printingSize_matchesSnprint", test_natural_printingSize_matchesSnprint},

  {"test_natural_addDigit_0", test_natural_addDigit_0},
  {"test_natural_addDigit_1", test_natural_addDigit_1},
  {"test_natural_addDigit_2", test_natural_addDigit_2},
  {"test_natural_addDigit_3", test_natural_addDigit_3},
  {"test_natural_addDigit_4a", test_natural_addDigit_4a},
  {"test_natural_addDigit_4b", test_natural_addDigit_4b},
  {"test_natural_addDigit_5", test_natural_addDigit_5},

  {"test_natural_distanceDigit_1", test_natural_distanceDigit_1},
  {"test_natural_distanceDigit_2", test_natural_distanceDigit_2},
  {"test_natural_distanceDigit_3", test_natural_distanceDigit_3},
  {"test_natural_distanceDigit_4", test_natural_distanceDigit_4},
  {"test_natural_distanceDigit_5a", test_natural_distanceDigit_5a},
  {"test_natural_distanceDigit_5b", test_natural_distanceDigit_5b},

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
  {"test_natural_div_addback", test_natural_div_addback},
  {"test_natural_div_single_digit_divisor", test_natural_div_single_digit_divisor},
  {"test_natural_div_equal_length", test_natural_div_equal_length},
  {"test_natural_div_large_quotient", test_natural_div_large_quotient},
  {"test_natural_div_max_digits", test_natural_div_max_digits},
  {"test_natural_div_theorem_multidigit", test_natural_div_theorem_multidigit},

  {"test_natural_gcd_1", test_natural_gcd_1},
  {"test_natural_gcd_2", test_natural_gcd_2},
  {"test_natural_gcd_zeroA", test_natural_gcd_zeroA},
  {"test_natural_gcd_zeroB", test_natural_gcd_zeroB},
  {"test_natural_gcd_bothZero", test_natural_gcd_bothZero},
  {"test_natural_gcd_coprime", test_natural_gcd_coprime},
  {"test_natural_gcd_equal", test_natural_gcd_equal},

  {"test_natural_copy_1", test_natural_copy_1},
  {"test_natural_copy_2", test_natural_copy_2},
  {"test_natural_copy_3", test_natural_copy_3},

  {"test_natural_add_0a", test_natural_add_0a},
  {"test_natural_add_0b", test_natural_add_0b},
  {"test_natural_add_1a", test_natural_add_1a},
  {"test_natural_add_1b", test_natural_add_1b},
  {"test_natural_add_2",  test_natural_add_2},
  {"test_natural_add_3a",  test_natural_add_3a},
  {"test_natural_add_3b",  test_natural_add_3b},

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
  {"test_natural_distance_4a", test_natural_distance_4a},
  {"test_natural_distance_4b", test_natural_distance_4b},
  {"test_natural_distance_4c", test_natural_distance_4c},

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

  {"test_natural_alloc_fails_set", test_natural_alloc_fails_set},
  {"test_natural_alloc_fails_setVec_firstAlloc", test_natural_alloc_fails_setVec_firstAlloc},
  {"test_natural_alloc_fails_setVec_growAlloc", test_natural_alloc_fails_setVec_growAlloc},
  {"test_natural_alloc_fails_copy", test_natural_alloc_fails_copy},
  {"test_natural_alloc_fails_add", test_natural_alloc_fails_add},
};
#define TEST_LEN (int)(sizeof(tests) / sizeof(tests[0]))

int main(void) {
  _alloc = i_failAlloc_new(&_heap, -1); /* never fail */
  run_tests("natural", tests, TEST_LEN);
}
/* END: DRIVER CODE */
