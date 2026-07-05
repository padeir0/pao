/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_stdAlloc_H
#define PAO_stdAlloc_H

#include "../pao_allocator.h"
#include <stdio.h>

static inline
void* i_pao_stdAlloc_alloc(
  __attribute__((unused)) void* heap,
  usize size,
  __attribute__((unused)) char* func
) {
  return malloc(size);
}

static inline
void i_pao_stdAlloc_free(__attribute__((unused)) void* heap, void* obj) {
  free(obj);
}

static inline
void i_pao_stdAlloc_freeAll(__attribute__((unused)) void* heap) {
  printf("error: standard malloc provides no free_all function.\n");
  abort();
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
  pao_Allocator _pao_stdAlloc = {
    .heap = NULL,
    .alloc = i_pao_stdAlloc_alloc,
    .free = i_pao_stdAlloc_free,
    .freeAll = i_pao_stdAlloc_freeAll,
    .info = i_pao_stdAlloc_info,
  };
  return _pao_stdAlloc;
}

#endif
