/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_LINEARALLOC_H
#define PAO_LINEARALLOC_H

#include <stdio.h>
#include "../basicTypes.h"
#include "../iallocator.h"

typedef struct {
  u8*   buffer;
  usize buffSize;
  usize allocated;
} LinearAlloc;

/* returns a arena allocated at the beginning of the buffer */
static inline
LinearAlloc* linearAlloc_create(u8* buffer, usize size) {
  LinearAlloc* out;
  if (buffer == NULL) {
    return NULL;
  }
  if (size < sizeof(LinearAlloc)) {
    return NULL;
  }

  out = (LinearAlloc*)buffer;
  out->buffer = buffer + sizeof(LinearAlloc);
  out->buffSize = size - sizeof(LinearAlloc);
  out->allocated = 0;

  return out;
}

/* returns NULL if it fails to allocate */
/* TODO: Allocate at proper alignment in AMD64/ARM64!!! */
static inline
void* linearAlloc_alloc(LinearAlloc* a, usize size) {
  void* out = (void*)(a->buffer + a->allocated);
  if (a->allocated+size > a->buffSize) {
    return NULL;
  }
  a->allocated += size;
  return out;
}

/* frees the entire arena */
static inline
Status linearAlloc_freeAll(LinearAlloc* a) {
  a->allocated = 0;
  return status_OK;
}

/* returns the amount of memory available */
static inline
usize linearAlloc_available(LinearAlloc* a) {
  return a->buffSize - a->allocated;
}

/* returns the amount of memory used */
static inline
usize linearAlloc_used(LinearAlloc* a) {
  return a->allocated;
}

/* returns the amount of memory managed by this allocator */
static inline
usize linearAlloc_total(LinearAlloc* a) {
  return a->buffSize;
}

/* returns true if the allocator is empty */
static inline
bool linearAlloc_empty(LinearAlloc* a) {
  return a->allocated == 0;
}

static inline
void* i_linearAlloc_alloc(
  void* heap,
  usize size,
  const char* func
) {
  (void)func;
  return linearAlloc_alloc((LinearAlloc*) heap, size);
}

static inline
Status i_linearAlloc_free(
  void* heap,
  void* obj
) {
  (void)heap;
  (void)obj;
  return status_FAILEDFREE;
}

static inline
Status i_linearAlloc_freeAll(void* heap) {
  return linearAlloc_freeAll((LinearAlloc*)heap);
}

static inline
AllocatorInfo i_linearAlloc_info(void* heap) {
  LinearAlloc* la = (LinearAlloc*)heap;
  AllocatorInfo info = {
    .total = linearAlloc_total(la),
    .used = linearAlloc_used(la),
  };
  return info;
}

static inline
IAllocator linearAlloc_createInterface(LinearAlloc* alloc) {
  IAllocator out = {
    .heap = (void*)alloc,
    .alloc = i_linearAlloc_alloc,
    .free = i_linearAlloc_free,
    .freeAll = i_linearAlloc_freeAll,
    .info = i_linearAlloc_info,
  };
  return out;
}
#endif
