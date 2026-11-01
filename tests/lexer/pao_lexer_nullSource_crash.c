#include "../common.h"
#include "../../lib/lexer.h"

int main(void) {
  Lexer l;

  /* src == NULL must trip the precondition check, even with len == 0 */
  lexer_new(NULL, 0, &l);

  return 0;
}
