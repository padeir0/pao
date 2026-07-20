// #include "../src/natural.h"
#include "../../lib/numbers/integer.h"
#include "../../lib/basicTypes.h"
#include "../../lib/status.h"
#include "../../lib/alloc/stdAlloc.h"
#include <stdio.h>
#include <strings.h>
#include "../common.h"

char buffer[DEFAULT_SIZE];

IAllocator _alloc;
#define alloc (&_alloc)

bool isValidInteger(Integer* n) {
  if (n->abs.len == 0) {
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
    if (n->abs.digits[i] >= natural_BASE) {
      return false;
    }
    i++;
  }
  return true;
}

bool isAllFree(void) {
  return alloc->info(alloc->heap).used == 0;
}

void printInt(Integer* i) {
  usize written = integer_snprint(i, buffer, DEFAULT_SIZE);
  if (written == 0) {
    printf("written 0 bytes.\n");
  }
  printf("%.*s", (int)written, buffer);
}

char test_buffer[DEFAULT_SIZE];

bool test_integer_snprint_0(void) {
  Status s;
  Integer A = integer_new();
  s = integer_set(alloc, 1, &A); checkStatus(s);

  usize written = integer_snprint(&A, test_buffer, DEFAULT_SIZE);
  if (written == 0) {
    integer_free(alloc, A);
    return false;
  }

  if (strncmp("1", test_buffer, written) != 0) {
    integer_free(alloc, A);
    return false;
  }
  integer_free(alloc, A);
  return true;
}

bool test_integer_snprint_1(void) {
  Status s;
  Integer A = integer_new();
  s = integer_set(alloc, -1, &A); checkStatus(s);

  usize written = integer_snprint(&A, test_buffer, DEFAULT_SIZE);
  if (written == 0) {
    integer_free(alloc, A);
    return false;
  }

  if (strncmp("-1", test_buffer, written) != 0) {
    integer_free(alloc, A);
    return false;
  }
  integer_free(alloc, A);
  return true;
}

bool test_integer_snprint_2(void) {
  Status s;
  Integer A = integer_new();

  u32 A_digs[] = {1, 111222333};
  s = integer_setVec(alloc, -1, A_digs, 2, &A); checkStatus(s);

  usize written = integer_snprint(&A, test_buffer, DEFAULT_SIZE);
  if (written == 0) {
    integer_free(alloc, A);
    return false;
  }

  if (strncmp("-1111222333", test_buffer, written) != 0) {
    integer_free(alloc, A);
    return false;
  }
  integer_free(alloc, A);
  return true;
}

bool test_integer_growShrink_1a(void) {
  Status s;
  Integer A = integer_new();
  Integer C = integer_new();

  s = integer_set(alloc, 7, &A); checkStatus(s);
  s = integer_set(alloc, 0, &C); checkStatus(s);;

  int i = 0;
  while (i < 100) {
    s = integer_add(alloc, &C, &A, &C); checkStatus(s);
    i++;
  }

  while (0 < i) {
    s = integer_sub(alloc, &C, &A, &C); checkStatus(s);
    i--;
  }
  
  bool ok = integer_isZero(&C);
  integer_free(alloc, A);
  integer_free(alloc, C);
  return ok && isAllFree();
}

bool test_integer_growShrink_1b(void) {
  Status s;
  Integer A = integer_new();
  Integer C = integer_new();

  s = integer_set(alloc, -7, &A); checkStatus(s);
  s = integer_set(alloc, 0, &C); checkStatus(s);;

  int i = 0;
  while (i < 100) {
    s = integer_add(alloc, &C, &A, &C); checkStatus(s);
    i++;
  }

  while (0 < i) {
    s = integer_sub(alloc, &C, &A, &C); checkStatus(s);
    i--;
  }
  
  bool ok = integer_isZero(&C);
  integer_free(alloc, A);
  integer_free(alloc, C);
  return ok && isAllFree();
}

// mult/div
bool test_integer_growShrink_2a(void) {
  Status s;
  Integer A = integer_new();
  Integer B = integer_new();
  Integer C = integer_new();
  Integer R = integer_new();
  Natural scratch = natural_new();
  const int maxIter = 16;

  s = integer_set(alloc, 1, &A); checkStatus(s);
  s = integer_set(alloc, 8, &B); checkStatus(s);
  s = integer_set(alloc, 1, &C); checkStatus(s);;
  s = integer_set(alloc, 0, &R); checkStatus(s);;

  int i = 0;
  while (i < maxIter) {
    s = integer_mult(alloc, &A, &B, &C); checkStatus(s);
    s = integer_copy(alloc, &C, &A); checkStatus(s);
    i++;
  }

  while (0 < i) {
    s = integer_div(alloc, &scratch, &A, &B, &C, &R); checkStatus(s);
    s = integer_copy(alloc, &C, &A); checkStatus(s);
    if (!integer_isZero(&R)) {
      integer_free(alloc, A);
      integer_free(alloc, B);
      integer_free(alloc, C);
      integer_free(alloc, R);
      natural_free(alloc, scratch);
      return false;
    }
    i--;
  }

  Integer expected = integer_new();
  s = integer_set(alloc, 1, &expected); checkStatus(s);

  bool ok = integer_equal(&A, &expected);
  integer_free(alloc, A);
  integer_free(alloc, B);
  integer_free(alloc, C);
  integer_free(alloc, R);
  integer_free(alloc, expected);
  natural_free(alloc, scratch);
  return ok && isAllFree();
}

bool test_integer_growShrink_2b(void) {
  Status s;
  Integer A = integer_new();
  Integer B = integer_new();
  Integer C = integer_new();
  Integer R = integer_new();
  Natural scratch = natural_new();
  const int maxIter = 16;

  s = integer_set(alloc, 1, &A); checkStatus(s);
  s = integer_set(alloc, 8, &B); checkStatus(s);
  s = integer_set(alloc, 1, &C); checkStatus(s);;
  s = integer_set(alloc, 0, &R); checkStatus(s);;

  int i = 0;
  while (i < maxIter) {
    s = integer_mult(alloc, &A, &B, &C); checkStatus(s);
    s = integer_copy(alloc, &C, &A); checkStatus(s);
    i++;
  }

  while (0 < i) {
    s = integer_div(alloc, &scratch, &A, &B, &C, &R); checkStatus(s);
    s = integer_copy(alloc, &C, &A); checkStatus(s);
    if (!integer_isZero(&R)) {
      integer_free(alloc, A);
      integer_free(alloc, B);
      integer_free(alloc, C);
      integer_free(alloc, R);
      natural_free(alloc, scratch);
      return false;
    }
    i--;
  }

  Integer expected = integer_new();
  s = integer_set(alloc, 1, &expected); checkStatus(s);

  bool ok = integer_equal(&A, &expected);
  integer_free(alloc, A);
  integer_free(alloc, B);
  integer_free(alloc, C);
  integer_free(alloc, R);
  integer_free(alloc, expected);
  natural_free(alloc, scratch);
  return ok && isAllFree();
}


/* BEGIN: testing compare */
bool test_integer_compare_1a(void) {
  Status s;
  Integer a = integer_new();
  Integer b = integer_new();
  Order out;

  s = integer_set(alloc, 42, &a); checkStatus(s);
  s = integer_set(alloc, 42, &b); checkStatus(s);
  
  out = integer_compare(&a, &b);

  integer_free(alloc, a);
  integer_free(alloc, b);

  return out == order_EQUAL && isAllFree();
}

bool test_integer_compare_1b(void) {
  Status s;
  Integer a = integer_new();
  Integer b = integer_new();
  Order out;

  s = integer_set(alloc, 42, &a); checkStatus(s);
  s = integer_set(alloc, -42, &b); checkStatus(s);
  
  out = integer_compare(&a, &b);

  integer_free(alloc, a);
  integer_free(alloc, b);

  return out == order_GREATER && isAllFree();
}

bool test_integer_compare_1c(void) {
  Status s;
  Integer a = integer_new();
  Integer b = integer_new();
  Order out;

  s = integer_set(alloc, -42, &a); checkStatus(s);
  s = integer_set(alloc, 42, &b); checkStatus(s);
  
  out = integer_compare(&a, &b);

  integer_free(alloc, a);
  integer_free(alloc, b);

  return out == order_LESS && isAllFree();
}

bool test_integer_compare_1d(void) {
  Status s;
  Integer a = integer_new();
  Integer b = integer_new();
  Order out;

  s = integer_set(alloc, -42, &a); checkStatus(s);
  s = integer_set(alloc, -42, &b); checkStatus(s);
  
  out = integer_compare(&a, &b);

  integer_free(alloc, a);
  integer_free(alloc, b);

  return out == order_EQUAL && isAllFree();
}

bool test_integer_compare_2a(void) {
  Status s;
  Integer a = integer_new();
  Integer b = integer_new();
  Order out;

  s = integer_set(alloc, 1, &a); checkStatus(s);
  s = integer_set(alloc, 100, &b); checkStatus(s);
  
  out  = integer_compare(&a, &b);

  integer_free(alloc, a);
  integer_free(alloc, b);

  return out == order_LESS && isAllFree();
}

bool test_integer_compare_2b(void) {
  Status s;
  Integer a = integer_new();
  Integer b = integer_new();
  Order out;

  s = integer_set(alloc, -1, &a); checkStatus(s);
  s = integer_set(alloc, 100, &b); checkStatus(s);
  
  out  = integer_compare(&a, &b);

  integer_free(alloc, a);
  integer_free(alloc, b);

  return out == order_LESS && isAllFree();
}

bool test_integer_compare_2c(void) {
  Status s;
  Integer a = integer_new();
  Integer b = integer_new();
  Order out;

  s = integer_set(alloc, 1, &a); checkStatus(s);
  s = integer_set(alloc, -100, &b); checkStatus(s);
  
  out  = integer_compare(&a, &b);

  integer_free(alloc, a);
  integer_free(alloc, b);

  return out == order_GREATER && isAllFree();
}

bool test_integer_compare_2d(void) {
  Status s;
  Integer a = integer_new();
  Integer b = integer_new();
  Order out;

  s = integer_set(alloc, -1, &a); checkStatus(s);
  s = integer_set(alloc, -100, &b); checkStatus(s);
  
  out  = integer_compare(&a, &b);

  integer_free(alloc, a);
  integer_free(alloc, b);

  return out == order_GREATER && isAllFree();
}

bool test_integer_compare_3a(void) {
  Status s;
  Integer a = integer_new();
  Integer b = integer_new();
  Order out;

  u32 A_DIGS[] = {1, 0};
  s = integer_setVec(alloc, +1, A_DIGS, 2, &a); checkStatus(s);
  s = integer_set(alloc, natural_BASE - 1, &b); checkStatus(s);
  
  out = integer_compare(&a, &b);

  integer_free(alloc, a);
  integer_free(alloc, b);

  return out == order_GREATER && isAllFree();
}

bool test_integer_compare_3b(void) {
  Status s;
  Integer a = integer_new();
  Integer b = integer_new();
  Order out;

  u32 A_DIGS[] = {1, 0};
  s = integer_setVec(alloc, -1, A_DIGS, 2, &a); checkStatus(s);
  s = integer_set(alloc, natural_BASE - 1, &b); checkStatus(s);
  
  out = integer_compare(&a, &b);

  integer_free(alloc, a);
  integer_free(alloc, b);

  return out == order_LESS && isAllFree();
}

bool test_integer_compare_3c(void) {
  Status s;
  Integer a = integer_new();
  Integer b = integer_new();
  Order out;

  u32 A_DIGS[] = {1, 0};
  s = integer_setVec(alloc, 1, A_DIGS, 2, &a); checkStatus(s);
  s = integer_set(alloc, -(natural_BASE - 1), &b); checkStatus(s);
  
  out = integer_compare(&a, &b);

  integer_free(alloc, a);
  integer_free(alloc, b);

  return out == order_GREATER && isAllFree();
}

bool test_integer_compare_3d(void) {
  Status s;
  Integer a = integer_new();
  Integer b = integer_new();
  Order out;

  u32 A_DIGS[] = {1, 0};
  s = integer_setVec(alloc, -1, A_DIGS, 2, &a); checkStatus(s);
  s = integer_set(alloc, -(natural_BASE - 1), &b); checkStatus(s);
  
  out = integer_compare(&a, &b);

  integer_free(alloc, a);
  integer_free(alloc, b);

  return out == order_LESS && isAllFree();
}

bool test_integer_compare_4a(void) {
  Status s;
  Integer a = integer_new();
  Integer b = integer_new();
  Order out;

  u32 A_DIGS[] = {127, 0, 0, 1};
  s = integer_setVec(alloc, +1, A_DIGS, 4, &a); checkStatus(s);
  u32 B_DIGS[] = {127, 0, 0, 1};
  s = integer_setVec(alloc, +1, B_DIGS, 4, &b); checkStatus(s);
  
  out = integer_compare(&a, &b);

  integer_free(alloc, a);
  integer_free(alloc, b);

  return out == order_EQUAL && isAllFree();
}

bool test_integer_compare_4b(void) {
  Status s;
  Integer a = integer_new();
  Integer b = integer_new();
  Order out;

  u32 A_DIGS[] = {127, 0, 0, 1};
  s = integer_setVec(alloc, -1, A_DIGS, 4, &a); checkStatus(s);
  u32 B_DIGS[] = {127, 0, 0, 1};
  s = integer_setVec(alloc, +1, B_DIGS, 4, &b); checkStatus(s);
  
  out = integer_compare(&a, &b);

  integer_free(alloc, a);
  integer_free(alloc, b);

  return out == order_LESS && isAllFree();
}

bool test_integer_compare_4c(void) {
  Status s;
  Integer a = integer_new();
  Integer b = integer_new();
  Order out;

  u32 A_DIGS[] = {127, 0, 0, 1};
  s = integer_setVec(alloc, +1, A_DIGS, 4, &a); checkStatus(s);
  u32 B_DIGS[] = {127, 0, 0, 1};
  s = integer_setVec(alloc, -1, B_DIGS, 4, &b); checkStatus(s);
  
  out = integer_compare(&a, &b);

  integer_free(alloc, a);
  integer_free(alloc, b);

  return out == order_GREATER && isAllFree();
}

bool test_integer_compare_4d(void) {
  Status s;
  Integer a = integer_new();
  Integer b = integer_new();
  Order out;

  u32 A_DIGS[] = {127, 0, 0, 1};
  s = integer_setVec(alloc, -1, A_DIGS, 4, &a); checkStatus(s);
  u32 B_DIGS[] = {127, 0, 0, 1};
  s = integer_setVec(alloc, -1, B_DIGS, 4, &b); checkStatus(s);
  
  out = integer_compare(&a, &b);

  integer_free(alloc, a);
  integer_free(alloc, b);

  return out == order_EQUAL && isAllFree();
}
/* END: testing compare */

/* BEGIN: testing copy */
bool test_integer_copy_1(void) {
  Integer a = integer_new();
  Integer out = integer_new();
  Status s;

  s = integer_set(alloc, 2222, &a); checkStatus(s);
  s = integer_copy(alloc, &a, &out); checkStatus(s);
  bool ok = integer_equal(&out, &a) &&
            isValidInteger(&a);

  integer_free(alloc, a);
  integer_free(alloc, out);

  return ok && isAllFree();
}

bool test_integer_copy_2a(void) {
  Status s;
  Integer a = integer_new();
  Integer out = integer_new();

  u32 A_DIGS[] = {999999999, 999999999};
  s = integer_setVec(alloc, +1, A_DIGS, 2, &a); checkStatus(s);
  s = integer_copy(alloc, &a, &out); checkStatus(s);

  bool ok = integer_equal(&a, &out) &&
            isValidInteger(&out);
  integer_free(alloc, a);
  integer_free(alloc, out);

  return ok && isAllFree();
}

bool test_integer_copy_2b(void) {
  Status s;
  Integer a = integer_new();
  Integer out = integer_new();

  u32 A_DIGS[] = {999999999, 999999999};
  s = integer_setVec(alloc, -1, A_DIGS, 2, &a); checkStatus(s);
  s = integer_copy(alloc, &a, &out); checkStatus(s);

  bool ok = integer_equal(&a, &out) &&
            isValidInteger(&out);
  integer_free(alloc, a);
  integer_free(alloc, out);

  return ok && isAllFree();
}

// must work even if has garbage in the out param
bool test_integer_copy_3a(void) {
  Status s;
  Integer a = integer_new();
  Integer out = integer_new();

  u32 A_DIGS[] = {999999999, 999999999};
  s = integer_setVec(alloc, +1, A_DIGS, 2, &a); checkStatus(s);
  s = integer_set(alloc, 123456, &out); checkStatus(s);

  s = integer_copy(alloc, &a, &out); checkStatus(s);
  bool ok = integer_equal(&a, &out) &&
            isValidInteger(&out);
  integer_free(alloc, a);
  integer_free(alloc, out);
  return ok && isAllFree();
}

bool test_integer_copy_3b(void) {
  Status s;
  Integer a = integer_new();
  Integer out = integer_new();

  u32 A_DIGS[] = {999999999, 999999999};
  s = integer_setVec(alloc, -1, A_DIGS, 2, &a); checkStatus(s);
  s = integer_set(alloc, 123456, &out); checkStatus(s);

  s = integer_copy(alloc, &a, &out); checkStatus(s);
  bool ok = integer_equal(&a, &out) &&
            isValidInteger(&out);
  integer_free(alloc, a);
  integer_free(alloc, out);
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
  _alloc = stdAlloc_new();
  run_tests("integer", tests, TEST_LEN);
}
/* END: DRIVER CODE */
