#include "../src/mb_integer.h"
#include "../src/mb_stdAlloc.h"
#include "../src/mb_status.h"
#include "common.h"

#define I_MB_memoryLength 2048
char g_mb_memory[I_MB_memoryLength];

#define alloc MB_stdAlloc

int main(void) {
  mb_Integer A = mb_integer_new();
  u32 A_digs[] = {314159265, 358979323, 846264338, 327950288, 419716000};
  mb_Status s = mb_integer_setVec(alloc, -1, A_digs, sizeof(A_digs)/sizeof(A_digs[0]), &A); checkStatus(s);

  usize written = mb_integer_snprint(&A, g_mb_memory, I_MB_memoryLength);
  if (written == 0) {
    printf("nothing was written");
  }
  printf("%.*s\n", (int)written, g_mb_memory);

  return 1;
}
