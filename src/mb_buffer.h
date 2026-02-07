/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef MB_buffer_H
#define MB_buffer_H

// TODO: mb_buffer_copy(in, out) -> usize
// TODO: mb_buffer_copySome(in, out, len) -> usize

#include <limits.h>
#include <stdio.h>
#include <strings.h>
#include "mb_basicTypes.h"

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
} mb_Buffer;

static inline
usize i_mb_buffer_absLen(const mb_Buffer* buff) {
  return buff->start+buff->len;
}
static inline
usize i_mb_buffer_available(const mb_Buffer* buff) {
  return buff->cap - i_mb_buffer_absLen(buff);
}

/* Prints n = min(buffer.len, INT_MAX) bytes as a string using printf. 
   UNTESTED: the output of this function is expected to be used for debugging purposes
   and does not posses an automated test routine. */
int mb_buffer_printStr(const mb_Buffer* buff) {
  usize bytesToPrint = buff->len;
  if (bytesToPrint > INT_MAX) {
    bytesToPrint = INT_MAX;
  }
  return printf("%.*s", (int)bytesToPrint, buff->ptr+buff->start);
}

mb_Buffer mb_buffer_create(byte* ptr, usize cap) {
  mb_Buffer out = {.ptr = ptr, .start = 0, .len = 0, .cap = cap};
  return out;
}

void mb_buffer_reset(mb_Buffer* buff) {
  buff->len = 0;
  buff->start = 0;
}

void mb_buffer_bzero(mb_Buffer* buff) {
  bzero(buff->ptr+buff->start, buff->len);
}

bool mb_buffer_hasSpace(const mb_Buffer* buff, usize size) {
  return size <= i_mb_buffer_available(buff);
}

usize mb_buffer_writeByte(mb_Buffer* buff, byte b) {
  usize absLen = i_mb_buffer_absLen(buff);
  if (absLen < buff->cap) {
    buff->ptr[absLen] = b;
    buff->len += 1;
    return 1;
  }
  return 0;
}

bool mb_buffer_readByte(mb_Buffer* buff, byte* out) {
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
usize mb_buffer_writeLiteral(mb_Buffer* buff, char* s) {
  usize i = 0;
  usize absLen = i_mb_buffer_absLen(buff);
  while (i + absLen < buff->cap && s[i] != '\0') {
    buff->ptr[i+absLen] = (byte)s[i];
    i++;
  }
  buff->len += i;
  return i;
}

/* Writes a string with given size to the buffer.
*/
usize mb_buffer_writeString(mb_Buffer* buff, char* s, usize size) {
  usize i = 0;
  usize absLen = i_mb_buffer_absLen(buff);
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
usize mb_buffer_toHex(const mb_Buffer* in, mb_Buffer* out) {
  usize writeStart = out->len;
  usize i = 0;
  const u8 charsPerByte = 3; // NOTE(2)
  while (i < in->len && mb_buffer_hasSpace(out, charsPerByte)) {
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
              keep I_MB_hexCharsPerByte+1 as the length. We then overwrite these
              null terminators in the next iteration.
     NOTE(2): Two chars plus a space, ie, 255 becomes "FF ",
              so that we can proper separate each char with spaces.
  */
}

/* Returns true if two buffers have the same length and
   have the same characters in order. The capacity is not
   taken into account.
*/
bool mb_buffer_equals(const mb_Buffer* A, const mb_Buffer* B) {
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
   mb_buffer_writeU16L(out, u16) -> usize
   mb_buffer_writeU32L(out, u32) -> usize
   mb_buffer_writeU64L(out, u64) -> usize
   mb_buffer_writeF32L(out, f32) -> usize
   mb_buffer_writeF64L(out, f64) -> usize
   mb_buffer_readU16L(out, *u16) -> bool
   mb_buffer_readU32L(out, *u32) -> bool
   mb_buffer_readU64L(out, *u64) -> bool
   mb_buffer_readF32L(out, *f32) -> bool
   mb_buffer_readF64L(out, *f64) -> bool

 big-endian:
   mb_buffer_writeU16B(out, u16) -> usize
   mb_buffer_writeU32B(out, u32) -> usize
   mb_buffer_writeU64B(out, u64) -> usize
   mb_buffer_writeF32B(out, f32) -> usize
   mb_buffer_writeF64B(out, f64) -> usize
   mb_buffer_readU16B(out, *u16) -> bool
   mb_buffer_readU32B(out, *u32) -> bool
   mb_buffer_readU64B(out, *u64) -> bool
   mb_buffer_readF32B(out, *f32) -> bool
   mb_buffer_readF64B(out, *f64) -> bool
*/

#endif
