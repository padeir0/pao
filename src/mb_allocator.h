/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef MB_allocator_H
#define MB_allocator_H

#include "mb_basicTypes.h"

/*
Allocates an object of given size.

heap: a pointer to the heap, usually contains metadata
size: size of the allocation, usually gets aligned to 8 bytes,
      some allocators may return NULL if the size is too big.
func: name of the function that called `alloc`, this is useful
      for debugging purposes, not all allocators use this.

return: pointer to allocated object, NULL if allocation failed
*/
typedef void* (*mb_Alloc)(void* heap, usize size, char* func);

/*
Frees an object previously allocated by mb_Alloc, freeing a
wild pointer will lead to undefined behaviour. The object must
be freed by the same allocator that allocated it, otherwise
you'll also end up with undefined behaviour.
*/
typedef void  (*mb_Free)(void* heap, void* obj);

/*
Frees the entire heap. Allocators that do not provide this should
crash the program when this function is called (ie, mb_stdmalloc).
*/
typedef void  (*mb_FreeAll)(void* heap);

/*
Generic allocator interface.

Rationale is that allocations should be bureoucratic for the programmer.
Procedures that allocate should receive mb_Allocator, and all calls to 
mb_allocator.alloc must pass the name of the function that allocated
that resource for future debugging purposes. A mb_debugalloc shall
be provided to debug memory leaks and it should be a drop-in
replacement to any other allocator.
*/
typedef struct {
  void* heap;
  mb_Alloc alloc;
  mb_Free free;
  mb_FreeAll freeAll;
} mb_Allocator;

#endif
