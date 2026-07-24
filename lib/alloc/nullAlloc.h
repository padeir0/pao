/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/


/*
 * You may think this is unusual, but sometimes we don't want to allocate!
 * The whole library should accept this allocator and fail gracefully.
 * This means we can use a static amount of memory if we want :)
 * Which means: the library can be used in embedded devices.
 * (but i highly doubt the IAllocator interface would pass
 *  DO-178C and friends lmao)
 */


#ifndef PAO_NULLALLOC_H
#define PAO_NULLALLOC_H

#include <stdio.h>
#include "../iallocator.h"

static inline
void* i_nullAlloc_alloc(
  void* heap,
  usize size,
  const char* func
) {
  (void)heap; (void)size; (void)func;
  return NULL;
}

static inline
Status i_nullAlloc_free(
  void* heap,
  void* obj
) {
  (void)heap; (void)obj;
  return status_FAILEDFREE;
}

static inline
Status i_nullAlloc_freeAll(void* heap) {
  (void)heap;
  return status_FAILEDFREE;
}

static inline
AllocatorInfo i_nullAlloc_info(void* heap) {
  (void) heap;
  AllocatorInfo out;
  out.total = 0;
  out.used = 0;
  return out;
}

static inline
IAllocator nullAlloc_new(void) {
  IAllocator alloc = {
    .heap = NULL,
    .alloc = i_nullAlloc_alloc,
    .free = i_nullAlloc_free,
    .freeAll = i_nullAlloc_freeAll,
    .info = i_nullAlloc_info,
  };
  return alloc;
}

#endif
