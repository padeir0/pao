#include "../src/pao_natural.h"
#include "../src/pao_basic_types.h"
#include "../src/pao_status.h"
#include "../src/pao_stdmalloc.h"
#include <stdio.h>
#include <strings.h>

void check_status(pao_status ns) {
  if (ns != pao_status_OK) {
    printf("fail: %d\n", ns);
    abort();
  }
}

#define BUFF_LENGTH 2048
char buffer[BUFF_LENGTH];

void print_nat(pao_Natural n) {
  int written = pao_natural_snprint(n, buffer, BUFF_LENGTH);
  if (written == 0) {
    printf("nothing printed :(");
    abort();
  }
  printf("%.*s", written, buffer);
  printf(" (length: %d, cap: %d)\n", n.len, n.cap);
}

bool test_natural_add_digit_1() {
  pao_Natural a = pao_natural_empty();
  pao_Natural out = pao_natural_empty();
  pao_Natural expected = pao_natural_empty();

  #define A_DIGS_LEN 2
  u32 A_DIGS[A_DIGS_LEN] = {999999999, 999999999};
  pao_natural_set_vec(PAO_STDMALLOC, &a, A_DIGS, A_DIGS_LEN);
  
  u32 EXP_DIGS[A_DIGS_LEN+1] = {1, 0, 0};
  pao_natural_set_vec(PAO_STDMALLOC,&expected, EXP_DIGS, A_DIGS_LEN+1);

  pao_status s = pao_natural_add_digit(PAO_STDMALLOC,a, 1, &out);
  check_status(s);

  return pao_natural_equal(out, expected);
}

char test_buffer[BUFF_LENGTH];

bool test_natural_snprint_1() {
  pao_Natural A = pao_natural_empty();

  #define DILEN 3
  u32 digits[DILEN] = {
    1,0,0
  };
  pao_natural_set_vec(PAO_STDMALLOC, &A, digits, DILEN);

  usize written = pao_natural_snprint(A, test_buffer, BUFF_LENGTH);
  if (written == 0) {
    return false;
  }

  if (strncmp("1000000000000000000", test_buffer, written) != 0) {
    return false;
  }
  return true;
}

typedef struct {
  char* name;
  bool (*func)();
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

#define TEST_LEN 2
Tester tests[TEST_LEN] = {
  {"test_natural_snprint_1", test_natural_snprint_1},
  {"test_natural_add_digit_1", test_natural_add_digit_1},
};

int main() {
  int i = 0;
  while (i < TEST_LEN) {
    test(tests[i]);
    i++;
  }
}
