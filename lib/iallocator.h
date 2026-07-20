/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_IALLOCATOR_H
#define PAO_IALLOCATOR_H

#include "basicTypes.h"
#include "status.h"

/*
Allocates an object of given size.

heap: a pointer to the heap, usually contains metadata
size: size of the allocation, usually gets aligned to 8 bytes,
      some allocators may return NULL if the size is too big.
func: name of the function that called `alloc`, this is useful
      for debugging purposes, not all allocators use this.

return: pointer to allocated object, NULL if allocation failed
*/
typedef void* (*IAlloc)(void* heap, usize size, const char* func);

/*
Frees an object previously allocated by IAlloc, freeing a
wild pointer will lead to undefined behaviour. The object must
be freed by the same allocator that allocated it, otherwise
you'll also end up with undefined behaviour.
*/
typedef Status (*IFree)(void* heap, void* obj);

/*
Frees the entire heap. Allocators that do not provide this should
crash the program when this function is called (ie, stdmalloc).
*/
typedef Status (*IFreeAll)(void* heap);

typedef struct {
  usize used;
  usize total;
} AllocatorInfo;

/*
Returns information about the heap.
If the allocator has no means to return info, then
it should return all fields set to 0.
*/
typedef AllocatorInfo (*IInfo)(void* heap);

/*
Generic allocator interface.

Rationale is that allocations should be bureoucratic for the programmer.
Procedures that allocate should receive IAllocator, and all calls to 
iallocator.alloc must pass the name of the function that allocated
that resource for future debugging purposes. A debugalloc shall
be provided to debug memory leaks and it should be a drop-in
replacement to any other allocator.
*/
typedef struct {
  void* heap;
  IAlloc alloc;
  IFree free;
  IFreeAll freeAll;
  IInfo info;
} IAllocator;
/*
  TODO: should allocators provide a function that frees AND BZEROES(0) a region of memory?
        this seems reasonable, but the allocator struct will grow quite a bit...
*/

#endif
