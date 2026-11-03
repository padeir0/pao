#include "../common.h"
#include "../../lib/alloc/stdAlloc.h"
#include "../../lib/literal.h"

int main(void) {
  IAllocator mem = stdAlloc_new();
  Number n;
  n.kind = numberKind_decimal;
  n.data.nativeInt = 0;
  literal_freeNumber(&mem, n);
  return 0;
}
