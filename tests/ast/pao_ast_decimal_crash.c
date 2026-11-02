#include "../common.h"
#include "../../lib/alloc/stdAlloc.h"
#include "../../lib/ast.h"

int main(void) {
  IAllocator mem = stdAlloc_new();
  AST* out = NULL;
  Lexeme tk = { tokenKind_NUM, { "1", 1 }, { { 0, 0 }, { 0, 1 } } };
  (void)tk; (void)out; (void)mem;
  Number n;
  n.kind = numberKind_decimal;
  n.data.nativeInt = 0;
  (void)ast_newNumber(&mem, tk, n, &out);
  return 0;
}
