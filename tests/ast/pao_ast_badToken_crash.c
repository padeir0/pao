#include "../common.h"
#include "../../lib/alloc/stdAlloc.h"
#include "../../lib/ast.h"

int main(void) {
  IAllocator mem = stdAlloc_new();
  AST* out = NULL;
  Lexeme tk = { tokenKind_NUM, { "1", 1 }, { { 0, 0 }, { 0, 1 } } };
  (void)tk; (void)out; (void)mem;
  (void)ast_newSymbol(&mem, tk, 0, &out); /* a NUM token is not a symbol */
  return 0;
}
