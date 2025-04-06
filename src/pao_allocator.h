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

#ifndef PAO_allocator_H
#define PAO_allocator_H

#include "pao_basicTypes.h"

/*
Allocates an object of given size.

heap: a pointer to the heap, usually contains metadata
size: size of the allocation, usually gets aligned to 8 bytes,
      some allocators may return NULL if the size is too big.
func: name of the function that called `alloc`, this is useful
      for debugging purposes, not all allocators use this.

return: pointer to allocated object, NULL if allocation failed
*/
typedef void* (*pao_Alloc)(void* heap, usize size, char* func);

/*
Frees an object previously allocated by pao_Alloc, freeing a
wild pointer will lead to undefined behaviour. The object must
be freed by the same allocator that allocated it, otherwise
you'll also end up with undefined behaviour.
*/
typedef void  (*pao_Free)(void* heap, void* obj);

/*
Frees the entire heap. Allocators that do not provide this should
crash the program when this function is called (ie, pao_stdmalloc).
*/
typedef void  (*pao_FreeAll)(void* heap);

/*
Generic allocator interface.

Rationale is that allocations should be bureoucratic for the programmer.
Procedures that allocate should receive pao_Allocator, and all calls to 
pao_allocator.alloc must pass the name of the function that allocated
that resource for future debugging purposes. A pao_debugalloc shall
be provided to debug memory leaks and it should be a drop-in
replacement to any other allocator.
*/
typedef struct {
  void* heap;
  pao_Alloc alloc;
  pao_Free free;
  pao_FreeAll freeAll;
} pao_Allocator;

#endif
