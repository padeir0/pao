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
  void* heap,
  usize size,
  const char* func
) {
  (void)heap; (void)func;
  return malloc(size);
}

static inline
Status i_stdAlloc_free(void* heap, void* obj) {
  (void)heap;
  free(obj);
  return status_OK;
}

static inline
Status i_stdAlloc_freeAll(void* heap) {
  (void)heap;
  return status_FAILEDFREE;
}

static inline
AllocatorInfo i_stdAlloc_info(void* heap) {
  (void)heap;
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
