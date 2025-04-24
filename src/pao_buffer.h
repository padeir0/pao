/*
Copyright 2025 Artur Iure Vianna Fernandes

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the “Software”), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef PAO_buffer_H
#define PAO_buffer_H

// TODO: pao_buffer_copy(in, out) -> usize
// TODO: pao_buffer_copySome(in, out, len) -> usize

#include <limits.h>
#include <stdio.h>
#include <strings.h>
#include "pao_basicTypes.h"

/* This is a general purpose read/write buffer.
   You can read and write to the same buffer with no problems.
   You read at the start of the buffer and write to the end.
*/
typedef struct {
  byte* ptr;
  usize start;
  usize len;
  usize cap;

  /* UNSAFE(1): We quietly assume that `len` or `cap` will never overflow,
                simply because, if this struct is properly used, you'd need
                more than 100 exabytes of memory to overflow it. This assumption
                is also true for other `usize` variables that are haphazardly incremented.
     NOTE(1): This struct is 32 bytes. The pointer is always needed,
              if we change usize to u32, we will only save 8bytes due to alignment.
              Changing it to u16 is unfeasible, but would cut in half the size of the struct.
              Anyway, this difference should be meaningless.
  */
} pao_Buffer;

static inline
usize i_pao_buffer_absLen(const pao_Buffer* buff) {
  return buff->start+buff->len;
}
static inline
usize i_pao_buffer_available(const pao_Buffer* buff) {
  return buff->cap - i_pao_buffer_absLen(buff);
}

/* Prints n = min(buffer.len, INT_MAX) bytes as a string using printf. 
   UNTESTED: the output of this function is expected to be used for debugging purposes
   and does not posses an automated test routine. */
int pao_buffer_printStr(const pao_Buffer* buff) {
  usize bytesToPrint = buff->len;
  if (bytesToPrint > INT_MAX) {
    bytesToPrint = INT_MAX;
  }
  return printf("%.*s", (int)bytesToPrint, buff->ptr+buff->start);
}

pao_Buffer pao_buffer_create(byte* ptr, usize cap) {
  pao_Buffer out = {.ptr = ptr, .start = 0, .len = 0, .cap = cap};
  return out;
}

void pao_buffer_reset(pao_Buffer* buff) {
  buff->len = 0;
  buff->start = 0;
}

void pao_buffer_bzero(pao_Buffer* buff) {
  bzero(buff->ptr+buff->start, buff->len);
}

bool pao_buffer_hasSpace(const pao_Buffer* buff, usize size) {
  return size <= i_pao_buffer_available(buff);
}

usize pao_buffer_writeByte(pao_Buffer* buff, byte b) {
  usize absLen = i_pao_buffer_absLen(buff);
  if (absLen < buff->cap) {
    buff->ptr[absLen] = b;
    buff->len += 1;
    return 1;
  }
  return 0;
}

bool pao_buffer_readByte(pao_Buffer* buff, byte* out) {
  if (buff->len > 0) {
    byte b = buff->ptr[buff->start];
    buff->start++;
    buff->len--;
    *out = b; 
    return true;
  }
  return false;
}

/* Writes a *NULL-terminated* string to the buffer.
*/
usize pao_buffer_writeLiteral(pao_Buffer* buff, char* s) {
  usize i = 0;
  usize absLen = i_pao_buffer_absLen(buff);
  while (i + absLen < buff->cap && s[i] != '\0') {
    buff->ptr[i+absLen] = (byte)s[i];
    i++;
  }
  buff->len += i;
  return i;
}

/* Writes a string with given size to the buffer.
*/
usize pao_buffer_writeString(pao_Buffer* buff, char* s, usize size) {
  usize i = 0;
  usize absLen = i_pao_buffer_absLen(buff);
  while (i + absLen < buff->cap && i < size) {
    buff->ptr[i+absLen] = (byte)s[i];
    i++;
  }
  buff->len += i;
  return i;
}

/* Converts a buffer into a hex string, separated by spaces.
   Output buffer will have 3 times input buffer's length.
   UNTESTED: (TODO:)
*/
usize pao_buffer_toHex(const pao_Buffer* in, pao_Buffer* out) {
  usize writeStart = out->len;
  usize i = 0;
  const u8 charsPerByte = 3; // NOTE(2)
  while (i < in->len && pao_buffer_hasSpace(out, charsPerByte)) {
    byte b = in->ptr[i];
    // SAFE(1): NOTE(1):
    int res = snprintf((char*)(out->ptr+out->len), charsPerByte+1, "%02X ", b);
    if (res < charsPerByte) {
      break;
    }
    out->len += charsPerByte;
    i++;
  }
  // SAFE(2):
  return out->len - writeStart;

  /* SAFE(1): we _know_ the out buffer has space for one more hex string
              because of the loop condition.
     SAFE(2): `start` is guaranteed to be less than `out->len`, so this
              will not underflow.
     NOTE(1): snprintf will always NULL-TERMINATE the strings, so we must
              keep I_PAO_hexCharsPerByte+1 as the length. We then overwrite these
              null terminators in the next iteration.
     NOTE(2): Two chars plus a space, ie, 255 becomes "FF ",
              so that we can proper separate each char with spaces.
  */
}

/* Returns true if two buffers have the same length and
   have the same characters in order. The capacity is not
   taken into account.
*/
bool pao_buffer_equals(const pao_Buffer* A, const pao_Buffer* B) {
  if (A->len != B->len) {
    return false;
  }
  // NOTE(1):
  usize i = 0;
  while (i < A->len) {
    // NOTE(2):
    if ((A->ptr+A->start)[i] != (B->ptr+B->start)[i]) {
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

/*
NOTE: Some serialization procedures are missing,
but i will not implement them until i need it,
here's some of them:
 little-endian:
   pao_buffer_writeU16L(out, u16) -> usize
   pao_buffer_writeU32L(out, u32) -> usize
   pao_buffer_writeU64L(out, u64) -> usize
   pao_buffer_writeF32L(out, f32) -> usize
   pao_buffer_writeF64L(out, f64) -> usize
   pao_buffer_readU16L(out, *u16) -> bool
   pao_buffer_readU32L(out, *u32) -> bool
   pao_buffer_readU64L(out, *u64) -> bool
   pao_buffer_readF32L(out, *f32) -> bool
   pao_buffer_readF64L(out, *f64) -> bool

 big-endian:
   pao_buffer_writeU16B(out, u16) -> usize
   pao_buffer_writeU32B(out, u32) -> usize
   pao_buffer_writeU64B(out, u64) -> usize
   pao_buffer_writeF32B(out, f32) -> usize
   pao_buffer_writeF64B(out, f64) -> usize
   pao_buffer_readU16B(out, *u16) -> bool
   pao_buffer_readU32B(out, *u32) -> bool
   pao_buffer_readU64B(out, *u64) -> bool
   pao_buffer_readF32B(out, *f32) -> bool
   pao_buffer_readF64B(out, *f64) -> bool
*/

#endif
