#include "../../common.h"
#include "../../../lib/numbers/natural.h"

int main(void) {
  Natural out = natural_new();

  /* mem == NULL must trip the null-pointer precondition check */
  natural_set(NULL, 42, &out);

  return 0;
}
