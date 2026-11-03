#include "../common.h"
#include "../../lib/alloc/stdAlloc.h"
#include "../../lib/literal.h"

int main(void) {
  IAllocator mem = stdAlloc_new();
  Str t = { NULL, 3 };
  Str out;
  (void)literal_toString(&mem, &t, &out);
  return 0;
}
