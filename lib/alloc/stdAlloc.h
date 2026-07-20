/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_STDALLOC_H
#define PAO_STDALLOC_H

#include <stdio.h>
#include <stdlib.h>
#include "../iallocator.h"

static inline
void* i_stdAlloc_alloc(
  __attribute__((unused)) void* heap,
  usize size,
  __attribute__((unused)) const char* func
) {
  return malloc(size);
}

static inline
Status i_stdAlloc_free(__attribute__((unused)) void* heap, void* obj) {
  free(obj);
  return status_OK;
}

static inline
Status i_stdAlloc_freeAll(__attribute__((unused)) void* heap) {
  return status_FAILEDFREE;
}

static inline
AllocatorInfo i_stdAlloc_info(__attribute__((unused)) void* heap) {
  AllocatorInfo out;
  out.total = 0;
  out.used = 0;
  return out;
}

static inline
IAllocator stdAlloc_new(void) {
  IAllocator alloc = {
    .heap = NULL,
    .alloc = i_stdAlloc_alloc,
    .free = i_stdAlloc_free,
    .freeAll = i_stdAlloc_freeAll,
    .info = i_stdAlloc_info,
  };
  return alloc;
}

#endif
