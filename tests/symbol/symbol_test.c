/*
MIT License
Copyright 2026 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#include "../common.h"
#include "../../lib/symbol.h"

#define LEN(array) ((int)(sizeof(array) / sizeof((array)[0])))
#define I_symbolTest_HEAPSIZE (1 << 20)

static u8 g_pao_heap[I_symbolTest_HEAPSIZE];

static Str i_symbolTest_lit(const char* s) {
  Str out = { s, strlen(s) };
  return out;
}

/* the name "sym<i>" in `buffer`, which must have at least 16 bytes. */
static Str i_symbolTest_name(char* buffer, int i) {
  Str out;
  out.length = (usize)snprintf(buffer, 16, "sym%d", i);
  out.data = buffer;
  return out;
}

// the same string is always the same symbol, in the order of creation
bool test_intern(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, I_symbolTest_HEAPSIZE, -1);
  SymbolSpace sp;
  Symbol foo, bar, empty, again, other;
  char buffer[4] = { 'f', 'o', 'o', 'x' };
  Str copy = { buffer, 3 };
  Str s;

  if (symbolSpace_new(&mem, 4, &sp) != status_OK) {
    return false;
  }
  s = i_symbolTest_lit("foo");
  if (symbolSpace_intern(&sp, &s, &foo) != status_OK || foo != 0) {
    return false;
  }
  s = i_symbolTest_lit("bar");
  if (symbolSpace_intern(&sp, &s, &bar) != status_OK || bar != 1) {
    return false;
  }
  /* equal content, other bytes */
  if (symbolSpace_intern(&sp, &copy, &again) != status_OK || again != foo) {
    return false;
  }
  s = i_symbolTest_lit("");
  if (symbolSpace_intern(&sp, &s, &empty) != status_OK || empty != 2) {
    return false;
  }
  if (symbolSpace_intern(&sp, &s, &other) != status_OK || other != empty) {
    return false;
  }
  s = i_symbolTest_lit("fo");
  if (symbolSpace_intern(&sp, &s, &other) != status_OK || other != 3) {
    return false;
  }
  if (sp.nextSymbol != 4) {
    return false;
  }
  symbolSpace_free(&sp);
  return mem.info(mem.heap).used == 0;
}

// the space has its own copy of the string
bool test_find(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, I_symbolTest_HEAPSIZE, -1);
  SymbolSpace sp;
  Symbol id;
  char buffer[4] = { 'f', 'o', 'o', 'x' };
  Str s = { buffer, 3 };
  Str found;
  Str foo = i_symbolTest_lit("foo");

  if (symbolSpace_new(&mem, 4, &sp) != status_OK ||
      symbolSpace_intern(&sp, &s, &id) != status_OK) {
    return false;
  }
  buffer[0] = 'z';
  found = symbolSpace_findSymbol(&sp, id);
  if (!str_equals(&found, &foo) || found.data == buffer) {
    return false;
  }
  symbolSpace_free(&sp);
  return true;
}

// strings that differ only in bytes that are easy to get wrong
bool test_bytes(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, I_symbolTest_HEAPSIZE, -1);
  SymbolSpace sp;
  Symbol ids[6];
  Str names[6] = {
    { "a\0b", 3 }, { "a\0c", 3 }, { "a", 1 },
    { "\xc3\xa9", 2 }, { "\xc3\xa8", 2 }, { "\xff", 1 },
  };
  int i = 0;
  int j;

  if (symbolSpace_new(&mem, 2, &sp) != status_OK) {
    return false;
  }
  while (i < 6) {
    if (symbolSpace_intern(&sp, &names[i], &ids[i]) != status_OK || ids[i] != i) {
      return false;
    }
    i++;
  }
  i = 0;
  while (i < 6) {
    Str found = symbolSpace_findSymbol(&sp, ids[i]);
    if (!str_equals(&found, &names[i])) {
      return false;
    }
    j = 0;
    while (j < i) {
      if (ids[i] == ids[j]) {
        return false;
      }
      j++;
    }
    i++;
  }
  symbolSpace_free(&sp);
  return mem.info(mem.heap).used == 0;
}

// starts tiny and grows many times, the symbols stay the same
bool test_growth(void) {
  const int n = 3000;
  usize starts[] = { 1, 2, 7 };
  int s = 0;
  while (s < LEN(starts)) {
    i_FailAllocHeap fa;
    IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, I_symbolTest_HEAPSIZE, -1);
    SymbolSpace sp;
    Symbol id;
    char buffer[16];
    Str name;
    usize used;
    int i = 0;

    if (symbolSpace_new(&mem, starts[s], &sp) != status_OK) {
      return false;
    }
    while (i < n) {
      name = i_symbolTest_name(buffer, i);
      if (symbolSpace_intern(&sp, &name, &id) != status_OK || id != i) {
        return false;
      }
      i++;
    }
    if (sp.symbolsLength < (usize)n || sp.bucketLength < (usize)n) {
      return false;
    }

    /* nothing is allocated for what already exists */
    used = mem.info(mem.heap).used;
    i = 0;
    while (i < n) {
      Str found;
      name = i_symbolTest_name(buffer, i);
      if (symbolSpace_intern(&sp, &name, &id) != status_OK || id != i) {
        return false;
      }
      found = symbolSpace_findSymbol(&sp, id);
      if (!str_equals(&found, &name)) {
        return false;
      }
      i++;
    }
    if (mem.info(mem.heap).used != used) {
      return false;
    }

    symbolSpace_free(&sp);
    if (mem.info(mem.heap).used != 0) {
      return false;
    }
    s++;
  }
  return true;
}

// the allocator fails at every possible moment: the space stays valid
// and nothing leaks
bool test_outOfMemory(void) {
  int k = 0;
  while (k <= 80) {
    i_FailAllocHeap fa;
    IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, I_symbolTest_HEAPSIZE, k);
    SymbolSpace sp;
    Symbol id;
    char buffer[16];
    Str name;
    Status st = symbolSpace_new(&mem, 2, &sp);

    if (st != status_OK) {
      if (st != status_OUTOFMEMORY || mem.info(mem.heap).used != 0) {
        return false;
      }
    } else {
      int made = 0;
      int i = 0;
      while (i < 10) {
        name = i_symbolTest_name(buffer, i);
        st = symbolSpace_intern(&sp, &name, &id);
        if (st != status_OK) {
          if (st != status_OUTOFMEMORY) {
            return false;
          }
          break;
        }
        if (id != i) {
          return false;
        }
        made++;
        i++;
      }

      /* from here on nothing can be allocated */
      fa.allocsUntilFail = 0;
      if (sp.nextSymbol != made) {
        return false;
      }
      i = 0;
      while (i < made) {
        Str found;
        name = i_symbolTest_name(buffer, i);
        if (symbolSpace_intern(&sp, &name, &id) != status_OK || id != i) {
          return false;
        }
        found = symbolSpace_findSymbol(&sp, id);
        if (!str_equals(&found, &name)) {
          return false;
        }
        i++;
      }
      symbolSpace_free(&sp);
      if (mem.info(mem.heap).used != 0) {
        return false;
      }
    }
    k++;
  }
  return true;
}

/* BEGIN: DRIVER CODE */
Tester tests[] = {
  {"test_symbol_intern", test_intern},
  {"test_symbol_find", test_find},
  {"test_symbol_bytes", test_bytes},
  {"test_symbol_growth", test_growth},
  {"test_symbol_outOfMemory", test_outOfMemory},
};

int main(void) {
  run_tests("symbol", tests, LEN(tests));
}
/* END: DRIVER CODE */
