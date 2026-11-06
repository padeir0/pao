#include "../common.h"
#include "../../lib/alloc/stdAlloc.h"
#include "../../lib/ast.h"

int main(void) {
  IAllocator mem = stdAlloc_new();
  AST* out = NULL;
  Lexeme tk = { tokenKind_COMMA, { ",", 1 }, { { 0, 0 }, { 0, 1 } } };
  (void)tk; (void)out; (void)mem;
  /* the operators of the suffixes are symbols, but a comma is not */
  (void)ast_newSymbol(&mem, tk, 0, &out);
  return 0;
}
