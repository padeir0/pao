#include "../common.h"
#include "../../lib/alloc/stdAlloc.h"
#include "../../lib/ast.h"

int main(void) {
  IAllocator mem = stdAlloc_new();
  AST* out = NULL;
  (void)out; (void)mem;
  /* only a CALL or a TUPLE has commas */
  (void)ast_newFormNode(&mem, astKind_PROGRAM, astForm_NONE, true, NULL, 0,
                        range_none(), &out);
  return 0;
}
