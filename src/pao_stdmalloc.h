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

#ifndef PAO_STDMALLOC_H
#define PAO_STDMALLOC_H

#include "pao_allocator.h"
#include <stdio.h>

static
void* _pao_stdmalloc_alloc(
  __attribute__((unused)) void* heap,
  usize size,
  __attribute__((unused)) char* func
) {
  return malloc(size);
}

static
void  _pao_stdmalloc_free(__attribute__((unused)) void* heap, void* obj) {
  free(obj);
}

static
void  _pao_stdmalloc_free_all(__attribute__((unused)) void* heap) {
  printf("error: standard malloc provides no free_all function.\n");
  abort();
}

const
pao_Allocator PAO_STDMALLOC = {
  .heap = NULL,
  .alloc = _pao_stdmalloc_alloc,
  .free = _pao_stdmalloc_free,
  .free_all = _pao_stdmalloc_free_all,
};

#endif /* PAO_STDMALLOC_H */
