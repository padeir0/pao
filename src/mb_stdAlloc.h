/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef MB_stdAlloc_H
#define MB_stdAlloc_H

#include "mb_allocator.h"
#include <stdio.h>

static inline
void* i_mb_stdAlloc_alloc(
  __attribute__((unused)) void* heap,
  usize size,
  __attribute__((unused)) char* func
) {
  return malloc(size);
}

static inline
void i_mb_stdAlloc_free(__attribute__((unused)) void* heap, void* obj) {
  free(obj);
}

static inline
void i_mb_stdAlloc_freeAll(__attribute__((unused)) void* heap) {
  printf("error: standard malloc provides no free_all function.\n");
  abort();
}

static inline
mb_AllocatorInfo i_mb_stdAlloc_info(__attribute__((unused)) void* heap) {
  mb_AllocatorInfo out;
  out.total = 0;
  out.used = 0;
  return out;
}

static inline
mb_Allocator mb_stdAlloc_new(void) {
  mb_Allocator _mb_stdAlloc = {
    .heap = NULL,
    .alloc = i_mb_stdAlloc_alloc,
    .free = i_mb_stdAlloc_free,
    .freeAll = i_mb_stdAlloc_freeAll,
    .info = i_mb_stdAlloc_info,
  };
  return _mb_stdAlloc;
}

#endif
