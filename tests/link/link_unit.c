/* The second translation unit of `link_test.c`. It is compiled with it. */

#include "link_headers.h"

int linkUnit_check(void);

/* uses a few of the functions, so that this unit has its own copies of them */
int linkUnit_check(void) {
  Str s = { "abc", 3 };
  Lexer l;
  Token t;
  Status st = lexer_new((const u8*)"abc", 3, &l);
  if (st != status_OK || lexer_next(&l, &t) != status_OK) {
    return 0;
  }
  return t.kind == tokenKind_ID && str_hash(&s) != 0 &&
         utf8_decode((const u8*)"a", 1).r == 'a';
}
