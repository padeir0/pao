/*
MIT License
Copyright 2026 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#include "../common.h"
#include "../../lib/str.h"

#define LEN(array) ((int)(sizeof(array) / sizeof((array)[0])))

static u8 g_pao_heap[common_HEAPSIZE];

static Str i_strTest_lit(const char* s) {
  Str out = { s, strlen(s) };
  return out;
}

// tests that equal means: same length and same bytes
bool test_equals(void) {
  Str abc = i_strTest_lit("abc");
  Str abd = i_strTest_lit("abd");
  Str ab = i_strTest_lit("ab");
  Str empty = { NULL, 0 };
  Str other = i_strTest_lit("");
  char buffer[4] = { 'a', 'b', 'c', 'x' };
  Str same = { buffer, 3 };

  return str_equals(&abc, &abc) &&
         str_equals(&abc, &same) &&
         !str_equals(&abc, &abd) &&
         !str_equals(&abc, &ab) &&
         !str_equals(&ab, &abc) &&
         str_equals(&empty, &empty) &&
         str_equals(&empty, &other) &&
         !str_equals(&empty, &abc);
}

// the bytes matter, even the ones above 0x7f and the NULs
bool test_equalsBytes(void) {
  Str a = { "a\0b", 3 };
  Str b = { "a\0c", 3 };
  Str c = { "a", 1 };
  Str e1 = { "\xc3\xa9", 2 };
  Str e2 = { "\xc3\xa8", 2 };
  return !str_equals(&a, &b) && !str_equals(&a, &c) &&
         !str_equals(&e1, &e2) && str_equals(&e1, &e1);
}

// the values were computed apart from this code. The bytes above 0x7f
// catch a hash that reads `char` instead of `u8`
bool test_hash(void) {
  Str empty = { NULL, 0 };
  Str a = i_strTest_lit("a");
  Str hello = i_strTest_lit("hello");
  Str acute = i_strTest_lit("\xc3\xa9");
  Str ff = i_strTest_lit("\xff");
  Str nul = { "a\0b", 3 };
  char copy[6] = { 'h', 'e', 'l', 'l', 'o', 0 };
  Str hello2 = { copy, 5 };

  return str_hash(&empty) == 0xcafebabeu &&
         str_hash(&a) == 0x773b25u &&
         str_hash(&hello) == 0x6e6ce801u &&
         str_hash(&acute) == 0xa7885ebau &&
         str_hash(&ff) == 0xbce86807u &&
         str_hash(&nul) == 0x262c1c99u &&
         str_hash(&hello) == str_hash(&hello2);
}

// tests that the copy has its own bytes
bool test_copy(void) {
  char source[4] = { 'a', 'b', 'c', 'd' };
  Str s = { source, 4 };
  Str copy;
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  Status st = str_copy(&mem, &s, &copy);
  if (st != status_OK) {
    return false;
  }
  source[0] = 'z';
  if (copy.length != 4 || copy.data == source || memcmp(copy.data, "abcd", 4) != 0) {
    return false;
  }
  if (mem.info(mem.heap).used == 0) {
    return false;
  }
  st = str_free(&mem, copy);
  return st == status_OK && mem.info(mem.heap).used == 0;
}

// an empty string needs no memory
bool test_copyEmpty(void) {
  Str empty = { NULL, 0 };
  Str copy = { "x", 1 };
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, 0);
  Status st = str_copy(&mem, &empty, &copy);
  if (st != status_OK || copy.length != 0 || copy.data != NULL) {
    return false;
  }
  return str_free(&mem, copy) == status_OK && mem.info(mem.heap).used == 0;
}

bool test_copyOutOfMemory(void) {
  Str s = i_strTest_lit("abcd");
  Str copy = { "x", 1 };
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, 0);
  Status st = str_copy(&mem, &s, &copy);
  // `out` is left alone
  return st == status_OUTOFMEMORY && copy.length == 1 &&
         mem.info(mem.heap).used == 0;
}

/* BEGIN: DRIVER CODE */
Tester tests[] = {
  {"test_str_equals", test_equals},
  {"test_str_equalsBytes", test_equalsBytes},
  {"test_str_hash", test_hash},
  {"test_str_copy", test_copy},
  {"test_str_copyEmpty", test_copyEmpty},
  {"test_str_copyOutOfMemory", test_copyOutOfMemory},
};

int main(void) {
  run_tests("str", tests, LEN(tests));
}
/* END: DRIVER CODE */
