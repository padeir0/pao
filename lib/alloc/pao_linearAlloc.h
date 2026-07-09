/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_linearAlloc_H
#define PAO_linearAlloc_H

#include <stdio.h>
#include "../pao_basicTypes.h"
#include "../pao_allocator.h"

typedef struct {
  u8*   buffer;
  usize buffSize;
  usize allocated;
} pao_LinearAlloc;

/* returns a arena allocated at the beginning of the buffer */
static inline
pao_LinearAlloc* pao_linearAlloc_create(u8* buffer, usize size) {
  pao_LinearAlloc* out;
  if (buffer == NULL) {
    return NULL;
  }
  if (size < sizeof(pao_LinearAlloc)) {
    return NULL;
  }

  out = (pao_LinearAlloc*)buffer;
  out->buffer = buffer + sizeof(pao_LinearAlloc);
  out->buffSize = size - sizeof(pao_LinearAlloc);
  out->allocated = 0;

  return out;
}

/* returns NULL if it fails to allocate */
/* TODO: Allocate at proper alignment in AMD64/ARM64!!! */
static inline
void* pao_linearAlloc_alloc(pao_LinearAlloc* a, usize size) {
  void* out = (void*)(a->buffer + a->allocated);
  if (a->allocated+size > a->buffSize) {
    return NULL;
  }
  a->allocated += size;
  return out;
}

/* frees the entire arena */
static inline
pao_Status pao_linearAlloc_freeAll(pao_LinearAlloc* a) {
  a->allocated = 0;
  return PAO_status_ok;
}

/* returns the amount of memory available */
static inline
usize pao_linearAlloc_available(pao_LinearAlloc* a) {
  return a->buffSize - a->allocated;
}

/* returns the amount of memory used */
static inline
usize pao_linearAlloc_used(pao_LinearAlloc* a) {
  return a->allocated;
}

/* returns the amount of memory managed by this allocator */
static inline
usize pao_linearAlloc_total(pao_LinearAlloc* a) {
  return a->buffSize;
}

/* returns true if the allocator is empty */
static inline
bool pao_linearAlloc_empty(pao_LinearAlloc* a) {
  return a->allocated == 0;
}

static inline
void* i_pao_linearAlloc_alloc(
  void* heap,
  usize size,
  __attribute__((unused)) const char* func
) {
  return pao_linearAlloc_alloc((pao_LinearAlloc*) heap, size);
}

static inline
pao_Status i_pao_linearAlloc_free(
  __attribute__((unused)) void* heap,
  __attribute__((unused)) void* obj
) {
  return PAO_status_failedFree;
}

static inline
pao_Status i_pao_linearAlloc_freeAll(void* heap) {
  return pao_linearAlloc_freeAll((pao_LinearAlloc*)heap);
}

static inline
pao_AllocatorInfo i_pao_linearAlloc_info(void* heap) {
  pao_LinearAlloc* la = (pao_LinearAlloc*)heap;
  pao_AllocatorInfo info = {
    .total = pao_linearAlloc_total(la),
    .used = pao_linearAlloc_used(la),
  };
  return info;
}

static inline
pao_Allocator pao_linearAlloc_createInterface(pao_LinearAlloc* alloc) {
  pao_Allocator out = {
    .heap = (void*)alloc,
    .alloc = i_pao_linearAlloc_alloc,
    .free = i_pao_linearAlloc_free,
    .freeAll = i_pao_linearAlloc_freeAll,
    .info = i_pao_linearAlloc_info,
  };
  return out;
}
#endif
