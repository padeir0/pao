#include "../src/pao_natural.h"
#include "../src/pao_status.h"
#include "../src/pao_stdAlloc.h"
#include <stdio.h>

#define BUFF_LENGTH 2048
char buffer[BUFF_LENGTH];

void print_nat(pao_Natural n) {
  usize written = pao_natural_snprint(n, buffer, BUFF_LENGTH);
  if (written == 0) {
    printf("nothing printed :(");
    abort();
  }
  printf("%.*s", (int)written, buffer);
  printf(" (length: %d, cap: %d)\n", n.len, n.cap);
}

int pi(void) {
  pao_Natural A = pao_natural_empty();
  u32 B = 2;
  pao_Natural OUT = pao_natural_empty();

  u32 pidigits[] = {
    314159265, 358979323, 846265338,
    327950288, 419716000,
  };
  #define PILEN (sizeof(pidigits) / sizeof(pidigits[0]))

  pao_natural_setVec(PAO_STDMALLOC, &A, pidigits, PILEN);
 
  pao_status s = pao_natural_addDigit(PAO_STDMALLOC, A, B, &OUT);
  if (s != PAO_status_ok) {
    printf("fail: %d\n", s);
    abort();
  }

  print_nat(A);
  print_nat(OUT);

  return 0;
}

int beep(void) {
  pao_Natural A = pao_natural_empty();

  #define DILEN 5
  u32 digits[DILEN] = {
    1, 0, 0,
  };

  pao_natural_setVec(PAO_STDMALLOC, &A, digits, DILEN);
  print_nat(A);
  return 0;
}

int main(void) {
  pi();
  //beep();
}
