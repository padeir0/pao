/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_UTF8_H
#define PAO_UTF8_H

#include "basicTypes.h"
#include "status.h"
#include "config.h"

#if config_DEBUG
  #include "debug.h"
#endif

// masks for `n` lower bits and `n` top bits
#define i_utf8_LOWBITS(n) (u8)((1u<<(n))-1u)
#define i_utf8_TOPBITS(n) (u8)(~((1u<<(8-(n)))-1u))

// represents an unicode rune.
typedef i32 Rune;

#define utf8_EOF (Rune)(-1)
#define utf8_INVALID (Rune)(-2)

// the return of `utf8_decode`
typedef struct {
  Rune   r;
  usize  bytesRead;
  Status status;
} UTF8DecodeReturn;

/* Conforms to RFC 3629:
 *   Rejects overlong encodings
 *   Rejects surrogate pairs (U+D800–U+DFFF)
 *   Rejects code points beyond U+10FFFF
 *   Rejects ill-formed lead bytes
 */
static inline
UTF8DecodeReturn utf8_decode(const u8* buffer, usize buff_size) {
  #if config_DEBUG
    if (buffer == NULL) {
      debug_FATAL("Buffer is null.");
    }
  #endif

  UTF8DecodeReturn out = {
    .r = utf8_INVALID,
    .bytesRead = 1, // the caller may choose to skip invalid sequences
    .status = status_INVALIDUTF8,
  };

  if (buff_size == 0) {
    out.status = status_EOF;
    out.bytesRead = 0;
    out.r = utf8_EOF;
    return out;
  }

  if ((buffer[0] & i_utf8_TOPBITS(1)) == 0) { // ASCII
    out.r = (Rune)buffer[0];
    out.bytesRead = 1;
    out.status = status_OK;
  } else if ((buffer[0] & i_utf8_TOPBITS(3)) == i_utf8_TOPBITS(2)) { /* TWO BYTE SEQUENCE */
    if (buff_size < 2) {
      out.status = status_BUFFERTOOSMALL;
      return out;
    }
    if ((buffer[1] & i_utf8_TOPBITS(2)) != i_utf8_TOPBITS(1)) {
      out.status = status_INVALIDUTF8;
      return out;
    }
    out.r = (Rune)(buffer[0] & i_utf8_LOWBITS(5)) << 6 |
            (Rune)(buffer[1] & i_utf8_LOWBITS(6));
    out.bytesRead = 2;
    // check for overlong sequence
    if (out.r < 0x0080) { out.status = status_INVALIDUTF8; return out; }
    out.status = status_OK;

  } else if ((buffer[0] & i_utf8_TOPBITS(4)) == i_utf8_TOPBITS(3)) {/* THREE BYTE SEQUENCE */
    if (buff_size < 3) {
      out.status = status_BUFFERTOOSMALL;
      return out;
    }
    if ((buffer[1] & i_utf8_TOPBITS(2)) != i_utf8_TOPBITS(1) ||
        (buffer[2] & i_utf8_TOPBITS(2)) != i_utf8_TOPBITS(1)) {
      out.status = status_INVALIDUTF8;
      return out;
    }
    out.r = (Rune)(buffer[0] & i_utf8_LOWBITS(4)) << 12 |
            (Rune)(buffer[1] & i_utf8_LOWBITS(6)) << 6 |
            (Rune)(buffer[2] & i_utf8_LOWBITS(6));
    out.bytesRead = 3;
    // check for overlong sequence
    if (out.r < 0x0800) { out.status = status_INVALIDUTF8; return out; }
    // check for surrogate pairs
    if (out.r >= 0xD800 && out.r <= 0xDFFF) {
      out.status = status_INVALIDUTF8;
      return out;
    }
    out.status = status_OK;

  } else if ((buffer[0] & i_utf8_TOPBITS(5)) == i_utf8_TOPBITS(4)) {/* FOUR BYTE SEQUENCE */
    if (buff_size < 4) {
      out.status = status_BUFFERTOOSMALL;
      return out;
    }
    if ((buffer[1] & i_utf8_TOPBITS(2)) != i_utf8_TOPBITS(1) ||
        (buffer[2] & i_utf8_TOPBITS(2)) != i_utf8_TOPBITS(1) ||
        (buffer[3] & i_utf8_TOPBITS(2)) != i_utf8_TOPBITS(1)) {
      out.status = status_INVALIDUTF8;
      return out;
    }
    out.r = (Rune)(buffer[0] & i_utf8_LOWBITS(3)) << 18 |
            (Rune)(buffer[1] & i_utf8_LOWBITS(6)) << 12 |
            (Rune)(buffer[2] & i_utf8_LOWBITS(6)) << 6 |
            (Rune)(buffer[3] & i_utf8_LOWBITS(6));
    out.bytesRead = 4;
    // check for overlong sequence
    if (out.r < 0x10000) { out.status = status_INVALIDUTF8; return out; }
    // unicode is only defined up to U+10FFFF
    if (out.r > 0x10FFFF) {
      out.status = status_INVALIDUTF8;
      return out;
    }
    out.status = status_OK;
  }
  return out;
}

#endif
