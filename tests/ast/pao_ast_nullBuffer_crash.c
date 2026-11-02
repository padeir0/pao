#include "../common.h"
#include "../../lib/alloc/stdAlloc.h"
#include "../../lib/ast.h"

int main(void) {
  IAllocator mem = stdAlloc_new();
  AST* out = NULL;
  Lexeme tk = { tokenKind_NUM, { "1", 1 }, { { 0, 0 }, { 0, 1 } } };
  (void)tk; (void)out; (void)mem;
  SymbolSpace sp;
  AST* node;
  checkStatus(symbolSpace_new(&mem, 4, &sp));
  checkStatus(ast_newNode(&mem, astKind_CALL, NULL, 0, range_none(), &node));
  (void)ast_snprint(node, &sp, NULL, 10);
  return 0;
}
