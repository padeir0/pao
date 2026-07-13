#include "../../common.h"
#include "../../../lib/alloc/pao_flAlloc.h"
#include <stdlib.h>

#define I_PAO_FLALLOC_TEST_buffSize  8192
u8    g_pao_buffer[I_PAO_FLALLOC_TEST_buffSize];

int main(void) {
  pao_Status s = pao_flAlloc_create(I_PAO_FLALLOC_TEST_buffSize, g_pao_buffer);
  if (s != PAO_status_ok) {
    return 1;
  }
  pao_flAlloc* fl = (pao_flAlloc*)g_pao_buffer;

  if (fl == NULL) {
    return 1;
  }

  u8* obj1 = pao_flAlloc_alloc(fl, 128);
  u8* obj2 = pao_flAlloc_alloc(fl, 128);
  pao_flAlloc_free(fl, obj1);
  pao_flAlloc_free(fl, obj2);
  pao_flAlloc_free(fl, obj2);

  return 0;
}
