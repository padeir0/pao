#include "../src/pao_natural.h"
#include "../src/pao_stdAlloc.h"

#define I_PAO_memoryLength 2048
char g_pao_memory[I_PAO_memoryLength];

int main(void) {
  pao_Natural A = pao_natural_empty();
  pao_natural_set(PAO_stdAlloc, &A, 5);
  u32 B = 3;
  pao_Natural Q = pao_natural_empty();
  u32 R;

  pao_Natural exp_Q = pao_natural_empty();
  pao_natural_set(PAO_stdAlloc, &exp_Q, 1);
  u32 exp_R = 2;

  pao_status st = pao_natural_divDigit(PAO_stdAlloc, &A, B, &Q, &R);

  if (st != PAO_status_ok) {
    printf("status: %d\n", st);
    return 1;
  }

  usize written = pao_natural_snprint(Q, g_pao_memory, I_PAO_memoryLength);
  printf("5 = %.*s * 3 + %u\n", (int)written, g_pao_memory, R);

  if (R != exp_R || pao_natural_equal(&Q, &exp_Q)) {
    printf("SUCCESS\n");
    return 0;
  }
  printf("FAIL\n");
  return 1;
}
