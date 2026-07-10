/*
MIT License
Copyright 2026 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#include "../common.h"
#include "../../lib/alloc/pao_flAlloc.h"
#include <stdlib.h>

// Shared test state
#define I_PAO_FLALLOC_TEST_buffSize  8192
#define I_PAO_FLALLOC_TEST_objsSize  64

u8    g_pao_buffer[I_PAO_FLALLOC_TEST_buffSize];
u8*   g_pao_objs[I_PAO_FLALLOC_TEST_objsSize];
usize g_pao_objSizes[I_PAO_FLALLOC_TEST_objsSize];

/* BEGIN: helpers */
static void i_pao_flAllocTest_setbuff(u8* obj, usize size, u8 value) {
  usize i = 0;
  while (i < size) {
    obj[i] = value;
    i++;
  }
}

static bool i_pao_flAllocTest_checkbuff(const u8* obj, usize size, u8 value) {
  usize i = 0;
  while (i < size) {
    if (obj[i] != value) {
      return false;
    }
    i++;
  }
  return true;
}

static int i_pao_flAllocTest_findNull(void) {
  int i = 0;
  while (i < I_PAO_FLALLOC_TEST_objsSize) {
    if (g_pao_objs[i] == NULL) {
      return i;
    }
    i++;
  }
  return -1;
}

static int i_pao_flAllocTest_findNonNull(void) {
  int i = 0;
  while (i < I_PAO_FLALLOC_TEST_objsSize) {
    if (g_pao_objs[i] != NULL) {
      return i;
    }
    i++;
  }
  return -1;
}

static pao_flAlloc* i_pao_flAllocTest_make(void) {
  pao_Status s = pao_flAlloc_create(I_PAO_FLALLOC_TEST_buffSize, g_pao_buffer);
  if (s != PAO_status_ok) {
    return NULL;
  }
  return (pao_flAlloc*)g_pao_buffer;
}

/* verifies each tracked slot still holds exactly the sentinel it was
 * given, and that the heap recorded enough space to cover what was
 * requested
 */
static bool i_pao_flAllocTest_verifyObjs(int len) {
  u8 j = 0;
  while (j < len) {
    if (g_pao_objs[j] == NULL) {
      return false;
    }
    if (!i_pao_flAllocTest_checkbuff(g_pao_objs[j], g_pao_objSizes[j], j)) {
      return false; // NOTE(1)
    }
    if (pao_flAlloc_objsize(g_pao_objs[j]) < g_pao_objSizes[j]) {
      return false; // NOTE(2)
    }
    j++;
  }
  return true;
  /*
   * NOTE(1): allocations must not overlap; each slot must hold exactly
   *          the sentinel written to it.
   * NOTE(2): the header must record an object size at least as large as
   *          what was requested, even after internal padding.
   */
}
/* END: helpers */

/* BEGIN: tests */
// tests if pao_flAlloc_create validates its arguments correctly
bool test_new(void) {
  pao_Status s;

  s = pao_flAlloc_create(1024, NULL);
  if (s != PAO_status_nullBuffer) {
    return false;
  }

  s = pao_flAlloc_create(sizeof(pao_flAlloc), g_pao_buffer);
  if (s != PAO_status_bufferTooSmall) {
    return false; /* NOTE(1) */
  }

  s = pao_flAlloc_create(I_PAO_FLALLOC_TEST_buffSize, g_pao_buffer);
  if (s != PAO_status_ok) {
    return false;
  }

  return true;
  /*
   * NOTE(1): a buffer that can't hold the pao_flAlloc header plus one
   *          free-list node must be rejected.
   */
}

// allocate slots of varying sizes, fill each with a sentinel, verify
bool test_allocFill(void) {
  pao_flAlloc* fl = i_pao_flAllocTest_make();
  u8* obj;
  usize size;
  int i = 0;

  if (fl == NULL) {
    return false;
  }

  while (i < I_PAO_FLALLOC_TEST_objsSize) {
    size = (usize)((i % 16) + 1) * 8;
    obj = (u8*)pao_flAlloc_alloc(fl, size);
    if (obj == NULL) {
      break; /* heap exhausted before array; that's fine */
    }
    i_pao_flAllocTest_setbuff(obj, size, (u8)i);
    g_pao_objs[i] = obj;
    g_pao_objSizes[i] = size;
    i++;
  }

  return i_pao_flAllocTest_verifyObjs(i);
}

// free everything in ascending address order, then alloc the same
// pattern again; the heap must return to full capacity and serve it
bool test_freeAndReuse(void) {
  pao_flAlloc* fl = i_pao_flAllocTest_make();
  usize objSize = 48;
  usize initial;
  int slots = 0;
  int i;

  if (fl == NULL) {
    return false;
  }

  initial = pao_flAlloc_available(fl);

  while (slots < I_PAO_FLALLOC_TEST_objsSize) {
    g_pao_objs[slots] = (u8*)pao_flAlloc_alloc(fl, objSize);
    if (g_pao_objs[slots] == NULL) {
      break; /* heap exhausted before array; that's fine */
    }
    slots++;
  }

  if (slots == 0) {
    return false;
  }

  i = 0;
  while (i < slots) {
    pao_flAlloc_free(fl, g_pao_objs[i]);
    g_pao_objs[i] = NULL;
    i++;
  }

  if (!pao_flAlloc_empty(fl)) {
    return false; /* NOTE(1) */
  }

  if (pao_flAlloc_available(fl) != initial) {
    return false; /* NOTE(2) */
  }

  i = 0;
  while (i < slots) {
    g_pao_objs[i] = (u8*)pao_flAlloc_alloc(fl, objSize);
    if (g_pao_objs[i] == NULL) {
      return false; /* NOTE(3) */
    }
    i++;
  }

  return true;
  /*
   * NOTE(1): after freeing every block in ascending address order the
   *          heap must report empty.
   * NOTE(2): available bytes must be restored to the original capacity.
   * NOTE(3): once fully freed, the heap must serve the same allocation
   *          pattern again without loss of capacity.
   */
}

// frees three adjacent blocks out of address order (ends, then middle)
// and checks the heap fully recombines them into one free run
bool test_coalesce(void) {
  pao_flAlloc* fl = i_pao_flAllocTest_make();
  usize objSize = 64;
  usize initial;
  u8* a;
  u8* b;
  u8* c;

  if (fl == NULL) {
    return false;
  }

  initial = pao_flAlloc_available(fl);

  a = (u8*)pao_flAlloc_alloc(fl, objSize);
  b = (u8*)pao_flAlloc_alloc(fl, objSize);
  c = (u8*)pao_flAlloc_alloc(fl, objSize);
  if (a == NULL || b == NULL || c == NULL) {
    return false;
  }

  pao_flAlloc_free(fl, a);
  pao_flAlloc_free(fl, c);
  pao_flAlloc_free(fl, b);

  if (pao_flAlloc_available(fl) != initial) {
    return false; /* NOTE(1) */
  }

  return pao_flAlloc_empty(fl);
  /*
   * NOTE(1): once every block carved out of a single contiguous region
   *          is freed, that region must be fully reassembled into one
   *          free run, regardless of the order the blocks were freed
   *          in (here: low end, high end, then the middle).
   */
}

// tests if pao_flAlloc_freeAll resets the heap fully
bool test_freeAll(void) {
  pao_flAlloc* fl = i_pao_flAllocTest_make();
  usize initial;
  u8* obj;

  if (fl == NULL) {
    return false;
  }

  initial = pao_flAlloc_available(fl);
  if (initial == 0) {
    return false;
  }

  obj = (u8*)pao_flAlloc_alloc(fl, 128);
  if (obj == NULL) {
    return false;
  }

  if (pao_flAlloc_available(fl) == initial) {
    return false; /* NOTE(1) */
  }

  pao_flAlloc_freeAll(fl);

  if (!pao_flAlloc_empty(fl)) {
    return false; /* NOTE(2) */
  }

  if (pao_flAlloc_available(fl) != initial) {
    return false; /* NOTE(3) */
  }

  return true;
  /*
   * NOTE(1): allocating must actually shrink the reported available
   *          space.
   * NOTE(2): after freeAll the heap must report empty.
   * NOTE(3): available bytes must be restored to the original capacity.
   */
}

// tests if used + available == total capacity as allocations proceed
bool test_usedAndAvailable(void) {
  pao_flAlloc* fl = i_pao_flAllocTest_make();
  usize objSize = 32;
  usize total;
  usize usedBefore;
  u8* obj;
  int i;

  if (fl == NULL) {
    return false;
  }

  total = pao_flAlloc_available(fl);

  if (pao_flAlloc_used(fl) != 0) {
    return false;
  }

  i = 0;
  while (i < 8) {
    usedBefore = pao_flAlloc_used(fl);
    obj = (u8*)pao_flAlloc_alloc(fl, objSize);
    if (obj == NULL) {
      return false;
    }

    if (pao_flAlloc_used(fl) + pao_flAlloc_available(fl) != total) {
      return false; /* NOTE(1) */
    }

    if (pao_flAlloc_used(fl) <= usedBefore) {
      return false; /* NOTE(2) */
    }

    i++;
  }

  return true;
  /*
   * NOTE(1): used + available must always equal the heap's total
   *          capacity.
   * NOTE(2): each successful allocation must strictly increase the
   *          amount reported as used.
   */
}

// tests if pao_flAlloc_objsize reports at least the requested size
bool test_objsize(void) {
  pao_flAlloc* fl = i_pao_flAllocTest_make();
  u8* obj;
  usize requested = 40;

  if (fl == NULL) {
    return false;
  }

  obj = (u8*)pao_flAlloc_alloc(fl, requested);
  if (obj == NULL) {
    return false;
  }

  if (pao_flAlloc_objsize(obj) < requested) {
    return false; /* NOTE(1) */
  }

  return true;
  /*
   * NOTE(1): the header must record enough space to cover what the
   *          caller asked for, even after alignment/padding.
   */
}

// random interleaving of variable-size alloc and free preserves
// memory integrity
static void i_pao_flAllocTest_mixedAlloc(pao_flAlloc* fl) {
  int slot;
  u8* obj;
  usize size;

  size = ((usize)(rand() % 96)) + 1;

  obj = (u8*)pao_flAlloc_alloc(fl, size);
  if (obj == NULL) {
    return; /* heap exhausted; nothing to do */
  }

  slot = i_pao_flAllocTest_findNull();
  if (slot < 0) {
    /* tracking array is full; return the block to the heap */
    pao_flAlloc_free(fl, obj);
    return;
  }

  // SAFE(1):
  i_pao_flAllocTest_setbuff(obj, size, (u8)slot);
  g_pao_objs[slot] = obj;
  g_pao_objSizes[slot] = size;
  /* SAFE(1): slot is in [0, I_PAO_FLALLOC_TEST_objsSize), cast to u8 is
   * safe because I_PAO_FLALLOC_TEST_objsSize <= 64 <= UINT8_MAX
   */
}

static bool i_pao_flAllocTest_mixedFree(pao_flAlloc* fl) {
  int slot;
  u8* obj;

  slot = i_pao_flAllocTest_findNonNull();
  if (slot < 0) {
    return true; /* nothing allocated yet */
  }

  obj = g_pao_objs[slot];

  // SAFE(1):
  if (!i_pao_flAllocTest_checkbuff(obj, g_pao_objSizes[slot], (u8)slot)) {
    return false; /* NOTE(1) */
  }

  pao_flAlloc_free(fl, obj);

  g_pao_objs[slot] = NULL;
  return true;
  /*
   * NOTE(1): the sentinel written at alloc time must be unmodified at
   *          free time; corruption means the heap handed out
   *          overlapping blocks or its free list is broken.
   * SAFE(1): slot is in [0, I_PAO_FLALLOC_TEST_objsSize) <= UINT8_MAX
   */
}

bool test_mixed(void) {
  pao_flAlloc* fl;
  int iterations;
  bool ok;
  int i;

  i = 0;
  while (i < I_PAO_FLALLOC_TEST_objsSize) {
    g_pao_objs[i] = NULL;
    g_pao_objSizes[i] = 0;
    i++;
  }

  fl = i_pao_flAllocTest_make();
  if (fl == NULL) {
    return false;
  }

  srand(42); /* deterministic seed for reproducibility */
  iterations = 5000;
  ok = true;

  i = 0;
  while (i < iterations && ok) {
    if (rand() % 2 == 0) {
      i_pao_flAllocTest_mixedAlloc(fl);
    } else {
      ok = i_pao_flAllocTest_mixedFree(fl);
    }
    i++;
  }

  if (!ok) {
    return false;
  }

  /* drain remaining live allocations */
  i = 0;
  while (i < I_PAO_FLALLOC_TEST_objsSize) {
    if (g_pao_objs[i] != NULL) {
      pao_flAlloc_free(fl, g_pao_objs[i]);
      g_pao_objs[i] = NULL;
    }
    i++;
  }

  if (!pao_flAlloc_empty(fl)) {
    return false;
  }

  return true;
}

// sequential alloc/free for a range of object sizes; exercises the
// internal size-padding logic across many alignment boundaries
bool test_multipleSizes(void) {
  usize size;
  pao_flAlloc* fl;
  int i;

  size = 1;
  while (size <= 256) {
    fl = i_pao_flAllocTest_make();
    if (fl == NULL) {
      return false;
    }

    i = 0;
    while (i < 8) {
      g_pao_objs[i] = (u8*)pao_flAlloc_alloc(fl, size);
      if (g_pao_objs[i] == NULL) {
        return false;
      }
      /* SAFE: i < 8 <= UINT8_MAX */
      i_pao_flAllocTest_setbuff(g_pao_objs[i], size, (u8)i);
      i++;
    }

    i = 0;
    while (i < 8) {
      if (!i_pao_flAllocTest_checkbuff(g_pao_objs[i], size, (u8)i)) {
        return false;
      }
      pao_flAlloc_free(fl, g_pao_objs[i]);
      g_pao_objs[i] = NULL;
      i++;
    }

    if (!pao_flAlloc_empty(fl)) {
      return false;
    }

    size++;
  }

  return true;
}

// tests if pao_flAlloc_free rejects an out-of-bounds pointer
//
// left disabled: the PAO_config_debug bounds check calls
// PAO_debug_fatalFmt, which aborts the process, so it can't be
// exercised from inside this same test run
/*
bool test_freeErrors(void) {
  pao_flAlloc* fl = i_pao_flAllocTest_make();
  u8 external[128];

  if (fl == NULL) {
    return false;
  }

  pao_flAlloc_free(fl, &external);

  return true;
}
*/
/* END: tests */

/* BEGIN: DRIVER CODE */
Tester tests[] = {
  {"test_flAlloc_new",              test_new},
  {"test_flAlloc_allocFill",        test_allocFill},
  {"test_flAlloc_freeAndReuse",     test_freeAndReuse},
  {"test_flAlloc_coalesce",         test_coalesce},
  {"test_flAlloc_freeAll",          test_freeAll},
  {"test_flAlloc_usedAndAvailable", test_usedAndAvailable},
  {"test_flAlloc_objsize",          test_objsize},
  {"test_flAlloc_mixed",            test_mixed},
  {"test_flAlloc_multipleSizes",    test_multipleSizes},
  // {"test_flAlloc_freeErrors",       test_freeErrors},
};

int main(void) {
  run_tests(tests, (int)(sizeof(tests) / sizeof(tests[0])));
}
/* END: DRIVER CODE */
