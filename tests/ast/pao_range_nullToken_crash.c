#include "../common.h"
#include "../../lib/ast.h"

int main(void) {
  u8 src[1] = { 'a' };
  Range r;
  /* a null token must trip the precondition check */
  range_fromToken(NULL, src, 1, &r);
  return 0;
}
