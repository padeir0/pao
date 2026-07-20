/*
MIT License
Copyright 2026 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_POOL_H
#define PAO_POOL_H

#include "../basicTypes.h"
#include "../status.h"
#include "../iallocator.h"
#include "../config.h"
#include "../debug.h"
#include "../util.h"
#include <string.h> /* only using memset */

typedef struct _pool_snode {
  struct _pool_snode* next;
} i_pool_Node;

typedef struct {
  i_pool_Node* _head;
  i_pool_Node* _tail;
  u8* begin;
  u8* end;
  usize chunkSize;
  usize size;
} Pool;

static inline
void i_pool_setList(Pool* p) {
  i_pool_Node* curr = (i_pool_Node*)p->begin;
  // NOTE(1)
  i_pool_Node* end = (i_pool_Node*)((u8*)(p->end) -p->chunkSize);

  p->_head = curr;
  while (curr < end) {
    curr->next = (i_pool_Node*)((u8*)(curr) + p->chunkSize);
    curr = curr->next;
  }

  // NOTE(2)
  if ((u8*)curr + p->chunkSize != p->end) {
    p->end = (u8*)curr;
    curr = (i_pool_Node*)((u8*)(curr) - p->chunkSize);
    p->size = util_distanceU8Ptr(p->begin, p->end);
  }

  curr->next = NULL;
  p->_tail = curr;
  /* NOTE(1): we need this because of alignment, the chunks may not align
              and leave a padding at the end of the buffer
     NOTE(2): curr is at the edge of the buffer, and may not be valid
              in case the end is not aligned, we leave padding
  */
}

#define pool_MINCHUNKSIZE sizeof(i_pool_Node)

/* creates a pool at the beginning of the outBuffer
   returns:
    - status_NULLBUFFER: if the given buffer is null;
    - status_BADSIZE: if the chunksize is smaller than pool_minChunkSize
    - status_BUFFERTOOSMALL: if the buffer can't fit the pool + 1 chunk.
 */
static inline
Status pool_new(usize buffsize, usize chunksize, u8* outBuffer) {
  Pool* p;

  if (outBuffer == NULL) {
    return status_NULLBUFFER;
  }

  if (chunksize < pool_MINCHUNKSIZE) {
    return status_BADSIZE;
  }

  if (buffsize < sizeof(Pool) + chunksize) {
    return status_BUFFERTOOSMALL;
  }

  p = (Pool*)outBuffer;
  p->begin = outBuffer + sizeof(Pool);
  p->end = outBuffer + buffsize;
  p->chunkSize = chunksize;
  p->size = util_distanceU8Ptr(p->begin, p->end);

  memset(p->begin, 0, p->size);
  i_pool_setList(p);
  return status_OK;
}

/* tries to allocate a object of size 'chunksize',
 * returns NULL if it fails to allocate
 */
static inline
void* pool_alloc(Pool* p) {
  void* curr;
  if (p->_head == NULL) {
    return NULL;
  }

  curr = p->_head;
  p->_head = p->_head->next;

  if (p->_head == NULL) {
    p->_tail = NULL;
  }
  return curr;
}

/* frees an object allocated in the pool,
 * returns:
    - status_OUTOFBOUNDS: indicates the pointer to be freed is not within the pool
    - status_BADALIGNMENT: indicates the pointer to be freed is unaligned with chunk boundaries
 */
static inline
void pool_free(Pool* p, void* ptr) {
  i_pool_Node* new;

  #if config_DEBUG
    if (!(p->begin <= (u8*)ptr && (u8*)ptr < p->end)) {
      debug_FATALFMT("Pointer to be freed is out of bounds. ptr = %p, pool->begin = %p, pool->end = %p.", ptr, (void*)p->begin, (void*)p->end);
    }

    if (util_distanceU8Ptr(ptr, p->begin) % p->chunkSize != 0) {
      debug_FATALFMT("Pointer to be freed is out of alignment. ptr = %p, pool->begin = %p, pool->chunkSize = %ld.", ptr, (void*)p->begin, p->chunkSize);
    }

    {
      i_pool_Node* curr = p->_head;
      while (curr != NULL) {
        if ((void*)curr == ptr) {
          debug_FATALFMT("Pointer was already freed. ptr = %p, pool->begin = %p.", ptr, (void*)p->begin);
        }
        curr = curr->next;
      }
    }
  #endif

  new = (i_pool_Node*)ptr;
  new->next = NULL;

  if (p->_head == NULL) {
    p->_head = new;
    p->_tail = new;
    return;
  }

  p->_tail->next = new;
  p->_tail = new;
  return;
}

/* frees all objects in the pool
 */
static inline
void pool_freeAll(Pool* p) {
  i_pool_setList(p);
}

/* returns the amount of memory available

TODO: this is O(n), it's nice as an integrity check, but
      a O(1) procedure should be provided, with an incremental
      counter in the Pool struct.
 */
static inline
usize pool_available(const Pool* p) {
  usize total = 0;
  i_pool_Node* curr = p->_head;
  while (curr != NULL) {
    total += p->chunkSize;
    curr = curr->next;
  }
  return total;
}

/* returns the amount of memory used
 */
static inline
usize pool_used(const Pool* p) {
  return p->size - pool_available(p);
}

/* returns the amount of total memory managed by this pool
 */
static inline
usize pool_total(const Pool* p) {
  return p->size;
}
/* returns if the pool is empty
 */
static inline
bool pool_empty(const Pool* p) {
  usize total = pool_available(p);
  if (total < p->size) {
    return false;
  }
  return true;
}

static inline
void* i_pool_alloc(
  void* heap,
  usize size,
  __attribute__((unused)) const char* func
) {
  Pool* p = (Pool*) heap;
  if (size > p->chunkSize) {
    return NULL;
  }
  return pool_alloc(p);
}

static inline
Status i_pool_free(
  void* heap,
  void* obj
) {
  pool_free((Pool*)heap, obj);
  return status_OK;
}

static inline
Status i_pool_freeAll(void* heap) {
  pool_freeAll((Pool*)heap);
  return status_OK;
}

static inline
AllocatorInfo i_pool_info(void* heap) {
  Pool* p = (Pool*)heap;
  AllocatorInfo info = {
    .used = pool_used(p),
    .total = pool_total(p)
  };
  return info;
}

static inline
IAllocator pool_createInterface(Pool* alloc) {
  IAllocator out = {
    .heap = (void*)alloc,
    .alloc = i_pool_alloc,
    .free = i_pool_free,
    .freeAll = i_pool_freeAll,
    .info = i_pool_info
  };
  return out;
}
#endif
