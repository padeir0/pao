#include "../common.h"
#include "../../lib/lexer.h"

int main(void) {
  Lexer l;
  u8 src[1] = { 'a' };
  Status st = lexer_new(src, 1, &l);
  checkStatus(st);

  /* out == NULL must trip the precondition check */
  lexer_next(&l, NULL);

  return 0;
}
