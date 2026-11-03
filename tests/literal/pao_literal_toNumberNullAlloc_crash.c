#include "../common.h"
#include "../../lib/alloc/stdAlloc.h"
#include "../../lib/literal.h"

int main(void) {
  IAllocator mem = stdAlloc_new();
  Str t = { "1", 1 };
  Number n;
  (void)mem;
  (void)literal_toNumber(NULL, &t, &n);
  return 0;
}
