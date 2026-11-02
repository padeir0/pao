/*
MIT License
Copyright 2026 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

/* The library is made of headers that are included in many `.c` files, so
   every function in them must be `static inline`. This test is compiled
   together with `link_unit.c`, and both include all the headers: if some
   function is not `static inline`, the linker complains of a duplicate. */

#include "../common.h"
#include "link_headers.h"

int linkUnit_check(void);

bool test_link(void) {
  Str s = { "abc", 3 };
  Lexer l;
  Token t;
  Status st = lexer_new((const u8*)"abc", 3, &l);
  if (st != status_OK || lexer_next(&l, &t) != status_OK) {
    return false;
  }
  return t.kind == tokenKind_ID && str_hash(&s) != 0 && linkUnit_check() == 1;
}

/* BEGIN: DRIVER CODE */
Tester tests[] = {
  {"test_link", test_link},
};

int main(void) {
  run_tests("link", tests, (int)(sizeof(tests) / sizeof(tests[0])));
}
/* END: DRIVER CODE */
