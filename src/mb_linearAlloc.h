/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef MB_linearAlloc_H
#define MB_linearAlloc_H

#include <stdio.h>
#include "mb_basicTypes.h"
#include "mb_allocator.h"

typedef struct {
  u8*   buffer;
  usize buffSize;
  usize allocated;
} mb_LinearAlloc;

/* returns a arena allocated at the beginning of the buffer */
mb_LinearAlloc* mb_linearAlloc_create(uint8_t* buffer, size_t size) {
  mb_LinearAlloc* out;
  if (buffer == NULL) {
    return NULL;
  }
  if (size < sizeof(mb_LinearAlloc)) {
    return NULL;
  }

  out = (mb_LinearAlloc*)buffer;
  out->buffer = buffer + sizeof(mb_LinearAlloc);
  out->buffSize = size - sizeof(mb_LinearAlloc);
  out->allocated = 0;

  return out;
}

/* returns NULL if it fails to allocate */
void* mb_linearAlloc_alloc(mb_LinearAlloc* a, size_t size) {
  void* out = (void*)(a->buffer + a->allocated);
  if (a->allocated+size >= a->buffSize) {
    return NULL;
  }
  a->allocated += size;
  return out;
}

/* frees the entire arena */
void mb_linearAlloc_freeAll(mb_LinearAlloc* a) {
  a->allocated = 0;
}

/* returns the amount of memory available */
size_t mb_linearAlloc_available(mb_LinearAlloc* a) {
  return a->buffSize - a->allocated;
}

/* returns the amount of memory used */
size_t mb_linearAlloc_used(mb_LinearAlloc* a) {
  return a->allocated;
}

/* returns true if the allocator is empty */
bool mb_linearAlloc_empty(mb_LinearAlloc* a) {
  return a->allocated == 0;
}

static
void* i_mb_linearAlloc_alloc(
  void* heap,
  usize size,
  __attribute__((unused)) char* func
) {
  return mb_linearAlloc_alloc((mb_LinearAlloc*) heap, size);
}

static
void i_mb_linearAlloc_free(
  __attribute__((unused)) void* heap,
  __attribute__((unused)) void* obj
) {
  printf("error: linear allocator provides no free() function.\n");
  abort();
}

static
void i_mb_linearAlloc_freeAll(void* heap) {
  mb_linearAlloc_freeAll((mb_LinearAlloc*)heap);
}

mb_Allocator mb_linearAlloc_createInterface(mb_LinearAlloc* alloc) {
  mb_Allocator out = {
    .heap = (void*)alloc,
    .alloc = i_mb_linearAlloc_alloc,
    .free = i_mb_linearAlloc_free,
    .freeAll = i_mb_linearAlloc_freeAll,
  };
  return out;
}
#endif
