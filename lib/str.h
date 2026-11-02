/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_STR_H
#define PAO_STR_H

#include <stdio.h>
#include <string.h>
#include "basicTypes.h"
#include "status.h"
#include "config.h"
#include "iallocator.h"

#if config_DEBUG
  #include "debug.h"
#endif

/* An immutable string: `length` bytes at `data`. It does not own them,
 * and it is not NUL-terminated. The empty string may have `data == NULL`.
 * Whoever allocated the bytes (see `str_copy`) must free them. */
typedef struct {
  const char* data;
  usize length;
} Str;

/* Returns true if two strings have the same length and
   have the same bytes in order.
*/
static inline
bool str_equals(const Str* A, const Str* B) {
  #if config_DEBUG
    if (A == NULL || B == NULL) {
      debug_FATAL("String is null.");
    }
  #endif
  if (A->length != B->length) {
    return false;
  }
  // NOTE(1):
  usize i = 0;
  while (i < A->length) {
    // NOTE(2):
    if (A->data[i] != B->data[i]) {
      return false;
    }
    i++;
  }

  return true;
  /* NOTE(1): From this point onward, A.len == B.len. If it is 0 the data
     may be NULL, and the loop does not run.
     NOTE(2): This may be optimized to compare 8 bytes each time,
              as long as boundary conditions are handled correctly.
  */
}

/* A hash of the bytes of the string. Equal strings have equal hashes. */
static inline
u32 str_hash(const Str* s) {
  #if config_DEBUG
    if (s == NULL) {
      debug_FATAL("String is null.");
    }
  #endif
  // NOTE(1)
  const u8* bytes = (const u8*)s->data;
  u32 out = 0xCAFEBABE;
  usize i = 0;
  while (i < s->length) {
    out = out ^ ((u32)bytes[i] * 0x5bd1e995);
    out = out ^ (out >> 15);
    i++;
  }
  return out;
  /* NOTE(1): `char` may be signed, so the bytes are read as `u8`.
     Otherwise bytes above 0x7f would be sign extended and the hash would
     depend on the platform.
  */
}

/* Copies `s` into memory from `mem`, and puts the copy in `out`.
 * The copy of an empty string allocates nothing.
 * Returns status_OUTOFMEMORY if the allocation fails.
 * The copy is freed with `str_free`, and `mem` must be the same. */
static inline
Status str_copy(const IAllocator* mem, const Str* s, Str* out) {
  #if config_DEBUG
    if (mem == NULL || s == NULL || out == NULL) {
      debug_FATAL("Argument is null.");
    }
    if (s->data == NULL && s->length != 0) {
      debug_FATAL("String has length but no data.");
    }
  #endif
  if (s->length == 0) {
    out->data = NULL;
    out->length = 0;
    return status_OK;
  }

  char* data = (char*)mem->alloc(mem->heap, s->length, __func__);
  if (data == NULL) {
    return status_OUTOFMEMORY;
  }
  memcpy(data, s->data, s->length);
  out->data = data;
  out->length = s->length;
  return status_OK;
}

/* Frees a string created by `str_copy`. The empty string needs no freeing.
 * Returns what the allocator returns. */
static inline
Status str_free(const IAllocator* mem, Str s) {
  #if config_DEBUG
    if (mem == NULL) {
      debug_FATAL("Allocator is null.");
    }
  #endif
  if (s.data == NULL) {
    return status_OK;
  }
  return mem->free(mem->heap, (void*)s.data); // NOTE(1)

  /* NOTE(1): a `Str` is immutable, but the bytes came from `alloc`, which
     gave a writable pointer. The cast only gives it back.
  */
}

static inline
void i_str_print(const Str* A) {
  printf("%.*s", (int)A->length, A->data);
}
#endif
