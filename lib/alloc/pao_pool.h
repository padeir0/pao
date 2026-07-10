/*
MIT License
Copyright 2026 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_pool_H
#define PAO_pool_H

#include "../pao_basicTypes.h"
#include "../pao_status.h"
#include "../pao_allocator.h"
#include "../pao_config.h"
#include "../pao_debug.h"
#include "../pao_util.h"
#include <string.h> /* only using memset */

typedef struct _pool_snode {
  struct _pool_snode* next;
} i_pao_pool_Node;

typedef struct {
  i_pao_pool_Node* _head;
  i_pao_pool_Node* _tail;
  u8* begin;
  u8* end;
  usize chunkSize;
  usize size;
} pao_Pool;

static inline
void i_pao_pool_setList(pao_Pool* p) {
  i_pao_pool_Node* curr = (i_pao_pool_Node*)p->begin;
  /* we need this because of alignment, the chunks may not align
   * and leave a padding at the end of the buffer
   */
  i_pao_pool_Node* end = (i_pao_pool_Node*)((u8*)(p->end) -p->chunkSize);

  p->_head = curr;
  while (curr < end) {
    curr->next = (i_pao_pool_Node*)((u8*)(curr) + p->chunkSize);
    curr = curr->next;
  }

  /* curr is at the edge of the buffer, and may not be valid
   * in case the end is not aligned, we leave padding
   */
  if ((u8*)curr + p->chunkSize != p->end) {
    p->end = (u8*)curr;
    curr = (i_pao_pool_Node*)((u8*)(curr) - p->chunkSize);
    p->size = pao_util_distanceU8Ptr(p->begin, p->end);
  }

  curr->next = NULL;
  p->_tail = curr;
}

#define PAO_pool_minChunkSize sizeof(i_pao_pool_Node)

/* creates a pool at the beginning of the outBuffer
   returns:
    - PAO_status_nullBuffer: if the given buffer is null;
    - PAO_status_badSize: if the chunksize is smaller than PAO_pool_minChunkSize
    - PAO_status_bufferTooSmall: if the buffer can't fit the pool + 1 chunk.
 */
static inline
pao_Status pao_pool_new(usize buffsize, usize chunksize, u8* outBuffer) {
  pao_Pool* p;

  if (outBuffer == NULL) {
    return PAO_status_nullBuffer;
  }

  if (chunksize < PAO_pool_minChunkSize) {
    return PAO_status_badSize;
  }

  if (buffsize < sizeof(pao_Pool) + chunksize) {
    return PAO_status_bufferTooSmall;
  }

  p = (pao_Pool*)outBuffer;
  p->begin = outBuffer + sizeof(pao_Pool);
  p->end = outBuffer + buffsize;
  p->chunkSize = chunksize;
  p->size = pao_util_distanceU8Ptr(p->begin, p->end);

  memset(p->begin, 0, p->size);
  i_pao_pool_setList(p);
  return PAO_status_ok;
}

/* tries to allocate a object of size 'chunksize',
 * returns NULL if it fails to allocate
 */
static inline
void* pao_pool_alloc(pao_Pool* p) {
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
    - PAO_status_outOfBounds: indicates the pointer to be freed is not within the pool
    - PAO_status_badAlignment: indicates the pointer to be freed is unaligned with chunk boundaries
 */
static inline
void pao_pool_free(pao_Pool* p, void* ptr) {
  i_pao_pool_Node* new;

  #if PAO_config_debug
    if (!(p->begin <= (u8*)ptr && (u8*)ptr < p->end)) {
      PAO_debug_fatalFmt("Pointer to be freed is out of bounds. ptr = %p, pool->begin = %p, pool->end = %p.", ptr, (void*)p->begin, (void*)p->end);
    }

    if (pao_util_distanceU8Ptr(ptr, p->begin) % p->chunkSize != 0) {
      PAO_debug_fatalFmt("Pointer to be freed is out of alignment. ptr = %p, pool->begin = %p, pool->chunkSize = %ld.", ptr, (void*)p->begin, p->chunkSize);
    }

    {
      i_pao_pool_Node* curr = p->_head;
      while (curr != NULL) {
        if ((void*)curr == ptr) {
          PAO_debug_fatalFmt("Pointer was already freed. ptr = %p, pool->begin = %p.", ptr, (void*)p->begin);
        }
        curr = curr->next;
      }
    }
  #endif

  new = (i_pao_pool_Node*)ptr;
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
void pao_pool_freeAll(pao_Pool* p) {
  i_pao_pool_setList(p);
}

/* returns the amount of memory available

TODO: this is O(n), it's nice as an integrity check, but
      a O(1) procedure should be provided, with an incremental
      counter in the pao_Pool struct.
 */
static inline
usize pao_pool_available(const pao_Pool* p) {
  usize total = 0;
  i_pao_pool_Node* curr = p->_head;
  while (curr != NULL) {
    total += p->chunkSize;
    curr = curr->next;
  }
  return total;
}

/* returns the amount of memory used
 */
static inline
usize pao_pool_used(const pao_Pool* p) {
  return p->size - pao_pool_available(p);
}

/* returns the amount of total memory managed by this pool
 */
static inline
usize pao_pool_total(const pao_Pool* p) {
  return p->size;
}
/* returns if the pool is empty
 */
static inline
bool pao_pool_empty(const pao_Pool* p) {
  usize total = pao_pool_available(p);
  if (total < p->size) {
    return false;
  }
  return true;
}

static inline
void* i_pao_pool_alloc(
  void* heap,
  usize size,
  __attribute__((unused)) const char* func
) {
  pao_Pool* p = (pao_Pool*) heap;
  if (size > p->chunkSize) {
    return NULL;
  }
  return pao_pool_alloc(p);
}

static inline
pao_Status i_pao_pool_free(
  void* heap,
  void* obj
) {
  pao_pool_free((pao_Pool*)heap, obj);
  return PAO_status_ok;
}

static inline
pao_Status i_pao_pool_freeAll(void* heap) {
  pao_pool_freeAll((pao_Pool*)heap);
  return PAO_status_ok;
}

static inline
pao_AllocatorInfo i_pao_pool_info(void* heap) {
  pao_Pool* p = (pao_Pool*)heap;
  pao_AllocatorInfo info = {
    .used = pao_pool_used(p),
    .total = pao_pool_total(p)
  };
  return info;
}

static inline
pao_Allocator pao_pool_createInterface(pao_Pool* alloc) {
  pao_Allocator out = {
    .heap = (void*)alloc,
    .alloc = i_pao_pool_alloc,
    .free = i_pao_pool_free,
    .freeAll = i_pao_pool_freeAll,
    .info = i_pao_pool_info
  };
  return out;
}
#endif
