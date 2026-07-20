/*
MIT License
Copyright 2026 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#include "../common.h"
#include "../../lib/alloc/pool.h"
#include <stdlib.h>

// Shared test state
#define I_pool_TEST_buffSize  4096
#define I_pool_TEST_objsSize  64

u8  g_pao_buffer[I_pool_TEST_buffSize];
u8* g_pao_objs[I_pool_TEST_objsSize];

/* BEGIN: helpers */
static void i_poolTest_setbuff(u8* obj, usize size, u8 value) {
  usize i = 0;
  while (i < size) {
    obj[i] = value;
    i++;
  }
}

static bool i_poolTest_checkbuff(u8* obj, usize size, u8 value) {
  usize i = 0;
  while (i < size) {
    if (obj[i] != value) {
      return false;
    }
    i++;
  }
  return true;
}

static int i_poolTest_findNull(void) {
  int i = 0;
  while (i < I_pool_TEST_objsSize) {
    if (g_pao_objs[i] == NULL) {
      return i;
    }
    i++;
  }
  return -1;
}

static int i_poolTest_findNonNull(void) {
  int i = 0;
  while (i < I_pool_TEST_objsSize) {
    if (g_pao_objs[i] != NULL) {
      return i;
    }
    i++;
  }
  return -1;
}

static Pool* i_poolTest_make(usize chunksize) {
  Status s = pool_new(I_pool_TEST_buffSize, chunksize, g_pao_buffer);
  if (s != status_OK) {
    return NULL;
  }
  return (Pool*)g_pao_buffer;
}

static int i_poolTest_allocAll(Pool* p) {
  u8* obj;
  u8 i = 0;
  while (i < I_pool_TEST_objsSize) {
    obj = (u8*)pool_alloc(p);
    if (obj == NULL) {
      break; /* pool exhausted before array; that's fine */
    }
    i_poolTest_setbuff(obj, p->chunkSize, i);
    g_pao_objs[i] = obj;
    i++;
  }
  return (int)i;
}

/* verifies each slot is intact */
static bool i_poolTest_verifyObjs(Pool* p, int len) {
  u8 j = 0;
  while (j < len) {
    if (g_pao_objs[j] == NULL) {
      return false;
    }
    if (!i_poolTest_checkbuff(g_pao_objs[j], p->chunkSize, j)) {
      return false; // NOTE(1)
    }
    j++;
  }
  return true;
  /*
   * NOTE(1): allocations must not overlap; each slot must hold exactly
   *          the sentinel written to it.
   */
}


/* END: helpers */

/* BEGIN: tests */
// tests if pool_new validates its arguments correctly
bool test_new(void) {
  Status s;

  s = pool_new(I_pool_TEST_buffSize, sizeof(i_pool_Node), NULL);
  if (s != status_NULLBUFFER) {
    return false; /* NOTE(1) */
  }

  s = pool_new(I_pool_TEST_buffSize, pool_MINCHUNKSIZE - 1, g_pao_buffer);
  if (s != status_BADSIZE) {
    return false; /* NOTE(2) */
  }

  s = pool_new(sizeof(Pool), pool_MINCHUNKSIZE, g_pao_buffer);
  if (s != status_BUFFERTOOSMALL) {
    return false; /* NOTE(3) */
  }

  s = pool_new(I_pool_TEST_buffSize, pool_MINCHUNKSIZE, g_pao_buffer);
  if (s != status_OK) {
    return false;
  }

  return true;
  /*
   * NOTE(1): null buffer must be rejected.
   * NOTE(2): chunksize below the minimum (one node-pointer) must be rejected.
   * NOTE(3): buffer that can't hold Pool header + one chunk must be rejected.
   */
}

/*
bool test_doubleFree(void) {
  usize chunksize = 32;
  Pool* p = i_poolTest_make(chunksize);

  if (p == NULL) {
    return false;
  }

  usize initial = pool_available(p);
  if (initial == 0) {
    return false;
  }

  u8* obj = (u8*)pool_alloc(p);
  pool_free(p, obj);
  pool_free(p, obj);
  // verifies if the pool can still function after a double free
  int len = i_poolTest_allocAll(p);
  return i_poolTest_verifyObjs(p, len);
}
*/

// allocate every slot, fill with a sentinel, verify
bool test_allocFill(void) {
  usize chunksize = 32;
  Pool* p = i_poolTest_make(chunksize);
  usize initial;

  if (p == NULL) {
    return false;
  }

  initial = pool_available(p);
  if (initial == 0) {
    return false;
  }

  int len = i_poolTest_allocAll(p);
  return i_poolTest_verifyObjs(p, len);;
}

// Free all slots, then alloc again. Pool must
// hand out the same memory range without error.
bool test_freeAndReuse(void) {
  usize chunksize = I_pool_TEST_buffSize / I_pool_TEST_objsSize;
  Pool* p = i_poolTest_make(chunksize);
  usize slots;
  usize i;

  if (p == NULL) {
    return false;
  }

  slots = pool_available(p) / chunksize;
  if (slots == 0 || slots > I_pool_TEST_objsSize) {
    return false;
  }

  /* fill the pool */
  i = 0;
  while (i < slots) {
    g_pao_objs[i] = (u8*)pool_alloc(p);
    if (g_pao_objs[i] == NULL) {
      return false;
    }
    i++;
  }

  if (pool_alloc(p) != NULL) {
    return false; /* NOTE(1) */
  }

  /* free all */
  i = 0;
  while (i < slots) {
    pool_free(p, g_pao_objs[i]);
    g_pao_objs[i] = NULL;
    i++;
  }

  if (!pool_empty(p)) {
    return false; /* NOTE(2) */
  }

  /* alloc again — must succeed for the full slot count */
  i = 0;
  while (i < slots) {
    g_pao_objs[i] = (u8*)pool_alloc(p);
    if (g_pao_objs[i] == NULL) {
      return false;
    }
    i++;
  }

  return true;
  /*
   * NOTE(1): pool must return NULL when exhausted, not crash.
   * NOTE(2): after freeing every slot the pool must report empty.
   */
}

// tests if pool_freeAll resets the pool fully
bool test_freeAll(void) {
  usize chunksize = I_pool_TEST_buffSize / I_pool_TEST_objsSize;
  Pool* p = i_poolTest_make(chunksize);
  usize initial;
  usize slots;
  usize i;

  if (p == NULL) {
    return false;
  }

  initial = pool_available(p);
  slots = initial / chunksize;
  if (slots == 0 || slots > I_pool_TEST_objsSize) {
    return false;
  }

  /* drain the pool */
  i = 0;
  while (i < slots) {
    if (pool_alloc(p) == NULL) {
      return false;
    }
    i++;
  }

  if (pool_available(p) != 0) {
    return false;
  }

  pool_freeAll(p);

  if (!pool_empty(p)) {
    return false; /* NOTE(1) */
  }

  if (pool_available(p) != initial) {
    return false; /* NOTE(2) */
  }

  return true;
  /*
   * NOTE(1): after freeAll the pool must report empty (all slots free).
   * NOTE(2): available bytes must be restored to the original capacity.
   */
}

// tests if pool_free rejects out-of-bounds and misaligned pointers
/*
bool test_freeErrors(void) {
  usize chunksize = 32;
  Pool* p = i_poolTest_make(chunksize);
  u8 external[128];
  u8* obj;

  if (p == NULL) {
    return false;
  }

  // pointer outside the pool entirely
  pool_free(p, &external);

  // pointer inside the pool but not aligned to a chunk boundary
  obj = (u8*)pool_alloc(p);
  if (obj == NULL) {
    return false;
  }

  pool_free(p, obj + 1);

  // the valid pointer itself must still free correctly
  pool_free(p, obj);

  return true;
  //   NOTE(1): obj+1 is inside the pool's memory range but is not
  //          aligned to a chunk boundary, so badAlignment is expected.
}
*/

// tests if used + available == total capacity
bool test_usedAndAvailable(void) {
  usize chunksize = I_pool_TEST_buffSize / I_pool_TEST_objsSize;
  Pool* p = i_poolTest_make(chunksize);
  usize total;
  usize slots;
  usize i;

  if (p == NULL) {
    return false;
  }

  total = pool_available(p);
  slots = total / chunksize;
  if (slots == 0 || slots > I_pool_TEST_objsSize) {
    return false;
  }

  if (pool_used(p) != 0) {
    return false;
  }

  /* alloc one at a time and check accounting at each step */
  i = 0;
  while (i < slots) {
    g_pao_objs[i] = (u8*)pool_alloc(p);
    if (g_pao_objs[i] == NULL) {
      return false;
    }

    if (pool_used(p) + pool_available(p) != total) {
      return false; /* NOTE(1) */
    }

    if (pool_used(p) != (i + 1) * chunksize) {
      return false;
    }

    i++;
  }

  return true;
  /*
   * NOTE(1): used + available must always equal the pool's total
   *          capacity, regardless of how many slots are in use.
   */
}

// random interleaving of alloc and free preserves memory integrity
static void i_poolTest_mixedAlloc(Pool* p, usize chunksize) {
  int slot;
  u8* obj;

  obj = (u8*)pool_alloc(p);
  if (obj == NULL) {
    return; /* pool exhausted; nothing to do */
  }

  slot = i_poolTest_findNull();
  if (slot < 0) {
    /* tracking array is full; return the chunk to the pool */
    pool_free(p, obj);
    return;
  }

  // SAFE(1):
  i_poolTest_setbuff(obj, chunksize, (u8)slot);
  g_pao_objs[slot] = obj;
  /* SAFE(1): slot is in [0, I_pool_TEST_objsSize), cast to u8 is safe
   * because I_pool_TEST_objsSize <= 64 <= UINT8_MAX
   */
}

static bool i_poolTest_mixedFree(Pool* p, usize chunksize) {
  int slot;
  u8* obj;

  slot = i_poolTest_findNonNull();
  if (slot < 0) {
    return true; /* nothing allocated yet */
  }

  obj = g_pao_objs[slot];

  // SAFE(1):
  if (!i_poolTest_checkbuff(obj, chunksize, (u8)slot)) {
    return false; /* NOTE(1) */
  }

  pool_free(p, obj);

  g_pao_objs[slot] = NULL;
  return true;
  /*
   * NOTE(1): the sentinel written at alloc time must be unmodified at
   *          free time; corruption means the pool handed out overlapping
   *          chunks.
   * SAFE(1): slot is in [0, I_pool_TEST_objsSize) <= UINT8_MAX
   */
}

bool test_mixed(void) {
  usize chunksize = 32;
  Pool* p;
  int iterations;
  bool ok;
  int i;

  /* zero the tracking array */
  i = 0;
  while (i < I_pool_TEST_objsSize) {
    g_pao_objs[i] = NULL;
    i++;
  }

  p = i_poolTest_make(chunksize);
  if (p == NULL) {
    return false;
  }

  srand(42); /* deterministic seed for reproducibility */
  iterations = 5000;
  ok = true;

  i = 0;
  while (i < iterations && ok) {
    if (rand() % 2 == 0) {
      i_poolTest_mixedAlloc(p, chunksize);
    } else {
      ok = i_poolTest_mixedFree(p, chunksize);
    }
    i++;
  }

  if (!ok) {
    return false;
  }

  /* drain remaining live allocations */
  i = 0;
  while (i < I_pool_TEST_objsSize) {
    if (g_pao_objs[i] != NULL) {
      pool_free(p, g_pao_objs[i]);
      g_pao_objs[i] = NULL;
    }
    i++;
  }

  if (!pool_empty(p)) {
    return false;
  }

  return true;
}

// run seq alloc/free for a range of sizes
bool test_multipleChunksizes(void) {
  usize chunksize;
  Pool* p;
  usize slots;
  usize i;

  chunksize = pool_MINCHUNKSIZE;
  while (chunksize <= 128) {
    p = i_poolTest_make(chunksize);
    if (p == NULL) {
      return false;
    }

    slots = pool_available(p) / chunksize;
    if (slots > I_pool_TEST_objsSize) {
      slots = I_pool_TEST_objsSize;
    }

    i = 0;
    while (i < slots) {
      g_pao_objs[i] = (u8*)pool_alloc(p);
      if (g_pao_objs[i] == NULL) {
        return false;
      }
      /* SAFE: i <= slots <= I_pool_TEST_objsSize <= 64 <= UINT8_MAX */
      i_poolTest_setbuff(g_pao_objs[i], chunksize, (u8)i);
      i++;
    }

    i = 0;
    while (i < slots) {
      if (!i_poolTest_checkbuff(g_pao_objs[i], chunksize, (u8)i)) {
        return false;
      }
      pool_free(p, g_pao_objs[i]);
      g_pao_objs[i] = NULL;
      i++;
    }

    if (!pool_empty(p)) {
      return false;
    }

    chunksize++;
  }

  return true;
}
/* END: tests */

/* BEGIN: DRIVER CODE */
Tester tests[] = {
  {"test_pool_new",               test_new},
  {"test_pool_allocFill",         test_allocFill},
  {"test_pool_freeAndReuse",      test_freeAndReuse},
  {"test_pool_freeAll",           test_freeAll},
  // {"test_pool_freeErrors",        test_freeErrors},
  // {"test_pool_doubleFree",        test_doubleFree},
  {"test_pool_usedAndAvailable",  test_usedAndAvailable},
  {"test_pool_mixed",             test_mixed},
  {"test_pool_multipleChunksizes",test_multipleChunksizes},
};

int main(void) {
  run_tests("pool", tests, (int)(sizeof(tests) / sizeof(tests[0])));
}
/* END: DRIVER CODE */
