#include "../common.h"
#include "../../lib/ast.h"

int main(void) {
  u8 src[1] = { 'a' };
  Token t = { tokenKind_ID, 0, 1, 0, 0 };
  range_fromToken(&t, src, 1, NULL);
  return 0;
}
