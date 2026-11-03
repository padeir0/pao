#include "../common.h"
#include "../../lib/alloc/stdAlloc.h"
#include "../../lib/literal.h"

int main(void) {
  IAllocator mem = stdAlloc_new();
  Str out;
  (void)literal_toString(&mem, NULL, &out);
  return 0;
}
