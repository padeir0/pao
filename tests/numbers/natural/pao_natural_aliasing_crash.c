#include "../../common.h"
#include "../../../lib/numbers/natural.h"
#include "../../../lib/alloc/stdAlloc.h"

int main(void) {
  IAllocator alloc = stdAlloc_new();
  Natural a = natural_new();
  Natural b = natural_new();
  Status s;

  s = natural_set(&alloc, 6, &a); checkStatus(s);
  s = natural_set(&alloc, 7, &b); checkStatus(s);

  /* out (&a) aliases A: natural_mult forbids this */
  natural_mult(&alloc, &a, &b, &a);

  return 0;
}
