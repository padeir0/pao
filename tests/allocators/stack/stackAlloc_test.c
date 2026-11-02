/*
MIT License
Copyright 2026 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#include "../../common.h"
#include "../../../lib/alloc/stackAlloc.h"

#define LEN(array) ((int)(sizeof(array) / sizeof((array)[0])))
#define I_stackAlloc_TEST_buffSize 8192
#define I_stackAlloc_TEST_objs 16

static u8 g_pao_buffer[I_stackAlloc_TEST_buffSize];
static u8* g_pao_objs[I_stackAlloc_TEST_objs];

static void setbuff(u8* obj, usize size, u8 value) {
  usize i = 0;
  while (i < size) {
    obj[i] = value;
    i++;
  }
}

static bool checkbuff(const u8* obj, usize size, u8 value) {
  usize i = 0;
  while (i < size) {
    if (obj[i] != value) {
      return false;
    }
    i++;
  }
  return true;
}

/* create rejects a null buffer and a buffer that cannot hold the header and
 * one chunk, and rounds the chunk up to a word. */
bool test_stackCreate(void) {
  StackAlloc* a;
  if (stackAlloc_create(NULL, I_stackAlloc_TEST_buffSize, 16) != NULL) {
    return false;
  }
  if (stackAlloc_create(g_pao_buffer, sizeof(StackAlloc), 16) != NULL) {
    return false;
  }
  if (stackAlloc_create(g_pao_buffer, 0, 16) != NULL) {
    return false;
  }
  if (stackAlloc_create(g_pao_buffer, I_stackAlloc_TEST_buffSize, SIZE_MAX) != NULL) {
    return false;
  }
  a = stackAlloc_create(g_pao_buffer, I_stackAlloc_TEST_buffSize, 1);
  if (a == NULL || stackAlloc_chunkSize(a) != WORD) {
    return false;
  }
  a = stackAlloc_create(g_pao_buffer, I_stackAlloc_TEST_buffSize, WORD + 1);
  if (a == NULL || stackAlloc_chunkSize(a) != 2 * WORD) {
    return false;
  }
  /* header plus exactly one chunk */
  a = stackAlloc_create(g_pao_buffer, sizeof(StackAlloc) + 2 * WORD, 2 * WORD);
  if (a == NULL || stackAlloc_total(a) != 2 * WORD || !stackAlloc_empty(a)) {
    return false;
  }
  return true;
}

/* the header starts clean: nothing is allocated (the old stack_f never
 * initialized it). */
bool test_stackCleanHeader(void) {
  StackAlloc* a;
  setbuff(g_pao_buffer, I_stackAlloc_TEST_buffSize, 0xAB);
  a = stackAlloc_create(g_pao_buffer, I_stackAlloc_TEST_buffSize, 24);
  return a != NULL && stackAlloc_empty(a) && stackAlloc_used(a) == 0 &&
         stackAlloc_available(a) == stackAlloc_total(a);
}

/* the old test_seq: many chunk sizes, allocate, check the contents, free. */
bool test_stackSeq(void) {
  usize chunk = 1;
  while (chunk < 64) {
    StackAlloc* a = stackAlloc_create(g_pao_buffer, I_stackAlloc_TEST_buffSize, chunk);
    usize size;
    int i = 0;
    if (a == NULL) {
      return false;
    }
    size = stackAlloc_chunkSize(a);
    while (i < I_stackAlloc_TEST_objs) {
      u8* obj = (u8*)stackAlloc_alloc(a);
      if (obj == NULL || (uptr)obj % WORD != 0) {
        return false;
      }
      setbuff(obj, size, (u8)i);
      g_pao_objs[i] = obj;
      i++;
    }
    if (stackAlloc_used(a) != size * I_stackAlloc_TEST_objs) {
      return false;
    }
    i = 0;
    while (i < I_stackAlloc_TEST_objs) {
      if (!checkbuff(g_pao_objs[i], size, (u8)i)) {
        return false;
      }
      i++;
    }
    i = I_stackAlloc_TEST_objs - 1;
    while (0 <= i) {
      if (stackAlloc_free(a, g_pao_objs[i]) != status_OK) {
        return false;
      }
      i--;
    }
    if (!stackAlloc_empty(a)) {
      return false;
    }
    chunk++;
  }
  return true;
}

/* the chunks are side by side, which is what the parser needs. */
bool test_stackContiguous(void) {
  StackAlloc* a = stackAlloc_create(g_pao_buffer, I_stackAlloc_TEST_buffSize, sizeof(void*));
  u8* first;
  usize i = 1;
  if (a == NULL) {
    return false;
  }
  first = (u8*)stackAlloc_alloc(a);
  if (first == NULL) {
    return false;
  }
  while (i < 10) {
    u8* next = (u8*)stackAlloc_alloc(a);
    if (next != first + i * stackAlloc_chunkSize(a)) {
      return false;
    }
    i++;
  }
  return true;
}

/* allocation stops when full (the old one wrote out of the buffer), and the
 * stack works again after a free. */
bool test_stackFull(void) {
  StackAlloc* a = stackAlloc_create(g_pao_buffer, sizeof(StackAlloc) + 100, 16);
  usize chunks;
  usize i = 0;
  void* last = NULL;
  if (a == NULL) {
    return false;
  }
  chunks = stackAlloc_total(a) / stackAlloc_chunkSize(a);
  if (chunks == 0) {
    return false;
  }
  while (i < chunks) {
    last = stackAlloc_alloc(a);
    if (last == NULL) {
      return false;
    }
    i++;
  }
  if (stackAlloc_available(a) != 0 || stackAlloc_alloc(a) != NULL) {
    return false;
  }
  if (stackAlloc_free(a, last) != status_OK) {
    return false;
  }
  if (stackAlloc_available(a) != stackAlloc_chunkSize(a)) {
    return false;
  }
  return stackAlloc_alloc(a) == last;
}

bool test_stackPopAndFreeAll(void) {
  StackAlloc* a = stackAlloc_create(g_pao_buffer, I_stackAlloc_TEST_buffSize, 32);
  u8* x;
  u8* y;
  if (a == NULL) {
    return false;
  }
  x = (u8*)stackAlloc_alloc(a);
  y = (u8*)stackAlloc_alloc(a);
  if (x == NULL || y == NULL || y != x + stackAlloc_chunkSize(a)) {
    return false;
  }
  stackAlloc_pop(a);
  if (stackAlloc_used(a) != stackAlloc_chunkSize(a) || stackAlloc_alloc(a) != y) {
    return false;
  }
  stackAlloc_freeAll(a);
  return stackAlloc_empty(a) && stackAlloc_alloc(a) == x;
}

/* the interface: a size bigger than the chunk fails, free is in order, and
 * `info` reports what was used. */
bool test_stackInterface(void) {
  StackAlloc* a = stackAlloc_create(g_pao_buffer, I_stackAlloc_TEST_buffSize, 24);
  IAllocator mem;
  void* p;
  void* q;
  if (a == NULL) {
    return false;
  }
  mem = stackAlloc_createInterface(a);
  if (mem.alloc(mem.heap, stackAlloc_chunkSize(a) + 1, __func__) != NULL) {
    return false;
  }
  p = mem.alloc(mem.heap, 1, __func__);
  q = mem.alloc(mem.heap, stackAlloc_chunkSize(a), __func__);
  if (p == NULL || q == NULL || mem.info(mem.heap).used != 2 * stackAlloc_chunkSize(a)) {
    return false;
  }
  if (mem.info(mem.heap).total != stackAlloc_total(a)) {
    return false;
  }
  if (mem.free(mem.heap, q) != status_OK || mem.free(mem.heap, p) != status_OK) {
    return false;
  }
  p = mem.alloc(mem.heap, 1, __func__);
  return p != NULL && mem.freeAll(mem.heap) == status_OK && stackAlloc_empty(a);
}

static Tester tests[] = {
  {"test_stackCreate", test_stackCreate},
  {"test_stackCleanHeader", test_stackCleanHeader},
  {"test_stackSeq", test_stackSeq},
  {"test_stackContiguous", test_stackContiguous},
  {"test_stackFull", test_stackFull},
  {"test_stackPopAndFreeAll", test_stackPopAndFreeAll},
  {"test_stackInterface", test_stackInterface},
};

int main(void) {
  run_tests("stackAlloc", tests, LEN(tests));
  return 0;
}
