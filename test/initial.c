#include "../src/pao_natural.h"
#include "../src/pao_status.h"
#include "../src/pao_stdmalloc.h"
#include <stdio.h>

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

int pi() {
  pao_Natural A = pao_natural_empty();
  u32 B = 2;
  pao_Natural OUT = pao_natural_empty();

  #define PILEN 5
  u32 pidigits[PILEN] = {
    314159265, 358979323, 846265338,
    327950288, 419716000,
  };

  pao_natural_set_vec(PAO_STDMALLOC, &A, pidigits, PILEN);
 
  pao_status s = pao_natural_add_digit(PAO_STDMALLOC, A, B, &OUT);
  if (s != pao_status_OK) {
    printf("fail: %d\n", s);
    abort();
  }

  print_nat(A);
  print_nat(OUT);

  return 0;
}

int beep() {
  pao_Natural A = pao_natural_empty();

  #define DILEN 5
  u32 digits[DILEN] = {
    1, 0, 0,
  };

  pao_natural_set_vec(PAO_STDMALLOC, &A, digits, DILEN);
  print_nat(A);
  return 0;
}

int main() {
  pi();
  //beep();
}
