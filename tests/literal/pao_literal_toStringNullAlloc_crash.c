#include "../common.h"
#include "../../lib/alloc/stdAlloc.h"
#include "../../lib/literal.h"

int main(void) {
  IAllocator mem = stdAlloc_new();
  Str t = { "\"a\"", 3 };
  Str out;
  (void)mem;
  (void)literal_toString(NULL, &t, &out);
  return 0;
}
