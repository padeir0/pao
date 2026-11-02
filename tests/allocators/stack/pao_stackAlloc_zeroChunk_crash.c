#include "../../common.h"
#include "../../../lib/alloc/stackAlloc.h"

u8 g_pao_buffer[1024];

int main(void) {
  (void)stackAlloc_create(g_pao_buffer, 1024, 0);
  return 0;
}
