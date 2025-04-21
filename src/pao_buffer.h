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

#include <limits.h>
#include <stdio.h>
#include "pao_basicTypes.h"

typedef struct {
  byte* ptr;
  usize len;
  usize cap;

  /* UNSAFE(1): We quietly assume that `len` or `cap` will never overflow,
                simply because, if this struct is properly used, you'd need
                more than 100 exabytes of memory to overflow it. This assumption
                is also true for other `usize` variables that are haphazardly incremented.
  */
} pao_Buffer;

/* Prints n = min(buffer.len, INT_MAX) bytes as a string using printf. 
   UNTESTED: the output of this function is expected to be used for debugging purposes
   and does not posses an automated test routine. */
int pao_buffer_printStr(const pao_Buffer buff) {
  usize bytesToPrint = buff.len;
  if (bytesToPrint > INT_MAX) {
    bytesToPrint = INT_MAX;
  }
  return printf("%.*s", (int)bytesToPrint, buff.ptr);
}

/* Writes a *NULL-terminated* string to the buffer.
   UNTESTED: (TODO)
*/
usize pao_buffer_writeLiteral(pao_Buffer* buff, char* s) {
  usize i = 0;
  while (i + buff->len < buff->cap && s[i] != '\0') {
    buff->ptr[i+buff->len] = (byte)s[i];
    i++;
  }
  buff->len += i;
  return i;
}

/* Writes a string with given size to the buffer.
   UNTESTED: (TODO)
*/
usize pao_buffer_writeStr(pao_Buffer* buff, char* s, int size) {
  usize i = 0;
  while (i + buff->len < buff->cap && i < (usize)size) {
    buff->ptr[i+buff->len] = (byte)s[i];
    i++;
  }
  buff->len += i;
  return i;
}

/* NOTE: two chars plus a space, ie, 255 becomes "FF ",
   so that we can proper separate each char with spaces. */
#define I_PAO_hexCharsPerByte 3 

/* Converts a buffer into a hex string, separated by spaces.
   UNTESTED: (TODO)
*/
usize pao_buffer_toHex(const pao_Buffer in, pao_Buffer* out) {
  usize start = out->len;
  usize i = 0;
  while (i < in.len && out->len + I_PAO_hexCharsPerByte < out->cap) {
    byte b = in.ptr[i];
    // SAFE(1):
    int res = snprintf((char*)out->ptr, I_PAO_hexCharsPerByte, "%X ", b);
    if (res != I_PAO_hexCharsPerByte) {
      break;
    }
    out->len += I_PAO_hexCharsPerByte;
    i++;
  }
  // SAFE(2):
  return out->len - start;

  /* SAFE(1): we _know_ the out buffer has space for one more hex string
              because of the loop condition.
     SAFE(2): `start` is guaranteed to be less than `out->len`, so this
              will not underflow.
  */
}

#endif
