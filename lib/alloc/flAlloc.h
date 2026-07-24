/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_FLALLOC_H
#define PAO_FLALLOC_H

#include <stdio.h>
#include "../basicTypes.h"
#include "../status.h"
#include "../debug.h"
#include "../config.h"
#include "../util.h"
#include "../iallocator.h"

typedef struct {
  usize size;
} i_flAlloc_objHeader;

typedef struct _fl_node {
  usize size;
  struct _fl_node *next;
} i_flAlloc_flNode;

typedef struct {
  i_flAlloc_flNode* head;
  u8* begin;
  u8* end;
  usize   size;
} FLAlloc;

static inline
usize i_flAlloc_pad(usize size) {
  size = size + sizeof(i_flAlloc_objHeader);
  if (size%WORD != 0) {
    size = size + (WORD-size%WORD);
  }
  // NOTE(1):
  if (size < sizeof(i_flAlloc_flNode)) {
    size = sizeof(i_flAlloc_flNode);
  }
  return size;
  /* NOTE(1): objects need space for a Node when deallocated 
  */
}

static inline
Status flAlloc_create(usize size, u8* buffer) {
  FLAlloc* fl;
  if ((uptr)buffer % WORD != 0 ) {
    return status_BADALIGNMENT;
  }
  if (buffer == NULL) {
    return status_NULLBUFFER;
  }
  if (size < sizeof(FLAlloc) + sizeof(i_flAlloc_flNode)) {
    return status_BUFFERTOOSMALL;
  }

  fl = (FLAlloc*)buffer;
  usize heapHeaderSize = sizeof(FLAlloc);
  if (heapHeaderSize % 8 == 0 ) {
    heapHeaderSize = heapHeaderSize + (WORD - heapHeaderSize%WORD);
  }
  fl->head = (i_flAlloc_flNode*)(buffer + heapHeaderSize);
  fl->head->size = size - heapHeaderSize;
  fl->head->next = NULL;

  fl->begin = (uint8_t*)fl->head;
  fl->end = buffer+size;

  fl->size = util_distanceU8Ptr(fl->begin, fl->end);
  return status_OK;
}

static inline
u8* i_flAlloc_pop(FLAlloc* fl, i_flAlloc_flNode* prev, i_flAlloc_flNode* curr) {
  if (prev != NULL) {
    prev->next = curr->next;
  } else {
    fl->head = curr->next;
  }
  return (uint8_t*)curr;
}

static inline
u8* i_flAlloc_split(FLAlloc* fl, i_flAlloc_flNode* prev, i_flAlloc_flNode* curr, usize requested_size) {
  i_flAlloc_flNode* newnode;

  newnode = (i_flAlloc_flNode*)((uint8_t*)curr + requested_size);
  newnode->size = curr->size - requested_size;
  newnode->next = curr->next;

  curr->size = requested_size;
  curr->next = newnode;

  return i_flAlloc_pop(fl, prev, curr);
}

static inline
void i_flAlloc_getNode(FLAlloc* fl, usize size, u8** outptr, usize* allocsize) {
  i_flAlloc_flNode* curr;
  i_flAlloc_flNode* prev;

  if (fl->head == NULL) {
    *outptr = NULL;
    *allocsize = 0;
    return;
  }

  curr = fl->head;
  prev = NULL;

  while (curr != NULL) {
    if (curr->size == size) {
      *outptr = i_flAlloc_pop(fl, prev, curr);
      *allocsize = size;
      return;
    }

    if (curr->size > size) {
      // NOTE(1)
      if (curr->size - size < sizeof(i_flAlloc_flNode)) {
        *outptr = i_flAlloc_pop(fl, prev, curr);
        *allocsize = curr->size;
        return;
      }
      *outptr = i_flAlloc_split(fl, prev, curr, size);
      *allocsize = size;
      return;
    }

    prev = curr;
    curr = curr->next;
  }

  *outptr = NULL;
  *allocsize = 0;
  return;
  /* NOTE(1): if we allocate an object and the remaining
              size is not sufficient for a node,
              we allocate the full space, without splitting
   */
}

/* tries to allocate a object of the given size,
 * returns NULL if it fails to allocate
 */
static inline
void* flAlloc_alloc(FLAlloc* fl, usize size) {
  u8* p;
  usize allocsize;

  size = i_flAlloc_pad(size);

  i_flAlloc_getNode(fl, size, &p, &allocsize);
  if (p == NULL) {
    return NULL;
  }
  ((i_flAlloc_objHeader*) p)->size = allocsize;
  p += sizeof(i_flAlloc_objHeader);
  return p;
}

static inline
void i_flAlloc_append(i_flAlloc_flNode* prev, i_flAlloc_flNode* new) {
  if ((i_flAlloc_flNode*)((u8*)prev + prev->size) == new) {
    /* coalescing: append */
    prev->size = prev->size + new->size;
    return;
  }
  prev->next = new;
  new->next = NULL;
  return;
}

static inline
void i_flAlloc_prepend(FLAlloc* fl, i_flAlloc_flNode* new) {
  if ((i_flAlloc_flNode*)((u8*)new + new->size) == fl->head) {
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
void i_flAlloc_join(i_flAlloc_flNode* prev, i_flAlloc_flNode* new, i_flAlloc_flNode* curr) {
  usize size;

  if ((i_flAlloc_flNode*)((u8*)prev + prev->size) == new) {
    /* coalescing: append */
    size = prev->size + new->size;

    if ((i_flAlloc_flNode*)((u8*)prev + size) == curr) {
      /* in this case, prev, new and curr are adjacent */
      prev->size = size + curr->size;
      prev->next = curr->next;
      return;
    }
    /* here only prev and new are adjacent */
    prev->size = size;
    return;
  }

  if ((i_flAlloc_flNode*)((u8*)new + new->size) == curr) {
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
usize flAlloc_objsize(void* ptr) {
  i_flAlloc_objHeader* obj = (i_flAlloc_objHeader*)((uint8_t*)ptr - sizeof(i_flAlloc_objHeader));
  return obj->size;
}

/* checks if `new` is within the bounds of `curr`, ie,
   the memory stored in `new` was already freed and joined
   with `curr`.
 */
static inline
bool i_flAlloc_within(i_flAlloc_flNode* curr, i_flAlloc_flNode* new) {
  return curr == new || // equal
         ((curr < new) && ((u8*)curr + curr->size) > (u8*)new); // within
}

/* frees an object allocated by the freelist
 * if the object was not allocated in that particular freelist,
 * or if the object is uncorrectly aligned,
 * it returns false
 */
static inline
void flAlloc_free(FLAlloc* fl, void* p) {
  usize size;
  i_flAlloc_flNode* new; i_flAlloc_flNode* prev; i_flAlloc_flNode* curr;
  uint8_t* obj = (uint8_t*)p;

  #if config_DEBUG
  if (obj < fl->begin || fl->end < obj) {
    debug_FATALFMT("Object to be freed was out of bounds. obj = %p, fl->begin = %p, fl->end = %p.", (void*)obj, (void*)fl->begin, (void*)fl->end);
  }
  #endif

  size = flAlloc_objsize(obj);
  new = (i_flAlloc_flNode*)(obj-sizeof(i_flAlloc_objHeader));

  new->size = size;
  new->next = NULL;

  if (fl->head == NULL) {
    fl->head = new;
    return;
  }

  if (new < fl->head) {
    i_flAlloc_prepend(fl, new);
    return;
  }

  prev = NULL;
  curr = fl->head;

  while (curr != NULL) {
    if (i_flAlloc_within(curr, new)) {
      debug_FATALFMT("Double free! obj = %p, curr = %p, curr.size = %zu;", (void*)new, (void*)curr, curr->size);
    }
    if (prev != NULL) {
      if (prev < new && new < curr) {
        /* in this case, 'new' is a middle node */
        i_flAlloc_join(prev, new, curr);
        return;
      }
    }
    
    prev = curr;
    curr = curr->next;
  }

  /* in this case, 'new' is the last node */
  i_flAlloc_append(prev, new);
  return;
}

/* frees all objects in the free list */
static inline
void flAlloc_freeAll(FLAlloc* fl) {
  fl->head = (i_flAlloc_flNode*)fl->begin;
  fl->head->size = fl->size;
  fl->head->next = NULL;
}

/* returns the amount of memory available */
static inline
usize flAlloc_available(FLAlloc* fl) {
  i_flAlloc_flNode* curr = fl->head;
  usize total = 0;

  while (curr != NULL) {
    total += curr->size;
    curr = curr->next;
  }
  return total;
}

/* returns the amount of memory used */
static inline
usize flAlloc_used(FLAlloc* fl) {
  return fl->size - flAlloc_available(fl);
}

/* returns the total amount of memory managed */
static inline
usize flAlloc_total(FLAlloc* fl) {
  return fl->size;
}

/* returns if the heap is empty */
static inline
bool flAlloc_empty(FLAlloc* fl) {
  return flAlloc_available(fl) == fl->size;
}

static inline
void* i_flAlloc_alloc(
  void* heap,
  usize size,
  const char* func
) {
  (void)func;
  FLAlloc* p = (FLAlloc*) heap;
  return flAlloc_alloc(p, size);
}

static inline
Status i_flAlloc_free(
  void* heap,
  void* obj
) {
  flAlloc_free((FLAlloc*)heap, obj);
  return status_OK;
}

static inline
Status i_flAlloc_freeAll(void* heap) {
  flAlloc_freeAll((FLAlloc*)heap);
  return status_OK;
}

static inline
AllocatorInfo i_flAlloc_info(void* heap) {
  FLAlloc* p = (FLAlloc*)heap;
  AllocatorInfo info = {
    .used = flAlloc_used(p),
    .total = flAlloc_total(p)
  };
  return info;
}

static inline
IAllocator flAlloc_createInterface(FLAlloc* alloc) {
  IAllocator out = {
    .heap = (void*)alloc,
    .alloc = i_flAlloc_alloc,
    .free = i_flAlloc_free,
    .freeAll = i_flAlloc_freeAll,
    .info = i_flAlloc_info
  };
  return out;
}

#endif
