/*
MIT License
Copyright 2026 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#include <string.h>
#include "../common.h"
#include "../../lib/alloc/stdAlloc.h"
#include "../../lib/literal.h"

#define LEN(array) ((int)(sizeof(array) / sizeof((array)[0])))

static u8 g_pao_heap[common_HEAPSIZE];

/* BEGIN: helpers */
static usize usedBy(const IAllocator* mem) {
  return mem->info(mem->heap).used;
}

/* The decimal text of an `nativeInt`, an `Integer` or a `Rational`. Returns
 * false for the other kinds, or if it does not fit in `buf`. */
static bool numText(Number n, char* buf, usize size) {
  usize len = 0;
  switch (n.kind) {
    case numberKind_nativeInt: {
      int k = snprintf(buf, size, "%lld", (long long)n.data.nativeInt);
      len = (k < 0) ? 0 : (usize)k;
      if (len >= size) {
        return false;
      }
      break;
    }
    case numberKind_integer:
      len = integer_snprint(n.data.integer, buf, size - 1);
      break;
    case numberKind_rational:
      len = rational_snprint(n.data.rational, buf, size - 1);
      break;
    default:
      return false;
  }
  buf[len] = '\0';
  return len > 0;
}

/* `text` is converted to a number of the kind `kind`, that prints as
 * `expected`. */
static bool numIs(const IAllocator* mem, const char* text, NumberKind kind,
                  const char* expected) {
  Str t;
  Number n;
  char buf[512];
  bool ok;
  t.data = text;
  t.length = strlen(text);
  if (literal_toNumber(mem, &t, &n) != status_OK) {
    return false;
  }
  ok = n.kind == kind && numText(n, buf, sizeof(buf)) &&
       strcmp(buf, expected) == 0;
  literal_freeNumber(mem, n);
  return ok;
}

static bool intIs(const IAllocator* mem, const char* text, const char* expected) {
  return numIs(mem, text, numberKind_nativeInt, expected);
}

static bool bigIs(const IAllocator* mem, const char* text, const char* expected) {
  return numIs(mem, text, numberKind_integer, expected);
}

static bool ratIs(const IAllocator* mem, const char* text, const char* expected) {
  return numIs(mem, text, numberKind_rational, expected);
}

static bool floatIs(const IAllocator* mem, const char* text, double expected) {
  Str t;
  Number n;
  bool ok;
  t.data = text;
  t.length = strlen(text);
  if (literal_toNumber(mem, &t, &n) != status_OK) {
    return false;
  }
  ok = n.kind == numberKind_nativeFloat && n.data.nativeFloat == expected;
  literal_freeNumber(mem, n);
  return ok;
}

/* `text` fails with `expected`, writes nothing in `out`, and leaks nothing. */
static bool numFails(const IAllocator* mem, const char* text, Status expected) {
  Str t;
  Number n;
  Status st;
  t.data = text;
  t.length = strlen(text);
  n.kind = numberKind_nativeInt;
  n.data.nativeInt = 777;
  st = literal_toNumber(mem, &t, &n);
  return st == expected && n.kind == numberKind_nativeInt &&
         n.data.nativeInt == 777 && usedBy(mem) == 0;
}

/* The conversion of `text` with an allocator that fails after `n` allocations,
 * for each `n`: every failure is `status_OUTOFMEMORY`, leaves nothing
 * allocated and writes nothing, and the conversion works from some `n` on.
 * `mustFail` is whether it allocates at all. */
static bool numOutOfMemory(const char* text, bool mustFail) {
  int n = 0;
  bool sawFailure = false;
  bool ok = true;
  while (n < 400) {
    i_FailAllocHeap fa;
    IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, n);
    Str t;
    Number out;
    Status st;
    t.data = text;
    t.length = strlen(text);
    out.kind = numberKind_nativeInt;
    out.data.nativeInt = 777;
    st = literal_toNumber(&mem, &t, &out);
    if (st == status_OUTOFMEMORY) {
      sawFailure = true;
      ok = ok && usedBy(&mem) == 0 && out.kind == numberKind_nativeInt &&
           out.data.nativeInt == 777;
    } else {
      ok = ok && st == status_OK;
      if (st == status_OK) {
        literal_freeNumber(&mem, out);
      }
      return ok && usedBy(&mem) == 0 && sawFailure == mustFail;
    }
    n++;
  }
  return false;
}

/* `text` (with its quotes) decodes to the `length` bytes of `expected`. */
static bool strIs(const IAllocator* mem, const char* text, const char* expected,
                  usize length) {
  Str t;
  Str out;
  bool ok;
  t.data = text;
  t.length = strlen(text);
  if (literal_toString(mem, &t, &out) != status_OK) {
    return false;
  }
  ok = out.length == length && (length == 0 || memcmp(out.data, expected, length) == 0);
  ok = ok && (length != 0 || out.data == NULL);
  (void)str_free(mem, out);
  return ok;
}

static bool strFails(const IAllocator* mem, const char* text, Status expected) {
  Str t;
  Str out;
  Status st;
  t.data = text;
  t.length = strlen(text);
  out.data = (const char*)&t;
  out.length = 777;
  st = literal_toString(mem, &t, &out);
  return st == expected && out.data == (const char*)&t && out.length == 777 &&
         usedBy(mem) == 0;
}
/* END: helpers */

/* BEGIN: tests */
static bool test_literalSmallIntegers(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  bool ok = intIs(&mem, "0", "0") && intIs(&mem, "7", "7") &&
            intIs(&mem, "42", "42") && intIs(&mem, "-5", "-5") &&
            intIs(&mem, "-0", "0") && intIs(&mem, "1_000", "1000") &&
            intIs(&mem, "1__", "1") && intIs(&mem, "0_0_7", "7") &&
            intIs(&mem, "00012", "12") && intIs(&mem, "-1_0", "-10") &&
            intIs(&mem, "1000000000", "1000000000") &&
            intIs(&mem, "999999999", "999999999");
  return ok && usedBy(&mem) == 0;
}

static bool test_literalI64Limits(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  bool ok = intIs(&mem, "9223372036854775807", "9223372036854775807") &&
            intIs(&mem, "-9223372036854775807", "-9223372036854775807") &&
            intIs(&mem, "-9223372036854775808", "-9223372036854775808") &&
            bigIs(&mem, "9223372036854775808", "9223372036854775808") &&
            bigIs(&mem, "-9223372036854775809", "-9223372036854775809") &&
            intIs(&mem, "0009_223_372_036_854_775_807", "9223372036854775807") &&
            bigIs(&mem, "18446744073709551615", "18446744073709551615") &&
            bigIs(&mem, "18446744073709551616", "18446744073709551616");
  return ok && usedBy(&mem) == 0;
}

static bool test_literalBigIntegers(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  /* digit counts around the groups of 9: 27 and 28 digits, and 29 and 36 */
  bool ok = bigIs(&mem, "100000000000000000000000000",
                  "100000000000000000000000000") &&
            bigIs(&mem, "1000000000000000000000000000",
                  "1000000000000000000000000000") &&
            bigIs(&mem, "12345678901234567890123456789",
                  "12345678901234567890123456789") &&
            bigIs(&mem, "123456789012345678901234567890123456",
                  "123456789012345678901234567890123456") &&
            bigIs(&mem, "-123456789012345678901234567890",
                  "-123456789012345678901234567890") &&
            bigIs(&mem, "1_2345_6789_0123_4567_8901_2345",
                  "1234567890123456789012345") &&
            bigIs(&mem, "000_00000000000000000000000000012345678901234567890",
                  "12345678901234567890") &&
            bigIs(&mem, "999999999999999999999999999",
                  "999999999999999999999999999") &&
            bigIs(&mem, "1000000000000000001000000000000000001",
                  "1000000000000000001000000000000000001");
  return ok && usedBy(&mem) == 0;
}

static bool test_literalRadix(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  bool ok = intIs(&mem, "0x0", "0") && intIs(&mem, "0xff", "255") &&
            intIs(&mem, "0xFf", "255") && intIs(&mem, "0xFF_ff", "65535") &&
            intIs(&mem, "0x_1", "1") && intIs(&mem, "0x0000_0001", "1") &&
            intIs(&mem, "0b0", "0") && intIs(&mem, "0b101", "5") &&
            intIs(&mem, "0b1_0", "2") &&
            intIs(&mem, "0x7FFFFFFFFFFFFFFF", "9223372036854775807") &&
            bigIs(&mem, "0x8000000000000000", "9223372036854775808") &&
            bigIs(&mem, "0xFFFFFFFFFFFFFFFF", "18446744073709551615") &&
            bigIs(&mem, "0xFFFFFFFFFFFFFFFFFFFF", "1208925819614629174706175") &&
            bigIs(&mem, "0x100000000000000000000000000000000",
                  "340282366920938463463374607431768211456") &&
            bigIs(&mem, "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",
                  "340282366920938463463374607431768211455") &&
            intIs(&mem, "0b111111111111111111111111111111111111111111111111111111111111111",
                  "9223372036854775807") &&
            bigIs(&mem, "0b1000000000000000000000000000000000000000000000000000000000000000",
                  "9223372036854775808") &&
            bigIs(&mem, "0b1111111111111111111111111111111111111111111111111111111111111111",
                  "18446744073709551615") &&
            bigIs(&mem, "0b10000000000000000000000000000000000000000000000000000000000000000000000",
                  "1180591620717411303424") &&
            intIs(&mem, "0x0000000000000000000000000000000000000000000000001", "1");
  return ok && usedBy(&mem) == 0;
}

/* `text` is a `Rational` with that sign, and a numerator that is zero or not. */
static bool ratSignIs(const IAllocator* mem, const char* text, i8 sign,
                      bool numeratorIsZero) {
  Str t;
  Number n;
  bool ok;
  t.data = text;
  t.length = strlen(text);
  if (literal_toNumber(mem, &t, &n) != status_OK) {
    return false;
  }
  ok = n.kind == numberKind_rational && n.data.rational->sign == sign &&
       natural_isZero(&n.data.rational->numerator) == numeratorIsZero;
  literal_freeNumber(mem, n);
  return ok;
}

static bool test_literalRationals(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  bool ok = ratIs(&mem, "22/7", "22/7") && ratIs(&mem, "157/50", "157/50") &&
            ratIs(&mem, "-3/4", "-3/4") && ratIs(&mem, "6/4", "6/4") &&
            ratIs(&mem, "4/1", "4/1") && ratIs(&mem, "0/5", "0/5") &&
            ratIs(&mem, "1_0/0_5", "10/5") &&
            ratIs(&mem, "007/0100", "7/100") &&
            ratIs(&mem, "1000000000/999999999", "1000000000/999999999") &&
            ratIs(&mem, "123456789012345678901234567890/987654321098765432109876543210",
                  "123456789012345678901234567890/987654321098765432109876543210") &&
            ratIs(&mem, "-1/123456789012345678901234567890",
                  "-1/123456789012345678901234567890");
  /* the sign: zero stays positive */
  ok = ok && ratSignIs(&mem, "3/4", +1, false) &&
       ratSignIs(&mem, "-3/4", -1, false) &&
       ratSignIs(&mem, "0/5", +1, true) &&
       ratSignIs(&mem, "-0/5", +1, true) &&
       ratSignIs(&mem, "-0_0/5", +1, true);
  return ok && usedBy(&mem) == 0;
}

static bool test_literalZeroDenominator(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  return numFails(&mem, "1/0", status_DIVISIONBYZERO) &&
         numFails(&mem, "1/000", status_DIVISIONBYZERO) &&
         numFails(&mem, "-1/0_0", status_DIVISIONBYZERO) &&
         numFails(&mem, "0/0", status_DIVISIONBYZERO) &&
         numFails(&mem, "123456789012345678901234567890/0",
                  status_DIVISIONBYZERO);
}

static bool test_literalFloats(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  bool ok = floatIs(&mem, "3.14f", 3.14) && floatIs(&mem, "1e3f", 1000.0) &&
            floatIs(&mem, "1f", 1.0) && floatIs(&mem, "0f", 0.0) &&
            floatIs(&mem, "-2.5f", -2.5) && floatIs(&mem, "3.f", 3.0) &&
            floatIs(&mem, "3.e2f", 300.0) &&
            floatIs(&mem, "1_0.5_0f", 10.5) && floatIs(&mem, "1e-3f", 0.001) &&
            floatIs(&mem, "2.5e2f", 250.0) && floatIs(&mem, "-1e-2f", -0.01) &&
            floatIs(&mem, "1e308f", 1e308) &&
            floatIs(&mem, "1e-400f", 0.0) &&
            floatIs(&mem, "9007199254740993f", 9007199254740992.0);
  return ok && usedBy(&mem) == 0;
}

static bool test_literalUnsupported(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  return numFails(&mem, "3.14", status_UNSUPPORTEDNUMBER) &&
         numFails(&mem, "3.", status_UNSUPPORTEDNUMBER) &&
         numFails(&mem, "0.5", status_UNSUPPORTEDNUMBER) &&
         numFails(&mem, "1e3", status_UNSUPPORTEDNUMBER) &&
         numFails(&mem, "1e-3", status_UNSUPPORTEDNUMBER) &&
         numFails(&mem, "1.5e3", status_UNSUPPORTEDNUMBER) &&
         numFails(&mem, "1/2f", status_UNSUPPORTEDNUMBER) &&
         numFails(&mem, "1/2e3", status_UNSUPPORTEDNUMBER) &&
         numFails(&mem, "1/2e3f", status_UNSUPPORTEDNUMBER) &&
         numFails(&mem, "1e999f", status_UNSUPPORTEDNUMBER) &&
         numFails(&mem, "-1e999f", status_UNSUPPORTEDNUMBER) &&
         numFails(&mem, "1.7977e309f", status_UNSUPPORTEDNUMBER);
}

static bool test_literalBadNumber(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  const char* bad[] = {
    "", "-", "x", "a1", "1x", "1.2.3", "0x", "0b", "0x_", "0b__", "0b12",
    "0xg", "0Xff", "-0x1", "-0b1", "1e", "1e+3", "1e-", "1/", "/2", "1/2/3",
    "1/2.5", "_1", "1 ", " 1", "1f2", "1ff", "1.5.", "1-2", "--1", "e3", ".5"
  };
  bool ok = true;
  int i = 0;
  while (i < LEN(bad)) {
    ok = ok && numFails(&mem, bad[i], status_BADNUMBER);
    i++;
  }
  return ok;
}

static bool test_literalSmallAllocatesNothing(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, 0);
  /* an allocator that fails at once: what is a native number needs nothing */
  return intIs(&mem, "42", "42") && intIs(&mem, "-9223372036854775808", "-9223372036854775808") &&
         intIs(&mem, "0xff", "255") && intIs(&mem, "0b101", "5") &&
         numFails(&mem, "3.14", status_UNSUPPORTEDNUMBER) &&
         numFails(&mem, "1/0", status_DIVISIONBYZERO) &&
         numFails(&mem, "1x", status_BADNUMBER);
}

static bool test_literalNumberOutOfMemory(void) {
  return numOutOfMemory("42", false) &&
         numOutOfMemory("0xff", false) &&
         numOutOfMemory("9223372036854775808", true) &&
         numOutOfMemory("-123456789012345678901234567890123456", true) &&
         numOutOfMemory("0xFFFFFFFFFFFFFFFFFFFF", true) &&
         numOutOfMemory("0x100000000000000000000000000000000", true) &&
         numOutOfMemory("0b1111111111111111111111111111111111111111111111111111111111111111", true) &&
         numOutOfMemory("22/7", true) &&
         numOutOfMemory("-123456789012345678901234567890/987654321098765432109876543210", true) &&
         numOutOfMemory("3.14f", true) &&
         numOutOfMemory("1_0e3f", true);
}

static bool test_literalFreeNativeNumbers(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  Number n;
  n.kind = numberKind_nativeInt;
  n.data.nativeInt = 3;
  literal_freeNumber(&mem, n);
  n.kind = numberKind_nativeFloat;
  n.data.nativeFloat = 3.0;
  literal_freeNumber(&mem, n);
  return usedBy(&mem) == 0;
}

static bool test_literalStrings(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  bool ok = strIs(&mem, "\"\"", "", 0) &&
            strIs(&mem, "\"abc\"", "abc", 3) &&
            strIs(&mem, "\"a b\"", "a b", 3) &&
            strIs(&mem, "\"\\n\"", "\n", 1) &&
            strIs(&mem, "\"\\t\"", "\t", 1) &&
            strIs(&mem, "\"\\'\"", "'", 1) &&
            strIs(&mem, "\"\\\"\"", "\"", 1) &&
            strIs(&mem, "\"\\\\\"", "\\", 1) &&
            strIs(&mem, "\"a\\nb\\tc\"", "a\nb\tc", 5) &&
            strIs(&mem, "\"\\\\n\"", "\\n", 2) &&
            strIs(&mem, "\"say \\\"hi\\\"\"", "say \"hi\"", 8) &&
            strIs(&mem, "\"'\"", "'", 1) &&
            strIs(&mem, "\"\\\\\\\\\"", "\\\\", 2) &&
            strIs(&mem, "\"\xc3\xa9\xe2\x82\xac\"", "\xc3\xa9\xe2\x82\xac", 5) &&
            strIs(&mem, "\"a\rb\"", "a\rb", 3) &&
            strIs(&mem, "\"#not a comment\"", "#not a comment", 14);
  return ok && usedBy(&mem) == 0;
}

static bool test_literalStringErrors(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  return strFails(&mem, "\"abc", status_UNTERMINATEDSTRING) &&
         strFails(&mem, "\"", status_UNTERMINATEDSTRING) &&
         strFails(&mem, "\"abc\\\"", status_UNTERMINATEDSTRING) &&
         strFails(&mem, "\"abc\\", status_UNTERMINATEDSTRING) &&
         strFails(&mem, "\"a\nb\"", status_UNTERMINATEDSTRING) &&
         strFails(&mem, "\"a\\\nb\"", status_UNTERMINATEDSTRING) &&
         strFails(&mem, "\"\\q\"", status_BADESCAPE) &&
         strFails(&mem, "\"\\x41\"", status_BADESCAPE) &&
         strFails(&mem, "\"\\0\"", status_BADESCAPE) &&
         strFails(&mem, "\"a\tb\"", status_BADCHAR) &&
         strFails(&mem, "\"a\x01\"", status_BADCHAR) &&
         strFails(&mem, "\"a\x7f\"", status_BADCHAR) &&
         strFails(&mem, "\"a\"b\"", status_BADCHAR) &&
         strFails(&mem, "\"a\"\"", status_BADCHAR) &&
         strFails(&mem, "abc", status_BADCHAR) &&
         strFails(&mem, "", status_BADCHAR) &&
         strFails(&mem, "'a'", status_BADCHAR);
}

static bool test_literalStringOutOfMemory(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, 0);
  Str t;
  Str out;
  bool ok;
  t.data = "\"a\\nb\"";
  t.length = strlen(t.data);
  out.data = (const char*)&t;
  out.length = 777;
  ok = literal_toString(&mem, &t, &out) == status_OUTOFMEMORY &&
       out.data == (const char*)&t && out.length == 777 && usedBy(&mem) == 0;
  /* the empty string allocates nothing, so an allocator that fails is fine */
  ok = ok && strIs(&mem, "\"\"", "", 0);
  fa.allocsUntilFail = 1;
  ok = ok && strIs(&mem, "\"a\\nb\"", "a\nb", 3) && usedBy(&mem) == 0;
  return ok;
}
/* END: tests */

static Tester tests[] = {
  {"test_literalSmallIntegers", test_literalSmallIntegers},
  {"test_literalI64Limits", test_literalI64Limits},
  {"test_literalBigIntegers", test_literalBigIntegers},
  {"test_literalRadix", test_literalRadix},
  {"test_literalRationals", test_literalRationals},
  {"test_literalZeroDenominator", test_literalZeroDenominator},
  {"test_literalFloats", test_literalFloats},
  {"test_literalUnsupported", test_literalUnsupported},
  {"test_literalBadNumber", test_literalBadNumber},
  {"test_literalSmallAllocatesNothing", test_literalSmallAllocatesNothing},
  {"test_literalNumberOutOfMemory", test_literalNumberOutOfMemory},
  {"test_literalFreeNativeNumbers", test_literalFreeNativeNumbers},
  {"test_literalStrings", test_literalStrings},
  {"test_literalStringErrors", test_literalStringErrors},
  {"test_literalStringOutOfMemory", test_literalStringOutOfMemory},
};

int main(void) {
  run_tests("literal", tests, LEN(tests));
  return 0;
}
