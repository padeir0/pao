#include "../common.h"
#include "../../lib/symbol.h"
#include "../../lib/alloc/stdAlloc.h"

int main(void) {
  IAllocator mem = stdAlloc_new();
  SymbolSpace sp;
  Symbol id;
  Str s = { "a", 1 };
  Status st = symbolSpace_new(&mem, 4, &sp);
  checkStatus(st);
  st = symbolSpace_intern(&sp, &s, &id);
  checkStatus(st);

  /* only symbol 0 exists, even though there is room for 4 */
  symbolSpace_findSymbol(&sp, 1);

  return 0;
}
