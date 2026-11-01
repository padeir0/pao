#include "../common.h"
#include "../../lib/lexer.h"

int main(void) {
  Token t;

  /* l == NULL must trip the precondition check */
  lexer_peek(NULL, &t);

  return 0;
}
