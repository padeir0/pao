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

/* returns a arena allocated at the beginning of the buffer, or NULL
 * if the buffer is NULL or too small. The buffer does not need to be aligned,
 * but a few bytes are lost if it is not. */
static inline
LinearAlloc* linearAlloc_create(u8* buffer, usize size) {
  LinearAlloc* out;
  usize shift;
  usize room;
  if (buffer == NULL) {
    return NULL;
  }

  // aligns buffer to word boundary
  shift = (WORD - (uptr)buffer % WORD) % WORD;
  if (size < shift || size - shift < sizeof(LinearAlloc)) {
    return NULL;
  }
  room = size - shift - sizeof(LinearAlloc);

  out = (LinearAlloc*)(buffer + shift); // SAFE(1)
  out->buffer = buffer + shift + sizeof(LinearAlloc); // NOTE(2)
  out->buffSize = room - room % WORD;
  out->allocated = 0;

  return out;
  /* SAFE(1): `shift` is less than `WORD`, and it was checked that it fits in
              `size` along with the header, so nothing here leaves the buffer.
     NOTE(2): `sizeof(LinearAlloc)` is a multiple of `WORD`, so `out->buffer`
              is aligned, and trimming `buffSize` makes the end aligned too.
  */
}

/* returns NULL if it fails to allocate. The size is rounded up to
 * a multiple of `WORD` so that every object is aligned. */
static inline
void* linearAlloc_alloc(LinearAlloc* a, usize size) {
  usize padded;
  void* out;
  // SAFE(1)
  if (size > SIZE_MAX - (WORD - 1)) {
    return NULL;
  }
  padded = (size + (WORD - 1)) / WORD * WORD;
  // SAFE(2)
  if (padded > a->buffSize - a->allocated) {
    return NULL;
  }
  out = (void*)(a->buffer + a->allocated);
  a->allocated += padded;
  return out;
  /* SAFE(1): the rounding up cannot wrap around.
     SAFE(2): `allocated` never exceeds `buffSize`, so the subtraction does not
              underflow. Comparing this way, and not `allocated + padded`,
              cannot overflow either.
  */
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
