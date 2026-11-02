/*
MIT License
Copyright 2026 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_STACKALLOC_H
#define PAO_STACKALLOC_H

#include "../basicTypes.h"
#include "../status.h"
#include "../iallocator.h"
#include "../config.h"

#if config_DEBUG
  #include "../debug.h"
#endif

/* A stack of chunks that all have the same size. A chunk is allocated on the
 * top and only the one on the top can be freed, so the chunks are always
 * side by side: the chunk number `i` is at `i * chunkSize` from the first one.
 * That is what makes it fit for a stack of work, like the nodes the parser
 * has made and not yet put in a parent: the nodes that go to the same parent
 * are an array, with no copying.
 *
 * Ported from the old `stack_f`, with what it did not have: the header
 * (including the count of what is allocated) is initialized, the allocation
 * checks the bounds, and the chunks are aligned to a `WORD`. */
typedef struct {
  u8*   buffer;
  usize buffSize;
  usize chunkSize;
  usize allocated;
} StackAlloc;

/* Returns a stack allocated at the beginning of `buffer`, whose chunks have
 * `chunkSize` bytes (rounded up to a multiple of `WORD`, see
 * `stackAlloc_chunkSize`), or NULL if the buffer is NULL, or too small for
 * the header and one chunk. The buffer does not need to be aligned, but a few
 * bytes are lost if it is not. A `chunkSize` of 0 is bad usage. */
static inline
StackAlloc* stackAlloc_create(u8* buffer, usize size, usize chunkSize) {
  #if config_DEBUG
    if (chunkSize == 0) {
      debug_FATAL("Chunk size cannot be 0.");
    }
  #endif
  StackAlloc* out;
  usize shift;
  usize room;
  if (buffer == NULL || chunkSize == 0 || chunkSize > SIZE_MAX - (WORD - 1)) {
    return NULL;
  }
  chunkSize = (chunkSize + (WORD - 1)) / WORD * WORD; // SAFE(1)

  shift = (WORD - (uptr)buffer % WORD) % WORD;
  if (size < shift || size - shift < sizeof(StackAlloc)) {
    return NULL;
  }
  room = size - shift - sizeof(StackAlloc);
  if (room < chunkSize) {
    return NULL;
  }

  out = (StackAlloc*)(buffer + shift); // SAFE(2)
  out->buffer = buffer + shift + sizeof(StackAlloc); // NOTE(3)
  out->buffSize = room - room % chunkSize;
  out->chunkSize = chunkSize;
  out->allocated = 0;
  return out;

  /* SAFE(1): `chunkSize` was checked against the wrap around of the rounding.
     SAFE(2): `shift` is less than `WORD`, and it was checked that it fits in
              `size` along with the header, so nothing here leaves the buffer.
     NOTE(3): `sizeof(StackAlloc)` is a multiple of `WORD`, and so is
              `chunkSize`, so every chunk is aligned. The usable size is cut
              to a whole number of chunks.
  */
}

/* The size of every chunk: the `chunkSize` that was asked for, rounded up
 * to a multiple of `WORD`. */
static inline
usize stackAlloc_chunkSize(const StackAlloc* a) {
  return a->chunkSize;
}

/* Allocates a chunk on the top of the stack, or returns NULL if there is no
 * room for another one. The memory is not initialized. */
static inline
void* stackAlloc_alloc(StackAlloc* a) {
  void* out;
  // SAFE(1)
  if (a->chunkSize > a->buffSize - a->allocated) {
    return NULL;
  }
  out = (void*)(a->buffer + a->allocated);
  a->allocated += a->chunkSize;
  return out;

  /* SAFE(1): `allocated` never exceeds `buffSize`, so the subtraction does
              not underflow, and it is a multiple of `chunkSize`, like
              `buffSize`, so the last chunk fits exactly.
  */
}

/* Frees the chunk on the top of the stack. Freeing an empty stack is bad
 * usage (and does nothing if the debug checks are off). */
static inline
void stackAlloc_pop(StackAlloc* a) {
  #if config_DEBUG
    if (a->allocated == 0) {
      debug_FATAL("The stack is empty.");
    }
  #endif
  if (a->allocated == 0) {
    return;
  }
  a->allocated -= a->chunkSize;
}

/* Frees `ptr`, which must be the chunk on the top of the stack: the others
 * cannot be freed, as the chunks are freed in the reverse order of their
 * allocation. Anything else is bad usage; with the debug checks off it
 * returns status_FAILEDFREE and does nothing. */
static inline
Status stackAlloc_free(StackAlloc* a, void* ptr) {
  #if config_DEBUG
    if (a->allocated == 0) {
      debug_FATAL("The stack is empty.");
    }
    if ((u8*)ptr != a->buffer + (a->allocated - a->chunkSize)) {
      debug_FATALFMT("Pointer to be freed is not the top. ptr = %p, top = %p.",
                     ptr, (void*)(a->buffer + (a->allocated - a->chunkSize)));
    }
  #endif
  if (a->allocated == 0 ||
      (u8*)ptr != a->buffer + (a->allocated - a->chunkSize)) {
    return status_FAILEDFREE;
  }
  a->allocated -= a->chunkSize;
  return status_OK;
}

/* Frees every chunk. */
static inline
void stackAlloc_freeAll(StackAlloc* a) {
  a->allocated = 0;
}

/* returns the amount of memory available */
static inline
usize stackAlloc_available(const StackAlloc* a) {
  return a->buffSize - a->allocated;
}

/* returns the amount of memory used */
static inline
usize stackAlloc_used(const StackAlloc* a) {
  return a->allocated;
}

/* returns the amount of memory managed by this allocator */
static inline
usize stackAlloc_total(const StackAlloc* a) {
  return a->buffSize;
}

/* returns true if no chunk is allocated */
static inline
bool stackAlloc_empty(const StackAlloc* a) {
  return a->allocated == 0;
}

static inline
void* i_stackAlloc_alloc(
  void* heap,
  usize size,
  const char* func
) {
  (void)func;
  StackAlloc* a = (StackAlloc*)heap;
  if (size > a->chunkSize) {
    return NULL;
  }
  return stackAlloc_alloc(a);
}

static inline
Status i_stackAlloc_free(
  void* heap,
  void* obj
) {
  return stackAlloc_free((StackAlloc*)heap, obj);
}

static inline
Status i_stackAlloc_freeAll(void* heap) {
  stackAlloc_freeAll((StackAlloc*)heap);
  return status_OK;
}

static inline
AllocatorInfo i_stackAlloc_info(void* heap) {
  StackAlloc* a = (StackAlloc*)heap;
  AllocatorInfo info = {
    .used = stackAlloc_used(a),
    .total = stackAlloc_total(a),
  };
  return info;
}

static inline
IAllocator stackAlloc_createInterface(StackAlloc* alloc) {
  IAllocator out = {
    .heap = (void*)alloc,
    .alloc = i_stackAlloc_alloc,
    .free = i_stackAlloc_free,
    .freeAll = i_stackAlloc_freeAll,
    .info = i_stackAlloc_info,
  };
  return out;
}
#endif
