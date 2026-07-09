/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_stdAlloc_H
#define PAO_stdAlloc_H

#include <stdio.h>
#include <stdlib.h>
#include "../pao_allocator.h"

static inline
void* i_pao_stdAlloc_alloc(
  __attribute__((unused)) void* heap,
  usize size,
  __attribute__((unused)) const char* func
) {
  return malloc(size);
}

static inline
pao_Status i_pao_stdAlloc_free(__attribute__((unused)) void* heap, void* obj) {
  free(obj);
  return PAO_status_ok;
}

static inline
pao_Status i_pao_stdAlloc_freeAll(__attribute__((unused)) void* heap) {
  return PAO_status_failedFree;
}

static inline
pao_AllocatorInfo i_pao_stdAlloc_info(__attribute__((unused)) void* heap) {
  pao_AllocatorInfo out;
  out.total = 0;
  out.used = 0;
  return out;
}

static inline
pao_Allocator pao_stdAlloc_new(void) {
  pao_Allocator alloc = {
    .heap = NULL,
    .alloc = i_pao_stdAlloc_alloc,
    .free = i_pao_stdAlloc_free,
    .freeAll = i_pao_stdAlloc_freeAll,
    .info = i_pao_stdAlloc_info,
  };
  return alloc;
}

#endif
