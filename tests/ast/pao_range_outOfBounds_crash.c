#include "../common.h"
#include "../../lib/ast.h"

int main(void) {
  u8 src[1] = { 'a' };
  Token t = { tokenKind_ID, 0, 2, 0, 0 };
  Range r;
  /* the token ends after the source */
  range_fromToken(&t, src, 1, &r);
  return 0;
}
