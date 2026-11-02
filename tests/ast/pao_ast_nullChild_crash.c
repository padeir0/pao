#include "../common.h"
#include "../../lib/alloc/stdAlloc.h"
#include "../../lib/ast.h"

int main(void) {
  IAllocator mem = stdAlloc_new();
  AST* out = NULL;
  Lexeme tk = { tokenKind_NUM, { "1", 1 }, { { 0, 0 }, { 0, 1 } } };
  (void)tk; (void)out; (void)mem;
  AST* kids[1] = { NULL };
  (void)ast_newNode(&mem, astKind_CALL, kids, 1, range_none(), &out);
  return 0;
}
