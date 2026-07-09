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
#include <strings.h> /*TODO: reimplement bzero and remove this dependency*/

typedef struct _pool_snode {
  struct _pool_snode* next;
} i_pao_pool_Node;

typedef struct {
  i_pao_pool_Node* head;
  i_pao_pool_Node* tail;
  u8* begin;
  u8* end;
  usize chunkSize;
  usize size;
} pao_Pool;

static inline
uptr i_pao_pool_distance(uint8_t* a, uint8_t* b) {
  if (a > b) {
    return (uptr)a-(uptr)b;
  } else {
    return (uptr)b-(uptr)a;
  }
}

static inline
void i_pao_pool_setList(pao_Pool* p) {
  i_pao_pool_Node* curr = (i_pao_pool_Node*)p->begin;
  /* we need this because of alignment, the chunks may not align
   * and leave a padding at the end of the buffer
   */
  i_pao_pool_Node* end = (i_pao_pool_Node*)((u8*)(p->end) -p->chunkSize);

  p->head = curr;
  while (curr < end) {
    curr->next = (i_pao_pool_Node*)((u8*)(curr) + p->chunkSize);
    curr = curr->next;
  }

  /* curr is at the edge of the buffer, and may not be valid
   * in case the end is not aligned, we leave padding
   */
  if ((uint8_t*)curr + p->chunkSize != p->end) {
    p->end = (uint8_t*)curr;
    curr = (i_pao_pool_Node*)((u8*)(curr) - p->chunkSize);
    p->size = i_pao_pool_distance(p->begin, p->end);
  }

  curr->next = NULL;
  p->tail = curr;
}

const size_t PAO_pool_minChunkSize = sizeof(i_pao_pool_Node);

/* creates a pool at the beginning of the outBuffer
   returns:
    - PAO_status_nullBuffer: if the given buffer is null;
    - PAO_status_badSize: if the chunksize is smaller than PAO_pool_minChunkSize
    - PAO_status_bufferTooSmall: if the buffer can't fit the pool + 1 chunk.
 */
static inline
pao_Status pao_pool_new(size_t buffsize, size_t chunksize, uint8_t* outBuffer) {
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
  p->size = i_pao_pool_distance(p->begin, p->end);

  bzero(p->begin, p->size);
  i_pao_pool_setList(p);
  return PAO_status_ok;
}

/* tries to allocate a object of size 'chunksize',
 * returns NULL if it fails to allocate
 */
static inline
void* pao_pool_alloc(pao_Pool* p) {
  void* curr;
  if (p->head == NULL) {
    return NULL;
  }

  curr = p->head;
  p->head = p->head->next;

  if (p->head == NULL) {
    p->tail = NULL;
  }
  return curr;
}

/* frees an object allocated in the pool,
 * returns:
    - PAO_status_outOfBounds: indicates the pointer to be freed is not within the pool
    - PAO_status_badAlignment: indicates the pointer to be freed is unaligned with chunk boundaries
 */
static inline
pao_Status pao_pool_free(pao_Pool* p, void* ptr) {
  i_pao_pool_Node* new;

  if (!(p->begin <= (uint8_t*)ptr && (uint8_t*)ptr < p->end)) {
    return PAO_status_outOfBounds;
  }

  if (i_pao_pool_distance(ptr, p->begin) % p->chunkSize != 0) {
    return PAO_status_badAlignment;
  }

  new = (i_pao_pool_Node*)ptr;
  new->next = NULL;

  if (p->head == NULL) {
    p->head = new;
    p->tail = new;
    return PAO_status_ok;
  }

  p->tail->next = new;
  p->tail = new;
  return PAO_status_ok;
}

/* frees all objects in the pool
 */
static inline
pao_Status pao_pool_freeAll(pao_Pool* p) {
  i_pao_pool_setList(p);
  return PAO_status_ok;
}

/* returns the amount of memory available
 */
static inline
size_t pao_pool_available(pao_Pool* p) {
  size_t total = 0;
  i_pao_pool_Node* curr = p->head;
  while (curr != NULL) {
    total += p->chunkSize;
    curr = curr->next;
  }
  return total;
}

/* returns the amount of memory used
 */
static inline
size_t pao_pool_used(pao_Pool* p) {
  return p->size - pao_pool_available(p);
}

/* returns the amount of total memory managed by this pool
 */
static inline
size_t pao_pool_total(pao_Pool* p) {
  return p->size;
}
/* returns if the pool is empty
 */
static inline
bool pao_pool_empty(pao_Pool* p) {
  size_t total = pao_pool_available(p);
  if (total < p->size) {
    return false;
  }
  return true;
}

static inline
void* i_pao_pool_alloc(
  void* heap,
  usize size,
  __attribute__((unused)) char* func
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
  return pao_pool_free((pao_Pool*)heap, obj);
}

static inline
pao_Status i_pao_pool_freeAll(void* heap) {
  return pao_pool_freeAll((pao_Pool*)heap);
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
