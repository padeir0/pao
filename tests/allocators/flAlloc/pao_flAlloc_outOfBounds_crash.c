#include "../../common.h"
#include "../../../lib/alloc/flAlloc.h"
#include <stdlib.h>

#define I_PAO_FLALLOC_TEST_buffSize  8192
u8    g_pao_buffer[I_PAO_FLALLOC_TEST_buffSize];

int main(void) {
  Status s = flAlloc_create(I_PAO_FLALLOC_TEST_buffSize, g_pao_buffer);
  if (s != status_OK) {
    return 1;
  }
  FLAlloc* fl = (FLAlloc*)g_pao_buffer;

  if (fl == NULL) {
    return 1;
  }
  u8* external = malloc(128);
  memset(external, 1, 128);

  flAlloc_free(fl, external+8);

  return 0;
}
