// #include "../src/pao_natural.h"
#include "../lib/pao_integer.h"
#include "../lib/pao_basicTypes.h"
#include "../lib/pao_status.h"
#include "../lib/pao_stdAlloc.h"
#include <stdio.h>
#include <strings.h>
#include "common.h"

// TODO: write sign tests

char buffer[DEFAULT_SIZE];

pao_Allocator _alloc;
#define alloc (&_alloc)

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
  _alloc = pao_stdAlloc_new();
  run_tests(tests, TEST_LEN);
}
/* END: DRIVER CODE */
