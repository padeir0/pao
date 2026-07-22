#include "../../common.h"
#include "../../../lib/alloc/pool.h"
#include <stdlib.h>
#include <string.h>

#define I_PAO_POOL_TEST_buffSize  4096
u8    g_pao_buffer[I_PAO_POOL_TEST_buffSize];

int main(void) {
  Status s = pool_new(I_PAO_POOL_TEST_buffSize, sizeof(i_pool_Node), g_pao_buffer);
  if (s != status_OK) {
    return 1;
  }
  Pool* p = (Pool*)g_pao_buffer;

  u8* external = malloc(sizeof(i_pool_Node));
  memset(external, 1, sizeof(i_pool_Node));

  pool_free(p, external);

  return 0;
}
