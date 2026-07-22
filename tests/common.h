#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../lib/status.h"
#include "../lib/colors.h"
#include "../lib/iallocator.h"
#include "../lib/alloc/flAlloc.h"

#ifndef PAO_TEST_COMMON_H

#define PAO_TEST_COMMON_H 1

#define DEFAULT_SIZE 2048

void checkStatus(Status ns) {
  if (ns != status_OK) {
    printf("fail: %d\n", ns);
    abort();
  }
}

typedef struct {
  char* name;
  bool (*func)(void);
} Tester;

static
char print_buff[DEFAULT_SIZE];

void run_tests(const char* name, Tester* tests, int length) {
  int successes = 0;
  int i = 0;
  while (i < length) {
    Tester t = tests[i];
    memset(print_buff, 0, DEFAULT_SIZE);
    bool ok = t.func();
    if (ok) {
      successes++;
    } else {
      strcat(print_buff, colors_RED "FAIL" colors_RESET ": ");
      strcat(print_buff, t.name);
      strcat(print_buff, "\n");
      printf("%s", print_buff);
    }
    i++;
  }
  printf("%s: %d/%d tests passed.\n", name, successes, length);
}

/* BEGIN: failing allocator
   An IAllocator, backed by a FLAlloc heap, that can be told to start
   returning NULL after a given number of successful allocations.
   This lets us simulate an out-of-memory allocator and check that
   natural functions propagate status_OUTOFMEMORY instead of
   crashing or corrupting already-allocated state.

   Being backed by FLAlloc rather than malloc/free is a deliberate
   upgrade over a bare malloc wrapper: `info().used`/`info().total`
   reflect real, precise heap accounting (so callers can assert on
   leaks), and `flAlloc_free` catches double-frees and out-of-bounds
   frees with a hard `abort()` instead of silently corrupting the
   heap or the process's real malloc arena.

   `allocsUntilFail == -1` means "never fail", which is handy for
   building up natural numbers before flipping the switch to 0 right
   before the call under test.

   NOTE: this is defined before the `alloc` macro below, since that
   macro would otherwise shadow the `IAllocator.alloc` field name
   used here.
*/
#define I_FAILALLOC_BUFFSIZE 65536

typedef struct {
  int allocsUntilFail;
  u8  buffer[I_FAILALLOC_BUFFSIZE];
  FLAlloc* fl;
} i_FailAllocHeap;

void* i_failAlloc_alloc(void* heap, usize size, const char* func) {
  (void)func;
  i_FailAllocHeap* h = (i_FailAllocHeap*)heap;
  if (h->allocsUntilFail == 0) {
    return NULL;
  }
  void* mem = flAlloc_alloc(h->fl, size);
  if (mem == NULL) {
    return NULL;
  }
  if (h->allocsUntilFail > 0) {
    h->allocsUntilFail--;
  }
  return mem;
}

Status i_failAlloc_free(void* heap, void* obj) {
  i_FailAllocHeap* h = (i_FailAllocHeap*)heap;
  flAlloc_free(h->fl, obj);
  return status_OK;
}

Status i_failAlloc_freeAll(void* heap) {
  i_FailAllocHeap* h = (i_FailAllocHeap*)heap;
  flAlloc_freeAll(h->fl);
  return status_OK;
}

AllocatorInfo i_failAlloc_info(void* heap) {
  i_FailAllocHeap* h = (i_FailAllocHeap*)heap;
  AllocatorInfo info;
  info.used = flAlloc_used(h->fl);
  info.total = flAlloc_total(h->fl);
  return info;
}

IAllocator i_failAlloc_new(i_FailAllocHeap* heap, int allocsUntilFail) {
  Status s;
  heap->allocsUntilFail = allocsUntilFail;
  s = flAlloc_create(sizeof(heap->buffer), heap->buffer);
  checkStatus(s);
  heap->fl = (FLAlloc*)heap->buffer;

  IAllocator a;
  a.heap = heap;
  a.alloc = i_failAlloc_alloc;
  a.free = i_failAlloc_free;
  a.freeAll = i_failAlloc_freeAll;
  a.info = i_failAlloc_info;
  return a;
}

#endif
