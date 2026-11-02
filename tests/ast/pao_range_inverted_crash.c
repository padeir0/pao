#include "../common.h"
#include "../../lib/ast.h"

int main(void) {
  Position b = { 1, 0 };
  Position e = { 0, 9 };
  /* begin after end is bad usage: range_none is for that */
  range_new(b, e);
  return 0;
}
