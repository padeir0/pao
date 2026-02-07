/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef MB_stdAlloc_H
#define MB_stdAlloc_H

#include "mb_allocator.h"
#include <stdio.h>

static
void* i_mb_stdAlloc_alloc(
  __attribute__((unused)) void* heap,
  usize size,
  __attribute__((unused)) char* func
) {
  return malloc(size);
}

static
void i_mb_stdAlloc_free(__attribute__((unused)) void* heap, void* obj) {
  free(obj);
}

static
void i_mb_stdAlloc_free_all(__attribute__((unused)) void* heap) {
  printf("error: standard malloc provides no free_all function.\n");
  abort();
}

const
mb_Allocator MB_stdAlloc = {
  .heap = NULL,
  .alloc = i_mb_stdAlloc_alloc,
  .free = i_mb_stdAlloc_free,
  .freeAll = i_mb_stdAlloc_free_all,
};

#endif
