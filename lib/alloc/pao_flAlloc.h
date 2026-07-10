/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_flAlloc_H
#define PAO_flAlloc_H

#include <stdio.h>
#include "../pao_basicTypes.h"
#include "../pao_status.h"
#include "../pao_debug.h"
#include "../pao_config.h"
#include "../pao_util.h"
#include "../pao_allocator.h"

typedef struct {
  usize size;
} i_pao_objHeader;

typedef struct _fl_node {
  usize size;
  struct _fl_node *next;
} i_pao_flNode;

typedef struct {
  i_pao_flNode* head;
  u8* begin;
  u8* end;
  usize   size;
} pao_flAlloc;

static inline
size_t pao_flAlloc_pad(size_t size) {
  size = size + sizeof(i_pao_objHeader);
  if (size%WORD != 0) {
    return size + (WORD-size%WORD);
  }
  /* objects need space for a Node when deallocated */
  if (size < sizeof(i_pao_flNode)) {
    size = sizeof(i_pao_flNode);
  }
  return size;
}

static inline
pao_Status pao_flAlloc_create(usize size, u8* buffer) {
  pao_flAlloc* fl;
  if (buffer == NULL) {
    return PAO_status_nullBuffer;
  }
  if (size < sizeof(pao_flAlloc) + sizeof(i_pao_flNode)) {
    return PAO_status_bufferTooSmall;
  }

  fl = (pao_flAlloc*)buffer;
  fl->head = (i_pao_flNode*)(buffer + sizeof(pao_flAlloc));
  fl->head->size = size - sizeof(pao_flAlloc);
  fl->head->next = NULL;

  fl->begin = (uint8_t*)fl->head;
  fl->end = buffer+size;

  fl->size = pao_util_distanceU8Ptr(fl->begin, fl->end);
  return PAO_status_ok;
}

static inline
u8* i_pao_flAlloc_pop(pao_flAlloc* fl, i_pao_flNode* prev, i_pao_flNode* curr) {
  if (prev != NULL) {
    prev->next = curr->next;
  } else {
    fl->head = curr->next;
  }
  return (uint8_t*)curr;
}

static inline
u8* i_pao_flAlloc_split(pao_flAlloc* fl, i_pao_flNode* prev, i_pao_flNode* curr, size_t requested_size) {
  i_pao_flNode* newnode;

  newnode = (i_pao_flNode*)((uint8_t*)curr + requested_size);
  newnode->size = curr->size - requested_size;
  newnode->next = curr->next;

  curr->size = requested_size;
  curr->next = newnode;

  return i_pao_flAlloc_pop(fl, prev, curr);
}

static inline
void i_pao_flAlloc_getNode(pao_flAlloc* fl, usize size, u8** outptr, usize* allocsize) {
  i_pao_flNode* curr;
  i_pao_flNode* prev;

  if (fl->head == NULL) {
    *outptr = NULL;
    *allocsize = 0;
    return;
  }

  curr = fl->head;
  prev = NULL;

  while (curr != NULL) {
    if (curr->size == size) {
      *outptr = i_pao_flAlloc_pop(fl, prev, curr);
      *allocsize = size;
      return;
    }

    if (curr->size > size) {
      /* if we allocate an object and the remaining
       * size is not sufficient for a node,
       * we allocate the full space, without splitting
       */
      if (curr->size - size < sizeof(i_pao_flNode)) {
        *outptr = i_pao_flAlloc_pop(fl, prev, curr);
        *allocsize = curr->size;
        return;
      }
      *outptr = i_pao_flAlloc_split(fl, prev, curr, size);
      *allocsize = size;
      return;
    }

    prev = curr;
    curr = curr->next;
  }

  *outptr = NULL;
  *allocsize = 0;
  return;
}

/* tries to allocate a object of the given size,
 * returns NULL if it fails to allocate
 */
static inline
void* pao_flAlloc_alloc(pao_flAlloc* fl, usize size) {
  u8* p;
  usize allocsize;

  size = pao_flAlloc_pad(size);

  i_pao_flAlloc_getNode(fl, size, &p, &allocsize);
  if (p == NULL) {
    return NULL;
  }
  ((i_pao_objHeader*) p)->size = allocsize;
  p += sizeof(i_pao_objHeader);
  return p;
}

static inline
void i_pao_flAlloc_append(i_pao_flNode* prev, i_pao_flNode* new) {
  if ((i_pao_flNode*)((u8*)prev + prev->size) == new) {
    /* coalescing: append */
    prev->size = prev->size + new->size;
    return;
  }
  prev->next = new;
  new->next = NULL;
  return;
}

static inline
void i_pao_flAlloc_prepend(pao_flAlloc* fl, i_pao_flNode* new) {
  if ((i_pao_flNode*)((u8*)new + new->size) == fl->head) {
    /* coalescing: prepend */
    new->size = new->size + fl->head->size;
    new->next = fl->head->next;
    fl->head = new;
    return;
  }

  new->next = fl->head;
  fl->head = new;
  return;
}

static inline
void i_pao_flAlloc_join(i_pao_flNode* prev, i_pao_flNode* new, i_pao_flNode* curr) {
  size_t size;

  if ((i_pao_flNode*)((u8*)prev + prev->size) == new) {
    /* coalescing: append */
    size = prev->size + new->size;

    if ((i_pao_flNode*)((u8*)prev + size) == curr) {
      /* in this case, prev, new and curr are adjacent */
      prev->size = size + curr->size;
      prev->next = curr->next;
      return;
    }
    /* here only prev and new are adjacent */
    prev->size = size;
    return;
  }

  if ((i_pao_flNode*)((u8*)new + new->size) == curr) {
    /* coalescing: prepend */
    prev->next = new;
    new->size = new->size + curr->size;
    new->next = curr->next;
    return;
  }

  prev->next = new;
  new->next = curr;
  return;
}

static inline
size_t pao_flAlloc_objsize(void* ptr) {
  i_pao_objHeader* obj = (i_pao_objHeader*)((uint8_t*)ptr - sizeof(i_pao_objHeader));
  return obj->size;
}

/* frees an object allocated by the freelist
 * if the object was not allocated in that particular freelist,
 * or if the object is uncorrectly aligned,
 * it returns false
 */
 // TODO: protect against double-frees in debug mode
static inline
void pao_flAlloc_free(pao_flAlloc* fl, void* p) {
  size_t size;
  i_pao_flNode* new; i_pao_flNode* prev; i_pao_flNode* curr;
  uint8_t* obj = (uint8_t*)p;

  #if PAO_config_debug
  if (obj < fl->begin || fl->end < obj) {
    PAO_debug_fatalFmt("Object to be freed was out of bounds. obj = %p, fl->begin = %p, fl->end = %p.", (void*)obj, (void*)fl->begin, (void*)fl->end);
  }
  #endif

  size = pao_flAlloc_objsize(obj);
  new = (i_pao_flNode*)(obj-sizeof(i_pao_objHeader));

  new->size = size;
  new->next = NULL;

  if (fl->head == NULL) {
    fl->head = new;
    return;
  }

  if (new < fl->head) {
    i_pao_flAlloc_prepend(fl, new);
    return;
  }

  prev = NULL;
  curr = fl->head;

  while (curr != NULL) {
    if (prev != NULL) {
      if (prev < new && new < curr) {
        /* in this case, 'new' is a middle node */
        i_pao_flAlloc_join(prev, new, curr);
        return;
      }
    }
    
    prev = curr;
    curr = curr->next;
  }

  /* in this case, 'new' is the last node */
  i_pao_flAlloc_append(prev, new);
  return;
}

/* frees all objects in the free list */
static inline
void pao_flAlloc_freeAll(pao_flAlloc* fl) {
  fl->head = (i_pao_flNode*)fl->begin;
  fl->head->size = fl->size;
  fl->head->next = NULL;
}

/* returns the amount of memory available */
static inline
size_t pao_flAlloc_available(pao_flAlloc* fl) {
  i_pao_flNode* curr = fl->head;
  size_t total = 0;

  while (curr != NULL) {
    total += curr->size;
    curr = curr->next;
  }
  return total;
}

/* returns the amount of memory used */
static inline
size_t pao_flAlloc_used(pao_flAlloc* fl) {
  return fl->size - pao_flAlloc_available(fl);
}

/* returns the total amount of memory managed */
static inline
size_t pao_flAlloc_total(pao_flAlloc* fl) {
  return fl->size;
}

/* returns if the heap is empty */
static inline
bool pao_flAlloc_empty(pao_flAlloc* fl) {
  return pao_flAlloc_available(fl) == fl->size;
}

static inline
void* i_pao_flAlloc_alloc(
  void* heap,
  usize size,
  __attribute__((unused)) const char* func
) {
  pao_flAlloc* p = (pao_flAlloc*) heap;
  return pao_flAlloc_alloc(p, size);
}

static inline
pao_Status i_pao_flAlloc_free(
  void* heap,
  void* obj
) {
  pao_flAlloc_free((pao_flAlloc*)heap, obj);
  return PAO_status_ok;
}

static inline
pao_Status i_pao_flAlloc_freeAll(void* heap) {
  pao_flAlloc_freeAll((pao_flAlloc*)heap);
  return PAO_status_ok;
}

static inline
pao_AllocatorInfo i_pao_flAlloc_info(void* heap) {
  pao_flAlloc* p = (pao_flAlloc*)heap;
  pao_AllocatorInfo info = {
    .used = pao_flAlloc_used(p),
    .total = pao_flAlloc_total(p)
  };
  return info;
}

static inline
pao_Allocator pao_flAlloc_createInterface(pao_flAlloc* alloc) {
  pao_Allocator out = {
    .heap = (void*)alloc,
    .alloc = i_pao_flAlloc_alloc,
    .free = i_pao_flAlloc_free,
    .freeAll = i_pao_flAlloc_freeAll,
    .info = i_pao_flAlloc_info
  };
  return out;
}

#endif
