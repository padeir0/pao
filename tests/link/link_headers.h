/* Every header of the library. Two translation units that include this must
   link together: a function that is not `static inline` would be defined
   twice. See `link_test.c`. */

#ifndef PAO_TEST_LINK_HEADERS_H
#define PAO_TEST_LINK_HEADERS_H

#include "../../lib/ast.h"
#include "../../lib/basicTypes.h"
#include "../../lib/blank.h"
#include "../../lib/buffer.h"
#include "../../lib/colors.h"
#include "../../lib/config.h"
#include "../../lib/debug.h"
#include "../../lib/iallocator.h"
#include "../../lib/lexer.h"
#include "../../lib/list.h"
#include "../../lib/literal.h"
#include "../../lib/order.h"
#include "../../lib/status.h"
#include "../../lib/str.h"
#include "../../lib/symbol.h"
#include "../../lib/utf8.h"
#include "../../lib/util.h"
#include "../../lib/alloc/flAlloc.h"
#include "../../lib/alloc/linearAlloc.h"
#include "../../lib/alloc/nullAlloc.h"
#include "../../lib/alloc/pool.h"
#include "../../lib/alloc/stackAlloc.h"
#include "../../lib/alloc/stdAlloc.h"
#include "../../lib/numbers/integer.h"
#include "../../lib/numbers/natural.h"
#include "../../lib/numbers/number.h"
#include "../../lib/numbers/rational.h"

#endif
