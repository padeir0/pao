#include "../common.h"
#include "../../lib/symbol.h"
#include "../../lib/alloc/stdAlloc.h"

int main(void) {
  IAllocator mem = stdAlloc_new();
  SymbolSpace sp;
  Status st = symbolSpace_new(&mem, 4, &sp);
  checkStatus(st);

  /* a negative symbol is never valid */
  symbolSpace_findSymbol(&sp, -1);

  return 0;
}
