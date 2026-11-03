#include "../common.h"
#include "../../lib/alloc/stdAlloc.h"
#include "../../lib/literal.h"

int main(void) {
  IAllocator mem = stdAlloc_new();
  Number n;
  (void)literal_toNumber(&mem, NULL, &n);
  return 0;
}
