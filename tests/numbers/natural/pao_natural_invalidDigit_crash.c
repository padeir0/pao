#include "../../common.h"
#include "../../../lib/numbers/natural.h"
#include "../../../lib/alloc/stdAlloc.h"

int main(void) {
  IAllocator alloc = stdAlloc_new();
  Natural out = natural_new();

  /* natural_BASE is one past the highest valid digit value */
  natural_set(&alloc, natural_BASE, &out);

  return 0;
}
