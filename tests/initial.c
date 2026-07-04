#include "../src/pao_integer.h"
#include "../src/pao_stdAlloc.h"
#include "../src/pao_status.h"
#include "common.h"

#define I_PAO_memoryLength 2048
char g_pao_memory[I_PAO_memoryLength];

#define alloc PAO_stdAlloc

int main(void) {
  pao_Integer A = pao_integer_new();
  u32 A_digs[] = {314159265, 358979323, 846264338, 327950288, 419716000};
  pao_Status s = pao_integer_setVec(alloc, -1, A_digs, sizeof(A_digs)/sizeof(A_digs[0]), &A); checkStatus(s);

  usize written = pao_integer_snprint(&A, g_pao_memory, I_PAO_memoryLength);
  if (written == 0) {
    printf("nothing was written");
  }
  printf("%.*s\n", (int)written, g_pao_memory);

  return 1;
}
