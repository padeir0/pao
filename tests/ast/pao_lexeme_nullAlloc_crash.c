#include "../common.h"
#include "../../lib/ast.h"

int main(void) {
  u8 src[1] = { 'a' };
  Token t = { tokenKind_ID, 0, 1, 0, 0 };
  Lexeme lx;
  lexeme_new(NULL, &t, src, 1, &lx);
  return 0;
}
