// #include "../src/pao_natural.h"
#include "../../lib/numbers/pao_integer.h"
#include "../../lib/pao_basicTypes.h"
#include "../../lib/pao_status.h"
#include "../../lib/alloc/pao_stdAlloc.h"
#include <stdio.h>
#include <strings.h>
#include "../common.h"

char buffer[DEFAULT_SIZE];

pao_Allocator _alloc;
#define alloc (&_alloc)

bool isValidInteger(pao_Integer* n) {
  if (n->abs.len == 0 && n->sign == +1) {
    if (n->sign == +1) {
      return true;
    } else {
      return false;
    }
  }

  // tests for invalid zero and invalid leading zeroes
  if (n->abs.digits[n->abs.len-1] == 0) {
    return false;
  }

  // invalid digits
  u32 i = 0;
  while (i < n->abs.len) {
    if (n->abs.digits[i] >= PAO_natural_base) {
      return false;
    }
    i++;
  }
  return true;
}

bool isAllFree(void) {
  return alloc->info(alloc->heap).used == 0;
}

void printInt(pao_Integer* i) {
  usize written = pao_integer_snprint(i, buffer, DEFAULT_SIZE);
  if (written == 0) {
    printf("written 0 bytes.\n");
  }
  printf("%.*s", (int)written, buffer);
}

char test_buffer[DEFAULT_SIZE];

bool test_integer_snprint_0(void) {
  pao_Status s;
  pao_Integer A = pao_integer_new();
  s = pao_integer_set(alloc, 1, &A); checkStatus(s);

  usize written = pao_integer_snprint(&A, test_buffer, DEFAULT_SIZE);
  if (written == 0) {
    pao_integer_free(alloc, A);
    return false;
  }

  if (strncmp("1", test_buffer, written) != 0) {
    pao_integer_free(alloc, A);
    return false;
  }
  pao_integer_free(alloc, A);
  return true;
}

bool test_integer_snprint_1(void) {
  pao_Status s;
  pao_Integer A = pao_integer_new();
  s = pao_integer_set(alloc, -1, &A); checkStatus(s);

  usize written = pao_integer_snprint(&A, test_buffer, DEFAULT_SIZE);
  if (written == 0) {
    pao_integer_free(alloc, A);
    return false;
  }

  if (strncmp("-1", test_buffer, written) != 0) {
    pao_integer_free(alloc, A);
    return false;
  }
  pao_integer_free(alloc, A);
  return true;
}

bool test_integer_snprint_2(void) {
  pao_Status s;
  pao_Integer A = pao_integer_new();

  u32 A_digs[] = {1, 111222333};
  s = pao_integer_setVec(alloc, -1, A_digs, 2, &A); checkStatus(s);

  usize written = pao_integer_snprint(&A, test_buffer, DEFAULT_SIZE);
  if (written == 0) {
    pao_integer_free(alloc, A);
    return false;
  }

  if (strncmp("-1111222333", test_buffer, written) != 0) {
    pao_integer_free(alloc, A);
    return false;
  }
  pao_integer_free(alloc, A);
  return true;
}

bool test_integer_growShrink_1a(void) {
  pao_Status s;
  pao_Integer A = pao_integer_new();
  pao_Integer C = pao_integer_new();

  s = pao_integer_set(alloc, 7, &A); checkStatus(s);
  s = pao_integer_set(alloc, 0, &C); checkStatus(s);;

  int i = 0;
  while (i < 100) {
    s = pao_integer_add(alloc, &C, &A, &C); checkStatus(s);
    i++;
  }

  while (0 < i) {
    s = pao_integer_sub(alloc, &C, &A, &C); checkStatus(s);
    i--;
  }
  
  bool ok = pao_integer_isZero(&C);
  pao_integer_free(alloc, A);
  pao_integer_free(alloc, C);
  return ok && isAllFree();
}

bool test_integer_growShrink_1b(void) {
  pao_Status s;
  pao_Integer A = pao_integer_new();
  pao_Integer C = pao_integer_new();

  s = pao_integer_set(alloc, -7, &A); checkStatus(s);
  s = pao_integer_set(alloc, 0, &C); checkStatus(s);;

  int i = 0;
  while (i < 100) {
    s = pao_integer_add(alloc, &C, &A, &C); checkStatus(s);
    i++;
  }

  while (0 < i) {
    s = pao_integer_sub(alloc, &C, &A, &C); checkStatus(s);
    i--;
  }
  
  bool ok = pao_integer_isZero(&C);
  pao_integer_free(alloc, A);
  pao_integer_free(alloc, C);
  return ok && isAllFree();
}

// mult/div
bool test_integer_growShrink_2a(void) {
  pao_Status s;
  pao_Integer A = pao_integer_new();
  pao_Integer B = pao_integer_new();
  pao_Integer C = pao_integer_new();
  pao_Integer R = pao_integer_new();
  pao_Natural scratch = pao_natural_new();
  const int maxIter = 16;

  s = pao_integer_set(alloc, 1, &A); checkStatus(s);
  s = pao_integer_set(alloc, 8, &B); checkStatus(s);
  s = pao_integer_set(alloc, 1, &C); checkStatus(s);;
  s = pao_integer_set(alloc, 0, &R); checkStatus(s);;

  int i = 0;
  while (i < maxIter) {
    s = pao_integer_mult(alloc, &A, &B, &C); checkStatus(s);
    s = pao_integer_copy(alloc, &C, &A); checkStatus(s);
    i++;
  }

  while (0 < i) {
    s = pao_integer_div(alloc, &scratch, &A, &B, &C, &R); checkStatus(s);
    s = pao_integer_copy(alloc, &C, &A); checkStatus(s);
    if (!pao_integer_isZero(&R)) {
      pao_integer_free(alloc, A);
      pao_integer_free(alloc, B);
      pao_integer_free(alloc, C);
      pao_integer_free(alloc, R);
      pao_natural_free(alloc, scratch);
      return false;
    }
    i--;
  }

  pao_Integer expected = pao_integer_new();
  s = pao_integer_set(alloc, 1, &expected); checkStatus(s);

  bool ok = pao_integer_equal(&A, &expected);
  pao_integer_free(alloc, A);
  pao_integer_free(alloc, B);
  pao_integer_free(alloc, C);
  pao_integer_free(alloc, R);
  pao_integer_free(alloc, expected);
  pao_natural_free(alloc, scratch);
  return ok && isAllFree();
}

bool test_integer_growShrink_2b(void) {
  pao_Status s;
  pao_Integer A = pao_integer_new();
  pao_Integer B = pao_integer_new();
  pao_Integer C = pao_integer_new();
  pao_Integer R = pao_integer_new();
  pao_Natural scratch = pao_natural_new();
  const int maxIter = 16;

  s = pao_integer_set(alloc, 1, &A); checkStatus(s);
  s = pao_integer_set(alloc, 8, &B); checkStatus(s);
  s = pao_integer_set(alloc, 1, &C); checkStatus(s);;
  s = pao_integer_set(alloc, 0, &R); checkStatus(s);;

  int i = 0;
  while (i < maxIter) {
    s = pao_integer_mult(alloc, &A, &B, &C); checkStatus(s);
    s = pao_integer_copy(alloc, &C, &A); checkStatus(s);
    i++;
  }

  while (0 < i) {
    s = pao_integer_div(alloc, &scratch, &A, &B, &C, &R); checkStatus(s);
    s = pao_integer_copy(alloc, &C, &A); checkStatus(s);
    if (!pao_integer_isZero(&R)) {
      pao_integer_free(alloc, A);
      pao_integer_free(alloc, B);
      pao_integer_free(alloc, C);
      pao_integer_free(alloc, R);
      pao_natural_free(alloc, scratch);
      return false;
    }
    i--;
  }

  pao_Integer expected = pao_integer_new();
  s = pao_integer_set(alloc, 1, &expected); checkStatus(s);

  bool ok = pao_integer_equal(&A, &expected);
  pao_integer_free(alloc, A);
  pao_integer_free(alloc, B);
  pao_integer_free(alloc, C);
  pao_integer_free(alloc, R);
  pao_integer_free(alloc, expected);
  pao_natural_free(alloc, scratch);
  return ok && isAllFree();
}


/* BEGIN: testing compare */
bool test_integer_compare_1a(void) {
  pao_Status s;
  pao_Integer a = pao_integer_new();
  pao_Integer b = pao_integer_new();
  pao_Order out;

  s = pao_integer_set(alloc, 42, &a); checkStatus(s);
  s = pao_integer_set(alloc, 42, &b); checkStatus(s);
  
  out = pao_integer_compare(&a, &b);

  pao_integer_free(alloc, a);
  pao_integer_free(alloc, b);

  return out == PAO_order_equal && isAllFree();
}

bool test_integer_compare_1b(void) {
  pao_Status s;
  pao_Integer a = pao_integer_new();
  pao_Integer b = pao_integer_new();
  pao_Order out;

  s = pao_integer_set(alloc, 42, &a); checkStatus(s);
  s = pao_integer_set(alloc, -42, &b); checkStatus(s);
  
  out = pao_integer_compare(&a, &b);

  pao_integer_free(alloc, a);
  pao_integer_free(alloc, b);

  return out == PAO_order_greater && isAllFree();
}

bool test_integer_compare_1c(void) {
  pao_Status s;
  pao_Integer a = pao_integer_new();
  pao_Integer b = pao_integer_new();
  pao_Order out;

  s = pao_integer_set(alloc, -42, &a); checkStatus(s);
  s = pao_integer_set(alloc, 42, &b); checkStatus(s);
  
  out = pao_integer_compare(&a, &b);

  pao_integer_free(alloc, a);
  pao_integer_free(alloc, b);

  return out == PAO_order_less && isAllFree();
}

bool test_integer_compare_1d(void) {
  pao_Status s;
  pao_Integer a = pao_integer_new();
  pao_Integer b = pao_integer_new();
  pao_Order out;

  s = pao_integer_set(alloc, -42, &a); checkStatus(s);
  s = pao_integer_set(alloc, -42, &b); checkStatus(s);
  
  out = pao_integer_compare(&a, &b);

  pao_integer_free(alloc, a);
  pao_integer_free(alloc, b);

  return out == PAO_order_equal && isAllFree();
}

bool test_integer_compare_2a(void) {
  pao_Status s;
  pao_Integer a = pao_integer_new();
  pao_Integer b = pao_integer_new();
  pao_Order out;

  s = pao_integer_set(alloc, 1, &a); checkStatus(s);
  s = pao_integer_set(alloc, 100, &b); checkStatus(s);
  
  out  = pao_integer_compare(&a, &b);

  pao_integer_free(alloc, a);
  pao_integer_free(alloc, b);

  return out == PAO_order_less && isAllFree();
}

bool test_integer_compare_2b(void) {
  pao_Status s;
  pao_Integer a = pao_integer_new();
  pao_Integer b = pao_integer_new();
  pao_Order out;

  s = pao_integer_set(alloc, -1, &a); checkStatus(s);
  s = pao_integer_set(alloc, 100, &b); checkStatus(s);
  
  out  = pao_integer_compare(&a, &b);

  pao_integer_free(alloc, a);
  pao_integer_free(alloc, b);

  return out == PAO_order_less && isAllFree();
}

bool test_integer_compare_2c(void) {
  pao_Status s;
  pao_Integer a = pao_integer_new();
  pao_Integer b = pao_integer_new();
  pao_Order out;

  s = pao_integer_set(alloc, 1, &a); checkStatus(s);
  s = pao_integer_set(alloc, -100, &b); checkStatus(s);
  
  out  = pao_integer_compare(&a, &b);

  pao_integer_free(alloc, a);
  pao_integer_free(alloc, b);

  return out == PAO_order_greater && isAllFree();
}

bool test_integer_compare_2d(void) {
  pao_Status s;
  pao_Integer a = pao_integer_new();
  pao_Integer b = pao_integer_new();
  pao_Order out;

  s = pao_integer_set(alloc, -1, &a); checkStatus(s);
  s = pao_integer_set(alloc, -100, &b); checkStatus(s);
  
  out  = pao_integer_compare(&a, &b);

  pao_integer_free(alloc, a);
  pao_integer_free(alloc, b);

  return out == PAO_order_greater && isAllFree();
}

bool test_integer_compare_3a(void) {
  pao_Status s;
  pao_Integer a = pao_integer_new();
  pao_Integer b = pao_integer_new();
  pao_Order out;

  u32 A_DIGS[] = {1, 0};
  s = pao_integer_setVec(alloc, +1, A_DIGS, 2, &a); checkStatus(s);
  s = pao_integer_set(alloc, PAO_natural_base - 1, &b); checkStatus(s);
  
  out = pao_integer_compare(&a, &b);

  pao_integer_free(alloc, a);
  pao_integer_free(alloc, b);

  return out == PAO_order_greater && isAllFree();
}

bool test_integer_compare_3b(void) {
  pao_Status s;
  pao_Integer a = pao_integer_new();
  pao_Integer b = pao_integer_new();
  pao_Order out;

  u32 A_DIGS[] = {1, 0};
  s = pao_integer_setVec(alloc, -1, A_DIGS, 2, &a); checkStatus(s);
  s = pao_integer_set(alloc, PAO_natural_base - 1, &b); checkStatus(s);
  
  out = pao_integer_compare(&a, &b);

  pao_integer_free(alloc, a);
  pao_integer_free(alloc, b);

  return out == PAO_order_less && isAllFree();
}

bool test_integer_compare_3c(void) {
  pao_Status s;
  pao_Integer a = pao_integer_new();
  pao_Integer b = pao_integer_new();
  pao_Order out;

  u32 A_DIGS[] = {1, 0};
  s = pao_integer_setVec(alloc, 1, A_DIGS, 2, &a); checkStatus(s);
  s = pao_integer_set(alloc, -(PAO_natural_base - 1), &b); checkStatus(s);
  
  out = pao_integer_compare(&a, &b);

  pao_integer_free(alloc, a);
  pao_integer_free(alloc, b);

  return out == PAO_order_greater && isAllFree();
}

bool test_integer_compare_3d(void) {
  pao_Status s;
  pao_Integer a = pao_integer_new();
  pao_Integer b = pao_integer_new();
  pao_Order out;

  u32 A_DIGS[] = {1, 0};
  s = pao_integer_setVec(alloc, -1, A_DIGS, 2, &a); checkStatus(s);
  s = pao_integer_set(alloc, -(PAO_natural_base - 1), &b); checkStatus(s);
  
  out = pao_integer_compare(&a, &b);

  pao_integer_free(alloc, a);
  pao_integer_free(alloc, b);

  return out == PAO_order_less && isAllFree();
}

bool test_integer_compare_4a(void) {
  pao_Status s;
  pao_Integer a = pao_integer_new();
  pao_Integer b = pao_integer_new();
  pao_Order out;

  u32 A_DIGS[] = {127, 0, 0, 1};
  s = pao_integer_setVec(alloc, +1, A_DIGS, 4, &a); checkStatus(s);
  u32 B_DIGS[] = {127, 0, 0, 1};
  s = pao_integer_setVec(alloc, +1, B_DIGS, 4, &b); checkStatus(s);
  
  out = pao_integer_compare(&a, &b);

  pao_integer_free(alloc, a);
  pao_integer_free(alloc, b);

  return out == PAO_order_equal && isAllFree();
}

bool test_integer_compare_4b(void) {
  pao_Status s;
  pao_Integer a = pao_integer_new();
  pao_Integer b = pao_integer_new();
  pao_Order out;

  u32 A_DIGS[] = {127, 0, 0, 1};
  s = pao_integer_setVec(alloc, -1, A_DIGS, 4, &a); checkStatus(s);
  u32 B_DIGS[] = {127, 0, 0, 1};
  s = pao_integer_setVec(alloc, +1, B_DIGS, 4, &b); checkStatus(s);
  
  out = pao_integer_compare(&a, &b);

  pao_integer_free(alloc, a);
  pao_integer_free(alloc, b);

  return out == PAO_order_less && isAllFree();
}

bool test_integer_compare_4c(void) {
  pao_Status s;
  pao_Integer a = pao_integer_new();
  pao_Integer b = pao_integer_new();
  pao_Order out;

  u32 A_DIGS[] = {127, 0, 0, 1};
  s = pao_integer_setVec(alloc, +1, A_DIGS, 4, &a); checkStatus(s);
  u32 B_DIGS[] = {127, 0, 0, 1};
  s = pao_integer_setVec(alloc, -1, B_DIGS, 4, &b); checkStatus(s);
  
  out = pao_integer_compare(&a, &b);

  pao_integer_free(alloc, a);
  pao_integer_free(alloc, b);

  return out == PAO_order_greater && isAllFree();
}

bool test_integer_compare_4d(void) {
  pao_Status s;
  pao_Integer a = pao_integer_new();
  pao_Integer b = pao_integer_new();
  pao_Order out;

  u32 A_DIGS[] = {127, 0, 0, 1};
  s = pao_integer_setVec(alloc, -1, A_DIGS, 4, &a); checkStatus(s);
  u32 B_DIGS[] = {127, 0, 0, 1};
  s = pao_integer_setVec(alloc, -1, B_DIGS, 4, &b); checkStatus(s);
  
  out = pao_integer_compare(&a, &b);

  pao_integer_free(alloc, a);
  pao_integer_free(alloc, b);

  return out == PAO_order_equal && isAllFree();
}
/* END: testing compare */

/* BEGIN: testing copy */
bool test_integer_copy_1(void) {
  pao_Integer a = pao_integer_new();
  pao_Integer out = pao_integer_new();
  pao_Status s;

  s = pao_integer_set(alloc, 2222, &a); checkStatus(s);
  s = pao_integer_copy(alloc, &a, &out); checkStatus(s);
  bool ok = pao_integer_equal(&out, &a) &&
            isValidInteger(&a);

  pao_integer_free(alloc, a);
  pao_integer_free(alloc, out);

  return ok && isAllFree();
}

bool test_integer_copy_2a(void) {
  pao_Status s;
  pao_Integer a = pao_integer_new();
  pao_Integer out = pao_integer_new();

  u32 A_DIGS[] = {999999999, 999999999};
  s = pao_integer_setVec(alloc, +1, A_DIGS, 2, &a); checkStatus(s);
  s = pao_integer_copy(alloc, &a, &out); checkStatus(s);

  bool ok = pao_integer_equal(&a, &out) &&
            isValidInteger(&out);
  pao_integer_free(alloc, a);
  pao_integer_free(alloc, out);

  return ok && isAllFree();
}

bool test_integer_copy_2b(void) {
  pao_Status s;
  pao_Integer a = pao_integer_new();
  pao_Integer out = pao_integer_new();

  u32 A_DIGS[] = {999999999, 999999999};
  s = pao_integer_setVec(alloc, -1, A_DIGS, 2, &a); checkStatus(s);
  s = pao_integer_copy(alloc, &a, &out); checkStatus(s);

  bool ok = pao_integer_equal(&a, &out) &&
            isValidInteger(&out);
  pao_integer_free(alloc, a);
  pao_integer_free(alloc, out);

  return ok && isAllFree();
}

// must work even if has garbage in the out param
bool test_integer_copy_3a(void) {
  pao_Status s;
  pao_Integer a = pao_integer_new();
  pao_Integer out = pao_integer_new();

  u32 A_DIGS[] = {999999999, 999999999};
  s = pao_integer_setVec(alloc, +1, A_DIGS, 2, &a); checkStatus(s);
  s = pao_integer_set(alloc, 123456, &out); checkStatus(s);

  s = pao_integer_copy(alloc, &a, &out); checkStatus(s);
  bool ok = pao_integer_equal(&a, &out) &&
            isValidInteger(&out);
  pao_integer_free(alloc, a);
  pao_integer_free(alloc, out);
  return ok && isAllFree();
}

bool test_integer_copy_3b(void) {
  pao_Status s;
  pao_Integer a = pao_integer_new();
  pao_Integer out = pao_integer_new();

  u32 A_DIGS[] = {999999999, 999999999};
  s = pao_integer_setVec(alloc, -1, A_DIGS, 2, &a); checkStatus(s);
  s = pao_integer_set(alloc, 123456, &out); checkStatus(s);

  s = pao_integer_copy(alloc, &a, &out); checkStatus(s);
  bool ok = pao_integer_equal(&a, &out) &&
            isValidInteger(&out);
  pao_integer_free(alloc, a);
  pao_integer_free(alloc, out);
  return ok && isAllFree();
}
/* END: testing copy */

/* BEGIN: DRIVER CODE */
Tester tests[] = {
  {"test_integer_snprint_0", test_integer_snprint_0},
  {"test_integer_snprint_1", test_integer_snprint_1},
  {"test_integer_snprint_2", test_integer_snprint_2},

  {"test_integer_compare_1a", test_integer_compare_1a},
  {"test_integer_compare_1b", test_integer_compare_1b},
  {"test_integer_compare_1c", test_integer_compare_1c},
  {"test_integer_compare_1d", test_integer_compare_1d},
  {"test_integer_compare_2a", test_integer_compare_2a},
  {"test_integer_compare_2b", test_integer_compare_2b},
  {"test_integer_compare_2c", test_integer_compare_2c},
  {"test_integer_compare_2d", test_integer_compare_2d},
  {"test_integer_compare_3a", test_integer_compare_3a},
  {"test_integer_compare_3b", test_integer_compare_3b},
  {"test_integer_compare_3c", test_integer_compare_3c},
  {"test_integer_compare_3d", test_integer_compare_3d},
  {"test_integer_compare_4a", test_integer_compare_4a},
  {"test_integer_compare_4b", test_integer_compare_4b},
  {"test_integer_compare_4c", test_integer_compare_4c},
  {"test_integer_compare_4d", test_integer_compare_4d},

  {"test_integer_copy_1", test_integer_copy_1},
  {"test_integer_copy_2a", test_integer_copy_2a},
  {"test_integer_copy_2b", test_integer_copy_2b},
  {"test_integer_copy_3a", test_integer_copy_3a},
  {"test_integer_copy_3b", test_integer_copy_3b},

  {"test_integer_growShrink_1a", test_integer_growShrink_1a},
  {"test_integer_growShrink_1b", test_integer_growShrink_1b},
  {"test_integer_growShrink_2a", test_integer_growShrink_2a},
  {"test_integer_growShrink_2b", test_integer_growShrink_2b},
};
#define TEST_LEN (int)(sizeof(tests) / sizeof(tests[0]))

int main(void) {
  _alloc = pao_stdAlloc_new();
  run_tests(tests, TEST_LEN);
}
/* END: DRIVER CODE */
