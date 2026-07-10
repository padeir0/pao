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
 * (but i highly doubt the pao_Allocator interface would pass
 *  DO-178C and friends lmao)
 */


#ifndef PAO_nullAlloc_H
#define PAO_nullAlloc_H

#include <stdio.h>
#include "../pao_allocator.h"

static inline
void* i_pao_nullAlloc_alloc(
  __attribute__((unused)) void* heap,
  __attribute__((unused)) usize size,
  __attribute__((unused)) const char* func
) {
  return NULL;
}

static inline
pao_Status i_pao_nullAlloc_free(
  __attribute__((unused)) void* heap,
  __attribute__((unused)) void* obj
) {
  return PAO_status_failedFree;
}

static inline
pao_Status i_pao_nullAlloc_freeAll(__attribute__((unused)) void* heap) {
  return PAO_status_failedFree;
}

static inline
pao_AllocatorInfo i_pao_nullAlloc_info(__attribute__((unused)) void* heap) {
  pao_AllocatorInfo out;
  out.total = 0;
  out.used = 0;
  return out;
}

static inline
pao_Allocator pao_nullAlloc_new(void) {
  pao_Allocator alloc = {
    .heap = NULL,
    .alloc = i_pao_nullAlloc_alloc,
    .free = i_pao_nullAlloc_free,
    .freeAll = i_pao_nullAlloc_freeAll,
    .info = i_pao_nullAlloc_info,
  };
  return alloc;
}

#endif
