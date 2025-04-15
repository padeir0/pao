#include "../src/pao_natural.h"
#include "../src/pao_basicTypes.h"
#include "../src/pao_status.h"
#include "../src/pao_stdAlloc.h"
#include <stdio.h>
#include <strings.h>

void check_status(pao_status ns) {
  if (ns != PAO_status_ok) {
    printf("fail: %d\n", ns);
    abort();
  }
}

#define BUFF_LENGTH 2048
char buffer[BUFF_LENGTH];

void print_nat(pao_Natural n) {
  usize written = pao_natural_snprint(n, buffer, BUFF_LENGTH);
  if (written == 0) {
    printf("nothing printed :(\n");
    abort();
  }
  printf("%.*s", (int)written, buffer);
  printf(" (length: %d, cap: %d)\n", n.len, n.cap);
}

// tests carry
bool test_natural_add_digit_1(void) {
  pao_Natural a = pao_natural_empty();
  pao_Natural out = pao_natural_empty();
  pao_Natural expected = pao_natural_empty();

  u32 A_DIGS[] = {999999999, 999999999};
  #define A_DIGS_LEN (sizeof(A_DIGS) / sizeof(A_DIGS[0]))

  pao_natural_setVec(PAO_STDMALLOC, &a, A_DIGS, A_DIGS_LEN);
  
  u32 EXP_DIGS[A_DIGS_LEN+1] = {1, 0, 0};
  pao_natural_setVec(PAO_STDMALLOC, &expected, EXP_DIGS, A_DIGS_LEN+1);

  pao_status s = pao_natural_addDigit(PAO_STDMALLOC,a, 1, &out);
  check_status(s);
  print_nat(a);
  print_nat(out);
  print_nat(expected);

  return pao_natural_equal(out, expected);
}

// tests 0 as identity
bool test_natural_add_digit_2(void) {
  pao_Natural a = pao_natural_empty();
  pao_Natural out = pao_natural_empty();
  pao_Natural expected = pao_natural_empty();

  pao_natural_set(PAO_STDMALLOC, &a, 0);
  pao_natural_set(PAO_STDMALLOC, &expected, 42);

  pao_status s = pao_natural_addDigit(PAO_STDMALLOC, a, 42, &out);
  check_status(s);

  return pao_natural_equal(out, expected);
}

// tests 0 as identity
bool test_natural_add_digit_3(void) {
  pao_Natural a = pao_natural_empty();
  pao_Natural out = pao_natural_empty();
  pao_Natural expected = pao_natural_empty();

  pao_natural_set(PAO_STDMALLOC, &a, 314159);
  pao_natural_set(PAO_STDMALLOC, &expected, 314159);

  pao_status s = pao_natural_addDigit(PAO_STDMALLOC, a, 0, &out);
  check_status(s);

  return pao_natural_equal(out, expected);
}

// ensures operands are not modified
bool test_natural_add_digit_4(void) {
  pao_Natural a = pao_natural_empty();
  pao_Natural out = pao_natural_empty();
  pao_Natural expected = pao_natural_empty();

  pao_natural_set(PAO_STDMALLOC, &a, 314159);
  pao_natural_set(PAO_STDMALLOC, &expected, 314160);

  pao_status s = pao_natural_addDigit(PAO_STDMALLOC, a, 1, &out);
  check_status(s);

  return pao_natural_equalDigit(a, 314159);
}

char test_buffer[BUFF_LENGTH];

bool test_natural_snprint_1(void) {
  pao_Natural A = pao_natural_empty();

  u32 digits[] = {
    1,0,0
  };
  #define DILEN (sizeof(digits) / sizeof(digits[0]))

  pao_natural_setVec(PAO_STDMALLOC, &A, digits, DILEN);

  usize written = pao_natural_snprint(A, test_buffer, BUFF_LENGTH);
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

  pao_natural_set(PAO_STDMALLOC, &A, 314159);

  usize written = pao_natural_snprint(A, test_buffer, BUFF_LENGTH);
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

  pao_natural_set(PAO_STDMALLOC, &A, 0);

  usize written = pao_natural_snprint(A, test_buffer, BUFF_LENGTH);
  if (written == 0) {
    return false;
  }

  if (strncmp("0", test_buffer, written) != 0) {
    return false;
  }
  return true;
}

typedef struct {
  char* name;
  bool (*func)(void);
} Tester;

char print_buff[BUFF_LENGTH];
void test(Tester t) {
  bzero(print_buff, BUFF_LENGTH);
  if (t.func()) {
    strcat(print_buff, "OK: ");
  } else {
    strcat(print_buff, "FAIL: ");
  }
  strcat(print_buff, t.name);
  strcat(print_buff, "\n");
  printf("%s", print_buff);
}

Tester tests[] = {
  {"test_natural_snprint_1", test_natural_snprint_1},
  {"test_natural_snprint_2", test_natural_snprint_2},
  {"test_natural_snprint_3", test_natural_snprint_3},
  {"test_natural_add_digit_1", test_natural_add_digit_1},
  {"test_natural_add_digit_2", test_natural_add_digit_2},
  {"test_natural_add_digit_3", test_natural_add_digit_3},
  {"test_natural_add_digit_4", test_natural_add_digit_4},
};
#define TEST_LEN (u32)(sizeof(tests) / sizeof(tests[0]))

int main(void) {
  u32 i = 0;
  while (i < TEST_LEN) {
    test(tests[i]);
    i++;
  }
}
