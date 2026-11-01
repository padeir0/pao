/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_STR_H
#define PAO_STR_H

#include "basicTypes.h"
#include "stdio.h"
#include "iallocator.h"

typedef struct {
  char* data;
  usize start;
  usize length;
} Str;

/* Returns true if two strings have the same length and
   have the same characters in order. The capacity is not
   taken into account.
*/
static inline
bool str_equals(const Str* A, const Str* B) {
  if (A->length != B->length) {
    return false;
  }
  // NOTE(1):
  usize i = 0;
  while (i < A->length) {
    // NOTE(2):
    if ((A->data+A->start)[i] != (B->data+B->start)[i]) {
      return false;
    }
    i++;
  }

  return true;
  /* NOTE(1): From this point onward, A.len == B.len.
     NOTE(2): This may be optimized to compare 8 bytes each time,
              as long as boundary conditions are handled correctly.
  */
}

u32 str_hash(const Str* s) {
  u32 out = 0xCAFEBABE;
  usize i;
  for (i = 0; i < s->length; i++) {
    out = out ^ (u32)(s->data + s->start)[i] * 0x5bd1e995;
    out = out ^ (out >> 15);
  }
  return out;
}

Str str_copy(IAllocator mem, const Str* s) {
  Str out;
  char* data = mem.alloc(mem.heap, s->length, __FUNCTION__);
  out.data = data;
  out.length = s->length;
  return out;
}

static inline
void i_str_print(const Str* A) {
  printf("%.*s", (int)A->length, A->data + A->start);
}
#endif
