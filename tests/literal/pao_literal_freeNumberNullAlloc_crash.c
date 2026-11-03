#include "../common.h"
#include "../../lib/alloc/stdAlloc.h"
#include "../../lib/literal.h"

int main(void) {
  IAllocator mem = stdAlloc_new();
  Number n;
  (void)mem;
  n.kind = numberKind_nativeInt;
  n.data.nativeInt = 1;
  literal_freeNumber(NULL, n);
  return 0;
}
