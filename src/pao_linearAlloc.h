/*
Copyright 2025 Artur Iure Vianna Fernandes

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the “Software”), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

// LINEARALLOC
#ifndef PAO_linearAlloc_H
#define PAO_linearAlloc_H

#include <stdio.h>
#include "pao_basicTypes.h"
#include "pao_allocator.h"

typedef struct {
  u8* buffer;
  usize   buffSize;
  usize   allocated;
} pao_LinearAlloc;

/* returns a arena allocated at the beginning of the buffer */
pao_LinearAlloc* pao_linearAlloc_create(uint8_t* buffer, size_t size) {
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
void* pao_linearAlloc_alloc(pao_LinearAlloc* a, size_t size) {
  void* out = (void*)(a->buffer + a->allocated);
  if (a->allocated+size >= a->buffSize) {
    return NULL;
  }
  a->allocated += size;
  return out;
}

/* frees the entire arena */
void pao_linearAlloc_freeAll(pao_LinearAlloc* a) {
  a->allocated = 0;
}

/* returns the amount of memory available */
size_t pao_linearAlloc_available(pao_LinearAlloc* a) {
  return a->buffSize - a->allocated;
}

/* returns the amount of memory used */
size_t pao_linearAlloc_used(pao_LinearAlloc* a) {
  return a->allocated;
}

/* returns true if the allocator is empty */
bool pao_linearAlloc_empty(pao_LinearAlloc* a) {
  return a->allocated == 0;
}

static
void* i_pao_linearAlloc_alloc(
  void* heap,
  usize size,
  __attribute__((unused)) char* func
) {
  return pao_linearAlloc_alloc((pao_LinearAlloc*) heap, size);
}

static
void i_pao_linearAlloc_free(
  __attribute__((unused)) void* heap,
  __attribute__((unused)) void* obj
) {
  printf("error: linear allocator provides no free() function.\n");
  abort();
}

static
void i_pao_linearAlloc_freeAll(void* heap) {
  pao_linearAlloc_freeAll((pao_LinearAlloc*)heap);
}

pao_Allocator pao_linearAlloc_createInterface(pao_LinearAlloc* alloc) {
  pao_Allocator out = {
    .heap = (void*)alloc,
    .alloc = i_pao_linearAlloc_alloc,
    .free = i_pao_linearAlloc_free,
    .freeAll = i_pao_linearAlloc_freeAll,
  };
  return out;
}
#endif
