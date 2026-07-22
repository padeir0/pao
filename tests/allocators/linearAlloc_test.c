#include "../common.h"
#include "../../lib/alloc/linearAlloc.h"
#include <stdlib.h>

// Shared test state
#define I_linearAlloc_TEST_buffSize  1024
#define I_linearAlloc_TEST_objsSize  10

u8  g_pao_buffer[I_linearAlloc_TEST_buffSize];
u8* g_pao_objs[I_linearAlloc_TEST_objsSize];

/* BEGIN: helpers */
static void i_linearAllocTest_setbuff(u8* obj, usize size, u8 value) {
  usize i = 0;
  while (i < size) {
    obj[i] = value;
    i++;
  }
}

static bool i_linearAllocTest_checkbuff(u8* obj, usize size, u8 value) {
  usize i = 0;
  while (i < size) {
    if (obj[i] != value) {
      return false;
    }
    i++;
  }
  return true;
}

static LinearAlloc* i_linearAllocTest_make(void) {
  return linearAlloc_create(g_pao_buffer, I_linearAlloc_TEST_buffSize);
}
/* END: helpers */

/* BEGIN: tests */
// tests if linearAlloc_create validates its arguments correctly
bool test_create(void) {
  LinearAlloc* a;

  a = linearAlloc_create(NULL, I_linearAlloc_TEST_buffSize);
  if (a != NULL) {
    return false; /* NOTE(1) */
  }

  a = linearAlloc_create(g_pao_buffer, sizeof(LinearAlloc) - 1);
  if (a != NULL) {
    return false; /* NOTE(2) */
  }

  a = i_linearAllocTest_make();
  if (a == NULL) {
    return false;
  }

  return true;
  /*
   * NOTE(1): null buffer must be rejected.
   * NOTE(2): buffer that can't even hold the LinearAlloc header must
   *          be rejected.
   */
}

// allocate growing sizes, fill with a sentinel, verify
bool test_allocFill(void) {
  LinearAlloc* a = i_linearAllocTest_make();
  usize alloc_size;
  u32 i;

  if (a == NULL) {
    return false;
  }

  i = 0;
  while (i < I_linearAlloc_TEST_objsSize) {
    alloc_size = (i+1) * 10;
    g_pao_objs[i] = (u8*)linearAlloc_alloc(a, alloc_size);
    if (g_pao_objs[i] == NULL) {
      return false;
    }
    // SAFE(1):
    i_linearAllocTest_setbuff(g_pao_objs[i], alloc_size, (u8)i);
    i++;
  }

  i = 0;
  while (i < I_linearAlloc_TEST_objsSize) {
    alloc_size = (i+1) * 10;
    if (!i_linearAllocTest_checkbuff(g_pao_objs[i], alloc_size, (u8)i)) {
      return false; /* NOTE(1) */
    }
    i++;
  }

  return true;
  /*
   * NOTE(1): allocations must not overlap; each block must hold
   *          exactly the sentinel written to it.
   * SAFE(1): I_linearAlloc_TEST_objsSize <= 10 <= UINT8_MAX
   */
}

// an allocation that doesn't fit must return NULL without
// corrupting the allocator's state
bool test_outOfMemory(void) {
  LinearAlloc* a = i_linearAllocTest_make();
  usize total;
  usize before;

  if (a == NULL) {
    return false;
  }

  total = linearAlloc_total(a);
  if (linearAlloc_alloc(a, total) == NULL) {
    return false; /* fits exactly once, must succeed */
  }

  before = linearAlloc_used(a);
  if (linearAlloc_alloc(a, 1) != NULL) {
    return false; /* NOTE(1) */
  }

  if (linearAlloc_used(a) != before) {
    return false; /* NOTE(2) */
  }

  return true;
  /*
   * NOTE(1): the arena is exhausted, so this allocation must fail.
   * NOTE(2): a failed allocation must not bump `allocated`.
   */
}

// tests if freeAll resets the arena fully
bool test_freeAll(void) {
  LinearAlloc* a = i_linearAllocTest_make();
  usize total;

  if (a == NULL) {
    return false;
  }

  total = linearAlloc_available(a);

  if (linearAlloc_alloc(a, total) == NULL) {
    return false;
  }

  if (!linearAlloc_empty(a) && linearAlloc_available(a) != 0) {
    return false; /* NOTE(1) */
  }

  linearAlloc_freeAll(a);

  if (!linearAlloc_empty(a)) {
    return false; /* NOTE(2) */
  }

  if (linearAlloc_available(a) != total) {
    return false; /* NOTE(3) */
  }

  return true;
  /*
   * NOTE(1): after draining the arena, no memory should remain.
   * NOTE(2): after freeAll the arena must report empty.
   * NOTE(3): available bytes must be restored to the original capacity.
   */
}

// tests if used + available == total capacity
bool test_usedAndAvailable(void) {
  LinearAlloc* a = i_linearAllocTest_make();
  usize total;
  u32 i;

  if (a == NULL) {
    return false;
  }

  total = linearAlloc_total(a);
  if (linearAlloc_used(a) != 0) {
    return false;
  }

  i = 0;
  while (i < I_linearAlloc_TEST_objsSize) {
    if (linearAlloc_alloc(a, 10) == NULL) {
      return false;
    }

    if (linearAlloc_used(a) + linearAlloc_available(a) != total) {
      return false; /* NOTE(1) */
    }

    if (linearAlloc_used(a) != (i + 1) * 10) {
      return false;
    }

    i++;
  }

  return true;
  /*
   * NOTE(1): used + available must always equal the arena's total
   *          capacity, regardless of how much has been allocated.
   */
}

// individual objects can't be freed; the IAllocator interface must
// report that plainly instead of silently doing nothing.
bool test_individualFreeFails(void) {
  LinearAlloc* a = i_linearAllocTest_make();
  IAllocator iface;
  void* obj;
  Status s;

  if (a == NULL) {
    return false;
  }

  iface = linearAlloc_createInterface(a);
  obj = iface.alloc(iface.heap, 16, (const char*)__func__);
  if (obj == NULL) {
    return false;
  }

  s = iface.free(iface.heap, obj);
  return s == status_FAILEDFREE && linearAlloc_used(a) == 16;
}
/* END: tests */

/* BEGIN: DRIVER CODE */
Tester tests[] = {
  {"test_linearAlloc_create",              test_create},
  {"test_linearAlloc_allocFill",            test_allocFill},
  {"test_linearAlloc_outOfMemory",          test_outOfMemory},
  {"test_linearAlloc_freeAll",              test_freeAll},
  {"test_linearAlloc_usedAndAvailable",     test_usedAndAvailable},
  {"test_linearAlloc_individualFreeFails",  test_individualFreeFails},
};

int main(void) {
  run_tests("linearAlloc", tests, (int)(sizeof(tests) / sizeof(tests[0])));
}
/* END: DRIVER CODE */
