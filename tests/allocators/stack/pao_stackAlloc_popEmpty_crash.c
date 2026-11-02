#include "../../common.h"
#include "../../../lib/alloc/stackAlloc.h"

u8 g_pao_buffer[1024];

int main(void) {
  StackAlloc* a = stackAlloc_create(g_pao_buffer, 1024, 16);
  if (a == NULL) {
    return 1;
  }
  stackAlloc_pop(a);
  return 0;
}
