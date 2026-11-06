#include "../common.h"
#include "../../lib/alloc/stdAlloc.h"
#include "../../lib/parser.h"

int main(void) {
  IAllocator mem = stdAlloc_new();
  SymbolSpace sp;
  AST* out = NULL;
  ParseError err;
  const u8 src[1] = { 0 };
  (void)mem; (void)sp; (void)out; (void)err; (void)src;
  checkStatus(symbolSpace_new(&mem, 8, &sp));
  (void)parser_parse(&mem, &sp, NULL, 0, 8, &out, &err);
  return 0;
}
