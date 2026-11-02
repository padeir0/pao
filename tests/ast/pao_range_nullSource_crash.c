#include "../common.h"
#include "../../lib/ast.h"

int main(void) {
  Token t = { tokenKind_EOF, 0, 0, 0, 0 };
  Range r;
  range_fromToken(&t, NULL, 0, &r);
  return 0;
}
