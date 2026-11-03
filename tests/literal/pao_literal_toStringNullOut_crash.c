#include "../common.h"
#include "../../lib/alloc/stdAlloc.h"
#include "../../lib/literal.h"

int main(void) {
  IAllocator mem = stdAlloc_new();
  Str t = { "\"a\"", 3 };
  (void)literal_toString(&mem, &t, NULL);
  return 0;
}
