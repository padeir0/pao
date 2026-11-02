#include "../../common.h"
#include "../../../lib/alloc/stackAlloc.h"

u8 g_pao_buffer[1024];

int main(void) {
  StackAlloc* a = stackAlloc_create(g_pao_buffer, 1024, 16);
  if (a == NULL) {
    return 1;
  }
  void* x = stackAlloc_alloc(a); (void)stackAlloc_alloc(a); (void)stackAlloc_free(a, x);
  return 0;
}
