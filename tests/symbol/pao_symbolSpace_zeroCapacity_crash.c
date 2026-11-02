#include "../common.h"
#include "../../lib/symbol.h"
#include "../../lib/alloc/stdAlloc.h"

int main(void) {
  IAllocator mem = stdAlloc_new();
  SymbolSpace sp;

  /* the capacity of a symbol space cannot be 0 */
  symbolSpace_new(&mem, 0, &sp);

  return 0;
}
