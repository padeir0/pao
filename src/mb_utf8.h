/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef MB_utf8_H
#define MB_utf8_H

#include "mb_basicTypes.h"
#include "mb_status.h"

#if MB_config_debug
  #include "mb_debug.h"
#endif

// masks for `n` lower bits and `n` top bits
#define I_MB_lowbits(n) (u8)((1u<<(n))-1u)
#define I_MB_topbits(n) (u8)(~((1u<<(8-(n)))-1u))

// represents an unicode rune.
typedef i32 mb_Rune;

#define MB_utf8_eof (mb_Rune)(-1)
#define MB_utf8_invalid (mb_Rune)(-2)

// the return of `mb_utf8_decode`
typedef struct {
  mb_Rune   r;
  usize     bytesRead;
  mb_Status status;
} mb_UTF8DecodeReturn;

/* Conforms to RFC 3629:
 *   Rejects overlong encodings
 *   Rejects surrogate pairs (U+D800–U+DFFF)
 *   Rejects code points beyond U+10FFFF
 *   Rejects ill-formed lead bytes
 */
static inline
mb_UTF8DecodeReturn mb_utf8_decode(const u8* buffer, usize buff_size) {
  #if MB_config_debug
    if (buffer == NULL) {
      MB_debug_fatalFmt("Buffer is null.");
    }
  #endif

  mb_UTF8DecodeReturn out = {
    .r = MB_utf8_invalid,
    .bytesRead = 1, // the caller may choose to skip invalid sequences
    .status = MB_status_invalidUTF8,
  };

  if (buff_size == 0) {
    out.status = MB_status_eof;
    out.bytesRead = 0;
    out.r = MB_utf8_eof;
    return out;
  }

  if ((buffer[0] & I_MB_topbits(1)) == 0) { // ASCII
    out.r = (mb_Rune)buffer[0];
    out.bytesRead = 1;
    out.status = MB_status_ok;
  } else if ((buffer[0] & I_MB_topbits(3)) == I_MB_topbits(2)) { /* TWO BYTE SEQUENCE */
    if (buff_size < 2) {
      out.status = MB_status_bufferTooSmall;
      return out;
    }
    if ((buffer[1] & I_MB_topbits(2)) != I_MB_topbits(1)) {
      out.status = MB_status_invalidUTF8;
      return out;
    }
    out.r = (mb_Rune)(buffer[0] & I_MB_lowbits(5)) << 6 |
            (mb_Rune)(buffer[1] & I_MB_lowbits(6));
    out.bytesRead = 2;
    // check for overlong sequence
    if (out.r < 0x0080) { out.status = MB_status_invalidUTF8; return out; }
    out.status = MB_status_ok;

  } else if ((buffer[0] & I_MB_topbits(4)) == I_MB_topbits(3)) {/* THREE BYTE SEQUENCE */
    if (buff_size < 3) {
      out.status = MB_status_bufferTooSmall;
      return out;
    }
    if ((buffer[1] & I_MB_topbits(2)) != I_MB_topbits(1) ||
        (buffer[2] & I_MB_topbits(2)) != I_MB_topbits(1)) {
      out.status = MB_status_invalidUTF8;
      return out;
    }
    out.r = (mb_Rune)(buffer[0] & I_MB_lowbits(4)) << 12 |
            (mb_Rune)(buffer[1] & I_MB_lowbits(6)) << 6 |
            (mb_Rune)(buffer[2] & I_MB_lowbits(6));
    out.bytesRead = 3;
    // check for overlong sequence
    if (out.r < 0x0800) { out.status = MB_status_invalidUTF8; return out; }
    // check for surrogate pairs
    if (out.r >= 0xD800 && out.r <= 0xDFFF) {
      out.status = MB_status_invalidUTF8;
      return out;
    }
    out.status = MB_status_ok;

  } else if ((buffer[0] & I_MB_topbits(5)) == I_MB_topbits(4)) {/* FOUR BYTE SEQUENCE */
    if (buff_size < 4) {
      out.status = MB_status_bufferTooSmall;
      return out;
    }
    if ((buffer[1] & I_MB_topbits(2)) != I_MB_topbits(1) ||
        (buffer[2] & I_MB_topbits(2)) != I_MB_topbits(1) ||
        (buffer[3] & I_MB_topbits(2)) != I_MB_topbits(1)) {
      out.status = MB_status_invalidUTF8;
      return out;
    }
    out.r = (mb_Rune)(buffer[0] & I_MB_lowbits(3)) << 18 |
            (mb_Rune)(buffer[1] & I_MB_lowbits(6)) << 12 |
            (mb_Rune)(buffer[2] & I_MB_lowbits(6)) << 6 |
            (mb_Rune)(buffer[3] & I_MB_lowbits(6));
    out.bytesRead = 4;
    // check for overlong sequence
    if (out.r < 0x10000) { out.status = MB_status_invalidUTF8; return out; }
    // unicode is only defined up to U+10FFFF
    if (out.r > 0x10FFFF) {
      out.status = MB_status_invalidUTF8;
      return out;
    }
    out.status = MB_status_ok;
  }
  return out;
}

#endif
