/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_stdAlloc_H
#define PAO_stdAlloc_H

#include "pao_allocator.h"
#include <stdio.h>

static
void* i_pao_stdAlloc_alloc(
  __attribute__((unused)) void* heap,
  usize size,
  __attribute__((unused)) char* func
) {
  return malloc(size);
}

static
void i_pao_stdAlloc_free(__attribute__((unused)) void* heap, void* obj) {
  free(obj);
}

static
void i_pao_stdAlloc_free_all(__attribute__((unused)) void* heap) {
  printf("error: standard malloc provides no free_all function.\n");
  abort();
}

const
pao_Allocator PAO_stdAlloc = {
  .heap = NULL,
  .alloc = i_pao_stdAlloc_alloc,
  .free = i_pao_stdAlloc_free,
  .freeAll = i_pao_stdAlloc_free_all,
};

#endif
