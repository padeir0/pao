#include "../../common.h"
#include "../../../lib/alloc/pool.h"
#include <stdlib.h>

#define I_PAO_POOL_TEST_buffSize  4096
#define I_PAO_POOL_TEST_chunkSize 32
u8    g_pao_buffer[I_PAO_POOL_TEST_buffSize];

int main(void) {
  Status s = pool_new(I_PAO_POOL_TEST_buffSize, I_PAO_POOL_TEST_chunkSize, g_pao_buffer);
  if (s != status_OK) {
    return 1;
  }
  Pool* p = (Pool*)g_pao_buffer;

  u8* obj = (u8*)pool_alloc(p);
  if (obj == NULL) {
    return 1;
  }

  /* obj+1 is inside the pool's memory range but not aligned to a
     chunk boundary */
  pool_free(p, obj + 1);

  return 0;
}
