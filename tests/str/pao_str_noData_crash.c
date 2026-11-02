#include "../common.h"
#include "../../lib/str.h"
#include "../../lib/alloc/stdAlloc.h"

int main(void) {
  IAllocator mem = stdAlloc_new();
  Str s = { NULL, 4 };
  Str out;

  /* a string with length but without data must trip the check */
  str_copy(&mem, &s, &out);

  return 0;
}
