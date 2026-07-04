/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_utf8_H
#define PAO_utf8_H

#include "pao_basicTypes.h"
#include "pao_status.h"

#if PAO_config_debug
  #include "pao_debug.h"
#endif

// masks for `n` lower bits and `n` top bits
#define I_PAO_lowbits(n) (u8)((1u<<(n))-1u)
#define I_PAO_topbits(n) (u8)(~((1u<<(8-(n)))-1u))

// represents an unicode rune.
typedef i32 pao_Rune;

#define PAO_utf8_eof (pao_Rune)(-1)
#define PAO_utf8_invalid (pao_Rune)(-2)

// the return of `pao_utf8_decode`
typedef struct {
  pao_Rune   r;
  usize     bytesRead;
  pao_Status status;
} pao_UTF8DecodeReturn;

/* Conforms to RFC 3629:
 *   Rejects overlong encodings
 *   Rejects surrogate pairs (U+D800–U+DFFF)
 *   Rejects code points beyond U+10FFFF
 *   Rejects ill-formed lead bytes
 */
static inline
pao_UTF8DecodeReturn pao_utf8_decode(const u8* buffer, usize buff_size) {
  #if PAO_config_debug
    if (buffer == NULL) {
      PAO_debug_fatalFmt("Buffer is null.");
    }
  #endif

  pao_UTF8DecodeReturn out = {
    .r = PAO_utf8_invalid,
    .bytesRead = 1, // the caller may choose to skip invalid sequences
    .status = PAO_status_invalidUTF8,
  };

  if (buff_size == 0) {
    out.status = PAO_status_eof;
    out.bytesRead = 0;
    out.r = PAO_utf8_eof;
    return out;
  }

  if ((buffer[0] & I_PAO_topbits(1)) == 0) { // ASCII
    out.r = (pao_Rune)buffer[0];
    out.bytesRead = 1;
    out.status = PAO_status_ok;
  } else if ((buffer[0] & I_PAO_topbits(3)) == I_PAO_topbits(2)) { /* TWO BYTE SEQUENCE */
    if (buff_size < 2) {
      out.status = PAO_status_bufferTooSmall;
      return out;
    }
    if ((buffer[1] & I_PAO_topbits(2)) != I_PAO_topbits(1)) {
      out.status = PAO_status_invalidUTF8;
      return out;
    }
    out.r = (pao_Rune)(buffer[0] & I_PAO_lowbits(5)) << 6 |
            (pao_Rune)(buffer[1] & I_PAO_lowbits(6));
    out.bytesRead = 2;
    // check for overlong sequence
    if (out.r < 0x0080) { out.status = PAO_status_invalidUTF8; return out; }
    out.status = PAO_status_ok;

  } else if ((buffer[0] & I_PAO_topbits(4)) == I_PAO_topbits(3)) {/* THREE BYTE SEQUENCE */
    if (buff_size < 3) {
      out.status = PAO_status_bufferTooSmall;
      return out;
    }
    if ((buffer[1] & I_PAO_topbits(2)) != I_PAO_topbits(1) ||
        (buffer[2] & I_PAO_topbits(2)) != I_PAO_topbits(1)) {
      out.status = PAO_status_invalidUTF8;
      return out;
    }
    out.r = (pao_Rune)(buffer[0] & I_PAO_lowbits(4)) << 12 |
            (pao_Rune)(buffer[1] & I_PAO_lowbits(6)) << 6 |
            (pao_Rune)(buffer[2] & I_PAO_lowbits(6));
    out.bytesRead = 3;
    // check for overlong sequence
    if (out.r < 0x0800) { out.status = PAO_status_invalidUTF8; return out; }
    // check for surrogate pairs
    if (out.r >= 0xD800 && out.r <= 0xDFFF) {
      out.status = PAO_status_invalidUTF8;
      return out;
    }
    out.status = PAO_status_ok;

  } else if ((buffer[0] & I_PAO_topbits(5)) == I_PAO_topbits(4)) {/* FOUR BYTE SEQUENCE */
    if (buff_size < 4) {
      out.status = PAO_status_bufferTooSmall;
      return out;
    }
    if ((buffer[1] & I_PAO_topbits(2)) != I_PAO_topbits(1) ||
        (buffer[2] & I_PAO_topbits(2)) != I_PAO_topbits(1) ||
        (buffer[3] & I_PAO_topbits(2)) != I_PAO_topbits(1)) {
      out.status = PAO_status_invalidUTF8;
      return out;
    }
    out.r = (pao_Rune)(buffer[0] & I_PAO_lowbits(3)) << 18 |
            (pao_Rune)(buffer[1] & I_PAO_lowbits(6)) << 12 |
            (pao_Rune)(buffer[2] & I_PAO_lowbits(6)) << 6 |
            (pao_Rune)(buffer[3] & I_PAO_lowbits(6));
    out.bytesRead = 4;
    // check for overlong sequence
    if (out.r < 0x10000) { out.status = PAO_status_invalidUTF8; return out; }
    // unicode is only defined up to U+10FFFF
    if (out.r > 0x10FFFF) {
      out.status = PAO_status_invalidUTF8;
      return out;
    }
    out.status = PAO_status_ok;
  }
  return out;
}

#endif
