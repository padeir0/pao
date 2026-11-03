#include "../common.h"
#include "../../lib/alloc/stdAlloc.h"
#include "../../lib/literal.h"

int main(void) {
  IAllocator mem = stdAlloc_new();
  Str t = { NULL, 3 };
  Number n;
  (void)literal_toNumber(&mem, &t, &n);
  return 0;
}
