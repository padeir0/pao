#include "../src/mb_natural.h"
#include "../src/mb_stdAlloc.h"

#define I_MB_memoryLength 2048
char g_mb_memory[I_MB_memoryLength];

int main(void) {
  mb_Natural A = mb_natural_empty();
  mb_natural_set(MB_stdAlloc, 5, &A);
  u32 B = 3;
  mb_Natural Q = mb_natural_empty();
  u32 R;

  mb_Natural exp_Q = mb_natural_empty();
  mb_natural_set(MB_stdAlloc, 1, &exp_Q);
  u32 exp_R = 2;

  mb_status st = mb_natural_divDigit(MB_stdAlloc, &A, B, &Q, &R);

  if (st != MB_status_ok) {
    printf("status: %d\n", st);
    return 1;
  }

  usize written = mb_natural_snprint(Q, g_mb_memory, I_MB_memoryLength);
  printf("5 = %.*s * 3 + %u\n", (int)written, g_mb_memory, R);

  if (R != exp_R || mb_natural_equal(&Q, &exp_Q)) {
    printf("SUCCESS\n");
    return 0;
  }
  printf("FAIL\n");
  return 1;
}
