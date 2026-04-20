// #include "../src/mb_natural.h"
#include "../src/mb_integer.h"
#include "../src/mb_basicTypes.h"
#include "../src/mb_status.h"
#include "../src/mb_stdAlloc.h"
#include <stdio.h>
#include <strings.h>
#include "common.h"

// TODO: write sign tests

char buffer[DEFAULT_SIZE];

#define alloc MB_stdAlloc

bool isAllFree(void) {
  return alloc->info(alloc->heap).used == 0;
}

void printInt(mb_Integer* i) {
  usize written = mb_integer_snprint(i, buffer, DEFAULT_SIZE);
  if (written == 0) {
    printf("written 0 bytes.\n");
  }
  printf("%.*s", (int)written, buffer);
}

char test_buffer[DEFAULT_SIZE];

bool test_integer_snprint_0(void) {
  mb_Status s;
  mb_Integer A = mb_integer_new();
  s = mb_integer_set(alloc, 1, &A); checkStatus(s);

  usize written = mb_integer_snprint(&A, test_buffer, DEFAULT_SIZE);
  if (written == 0) {
    mb_integer_free(alloc, A);
    return false;
  }

  if (strncmp("1", test_buffer, written) != 0) {
    mb_integer_free(alloc, A);
    return false;
  }
  mb_integer_free(alloc, A);
  return true;
}

bool test_integer_snprint_1(void) {
  mb_Status s;
  mb_Integer A = mb_integer_new();
  s = mb_integer_set(alloc, -1, &A); checkStatus(s);

  usize written = mb_integer_snprint(&A, test_buffer, DEFAULT_SIZE);
  if (written == 0) {
    mb_integer_free(alloc, A);
    return false;
  }

  if (strncmp("-1", test_buffer, written) != 0) {
    mb_integer_free(alloc, A);
    return false;
  }
  mb_integer_free(alloc, A);
  return true;
}

bool test_integer_snprint_2(void) {
  mb_Status s;
  mb_Integer A = mb_integer_new();

  u32 A_digs[] = {1, 111222333};
  s = mb_integer_setVec(alloc, -1, A_digs, 2, &A); checkStatus(s);

  usize written = mb_integer_snprint(&A, test_buffer, DEFAULT_SIZE);
  if (written == 0) {
    mb_integer_free(alloc, A);
    return false;
  }

  if (strncmp("-1111222333", test_buffer, written) != 0) {
    mb_integer_free(alloc, A);
    return false;
  }
  mb_integer_free(alloc, A);
  return true;
}

bool test_integer_growShrink_1a(void) {
  mb_Status s;
  mb_Integer A = mb_integer_new();
  mb_Integer C = mb_integer_new();

  s = mb_integer_set(alloc, 7, &A); checkStatus(s);
  s = mb_integer_set(alloc, 0, &C); checkStatus(s);;

  int i = 0;
  while (i < 100) {
    s = mb_integer_add(alloc, &C, &A, &C); checkStatus(s);
    i++;
  }

  while (0 < i) {
    s = mb_integer_sub(alloc, &C, &A, &C); checkStatus(s);
    i--;
  }
  
  bool ok = mb_integer_isZero(&C);
  mb_integer_free(alloc, A);
  mb_integer_free(alloc, C);
  return ok && isAllFree();
}

bool test_integer_growShrink_1b(void) {
  mb_Status s;
  mb_Integer A = mb_integer_new();
  mb_Integer C = mb_integer_new();

  s = mb_integer_set(alloc, -7, &A); checkStatus(s);
  s = mb_integer_set(alloc, 0, &C); checkStatus(s);;

  int i = 0;
  while (i < 100) {
    s = mb_integer_add(alloc, &C, &A, &C); checkStatus(s);
    i++;
  }

  while (0 < i) {
    s = mb_integer_sub(alloc, &C, &A, &C); checkStatus(s);
    i--;
  }
  
  bool ok = mb_integer_isZero(&C);
  mb_integer_free(alloc, A);
  mb_integer_free(alloc, C);
  return ok && isAllFree();
}

// mult/div
bool test_integer_growShrink_2a(void) {
  mb_Status s;
  mb_Integer A = mb_integer_new();
  mb_Integer B = mb_integer_new();
  mb_Integer C = mb_integer_new();
  mb_Integer R = mb_integer_new();
  mb_Natural scratch = mb_natural_new();
  const int maxIter = 16;

  s = mb_integer_set(alloc, 1, &A); checkStatus(s);
  s = mb_integer_set(alloc, 8, &B); checkStatus(s);
  s = mb_integer_set(alloc, 1, &C); checkStatus(s);;
  s = mb_integer_set(alloc, 0, &R); checkStatus(s);;

  int i = 0;
  while (i < maxIter) {
    s = mb_integer_mult(alloc, &A, &B, &C); checkStatus(s);
    s = mb_integer_copy(alloc, &C, &A); checkStatus(s);
    i++;
  }

  while (0 < i) {
    s = mb_integer_div(alloc, &scratch, &A, &B, &C, &R); checkStatus(s);
    s = mb_integer_copy(alloc, &C, &A); checkStatus(s);
    if (!mb_integer_isZero(&R)) {
      mb_integer_free(alloc, A);
      mb_integer_free(alloc, B);
      mb_integer_free(alloc, C);
      mb_integer_free(alloc, R);
      mb_natural_free(alloc, scratch);
      return false;
    }
    i--;
  }

  mb_Integer expected = mb_integer_new();
  s = mb_integer_set(alloc, 1, &expected); checkStatus(s);

  bool ok = mb_integer_equal(&A, &expected);
  mb_integer_free(alloc, A);
  mb_integer_free(alloc, B);
  mb_integer_free(alloc, C);
  mb_integer_free(alloc, R);
  mb_integer_free(alloc, expected);
  mb_natural_free(alloc, scratch);
  return ok && isAllFree();
}

bool test_integer_growShrink_2b(void) {
  mb_Status s;
  mb_Integer A = mb_integer_new();
  mb_Integer B = mb_integer_new();
  mb_Integer C = mb_integer_new();
  mb_Integer R = mb_integer_new();
  mb_Natural scratch = mb_natural_new();
  const int maxIter = 16;

  s = mb_integer_set(alloc, 1, &A); checkStatus(s);
  s = mb_integer_set(alloc, 8, &B); checkStatus(s);
  s = mb_integer_set(alloc, 1, &C); checkStatus(s);;
  s = mb_integer_set(alloc, 0, &R); checkStatus(s);;

  int i = 0;
  while (i < maxIter) {
    s = mb_integer_mult(alloc, &A, &B, &C); checkStatus(s);
    s = mb_integer_copy(alloc, &C, &A); checkStatus(s);
    i++;
  }

  while (0 < i) {
    s = mb_integer_div(alloc, &scratch, &A, &B, &C, &R); checkStatus(s);
    s = mb_integer_copy(alloc, &C, &A); checkStatus(s);
    if (!mb_integer_isZero(&R)) {
      mb_integer_free(alloc, A);
      mb_integer_free(alloc, B);
      mb_integer_free(alloc, C);
      mb_integer_free(alloc, R);
      mb_natural_free(alloc, scratch);
      return false;
    }
    i--;
  }

  mb_Integer expected = mb_integer_new();
  s = mb_integer_set(alloc, 1, &expected); checkStatus(s);

  bool ok = mb_integer_equal(&A, &expected);
  mb_integer_free(alloc, A);
  mb_integer_free(alloc, B);
  mb_integer_free(alloc, C);
  mb_integer_free(alloc, R);
  mb_integer_free(alloc, expected);
  mb_natural_free(alloc, scratch);
  return ok && isAllFree();
}

/* BEGIN: DRIVER CODE */
Tester tests[] = {
  {"test_integer_snprint_0", test_integer_snprint_0},
  {"test_integer_snprint_1", test_integer_snprint_1},
  {"test_integer_snprint_2", test_integer_snprint_2},

  {"test_integer_growShrink_1a", test_integer_growShrink_1a},
  {"test_integer_growShrink_1b", test_integer_growShrink_1b},
  {"test_integer_growShrink_2a", test_integer_growShrink_2a},
  {"test_integer_growShrink_2b", test_integer_growShrink_2b},
};
#define TEST_LEN (int)(sizeof(tests) / sizeof(tests[0]))

int main(void) {
  run_tests(tests, TEST_LEN);
}
/* END: DRIVER CODE */
