/*
MIT License
Copyright 2026 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_LITERAL_H
#define PAO_LITERAL_H

#include <float.h>
#include <stdlib.h>
#include <string.h>
#include "basicTypes.h"
#include "status.h"
#include "config.h"
#include "iallocator.h"
#include "str.h"
#include "numbers/number.h"

#if config_DEBUG
  #include "debug.h"
#endif

/* Conversion of the text of a literal into its value: `NUM` into a `Number`
 * and `STR` into a `Str`. It is what the parser needs to make the terminals
 * of the tree (see `ast_newNumber` and `ast_newString`), and nothing more.
 *
 * Everything that is allocated comes from `mem`, and it is the same `mem`
 * that must free it: `literal_freeNumber` and `str_free`. When a function
 * fails, nothing stays allocated and `out` is not written. */

/* BEGIN: numbers */

static inline
bool i_literal_isDigit(char c) {
  return '0' <= c && c <= '9';
}

/* The value of the digit `c` (as in a hexadecimal number), or -1. */
static inline
int i_literal_digitValue(char c) {
  if ('0' <= c && c <= '9') {
    return c - '0';
  }
  if ('a' <= c && c <= 'f') {
    return c - 'a' + 10;
  }
  if ('A' <= c && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}

/* Consumes `digit {digit | '_'}` of `t`, starting at `pos`, and puts where it
 * stopped in `end`. Returns false, with `end == pos`, if there is no digit. */
static inline
bool i_literal_integer(const Str* t, usize pos, usize* end) {
  if (pos >= t->length || !i_literal_isDigit(t->data[pos])) {
    *end = pos;
    return false;
  }
  usize i = pos + 1;
  while (i < t->length && (i_literal_isDigit(t->data[i]) || t->data[i] == '_')) {
    i++;
  }
  *end = i;
  return true;
}

/* The parts of a decimal number,
 *   [neg] integer [('/' integer) | ('.' [integer])] ['e' ['-'] integer] ['f']
 * as byte ranges `[begin, end)` of the text. */
typedef struct {
  bool neg;
  usize num0, num1;
  bool hasDen;
  usize den0, den1;
  bool hasDot;
  bool hasExp;
  bool hasSuffix;
} i_LiteralShape;

/* Splits a decimal number in its parts. `status_BADNUMBER` if `t` is not
 * exactly one of them. */
static inline
Status i_literal_scan(const Str* t, i_LiteralShape* out) {
  i_LiteralShape sh;
  usize i = 0;
  usize end;
  memset(&sh, 0, sizeof(sh));

  if (i < t->length && t->data[i] == '-') {
    sh.neg = true;
    i++;
  }
  if (!i_literal_integer(t, i, &end)) {
    return status_BADNUMBER;
  }
  sh.num0 = i;
  sh.num1 = end;
  i = end;

  if (i < t->length && t->data[i] == '/') {
    i++;
    if (!i_literal_integer(t, i, &end)) {
      return status_BADNUMBER;
    }
    sh.hasDen = true;
    sh.den0 = i;
    sh.den1 = end;
    i = end;
  } else if (i < t->length && t->data[i] == '.') {
    i++;
    sh.hasDot = true;
    if (i_literal_integer(t, i, &end)) {
      i = end;
    }
  }

  if (i < t->length && t->data[i] == 'e') {
    i++;
    if (i < t->length && t->data[i] == '-') {
      i++;
    }
    if (!i_literal_integer(t, i, &end)) {
      return status_BADNUMBER;
    }
    sh.hasExp = true;
    i = end;
  }

  if (i < t->length && t->data[i] == 'f') {
    sh.hasSuffix = true;
    i++;
  }

  if (i != t->length) {
    return status_BADNUMBER;
  }
  *out = sh;
  return status_OK;
}

/* Reads the decimal digits `t[a0, a1)` (that may have `_`) as an `i64`, with
 * the sign `neg`. Returns false, without writing `out`, if it does not fit. */
static inline
bool i_literal_decI64(const Str* t, usize a0, usize a1, bool neg, i64* out) {
  u64 limit = (u64)INT64_MAX;
  u64 v = 0;
  usize k = a0;
  if (neg) {
    limit = limit + 1;
  }
  while (k < a1) {
    char c = t->data[k];
    if (c != '_') {
      u64 d = (u64)(c - '0');
      if (v > (limit - d) / 10) { // NOTE(1)
        return false;
      }
      v = v * 10 + d;
    }
    k++;
  }
  if (neg) {
    if (v == limit) { // NOTE(2)
      *out = INT64_MIN;
    } else {
      *out = -(i64)v;
    }
  } else {
    *out = (i64)v;
  }
  return true;
  /* NOTE(1): `v * 10 + d <= limit` is the same as `v <= (limit - d) / 10`,
     with the division rounded down. This way nothing overflows, as `d` is
     at most 9 and `limit` is far bigger.
     NOTE(2): `limit` is 2^63 here, which is not an `i64`, and so neither is
     its negation computed with the cast below. But -2^63 is.
  */
}

/* Reads the decimal digits `t[a0, a1)` (that may have `_`) into the `Natural`
 * `out`, that must be a new one (`natural_new`). The digits are cut in groups
 * of 9 from the right, as 10^9 is the base of the `Natural`. If it fails, `out`
 * is a new one again. */
static inline
Status i_literal_decNatural(const IAllocator* mem, const Str* t,
                            usize a0, usize a1, Natural* out) {
  usize n = 0;
  usize k = a0;
  bool started = false;
  while (k < a1) {
    char c = t->data[k];
    if (c != '_') {
      if (started || c != '0') {
        started = true;
        n++;
      }
    }
    k++;
  }
  if (n == 0) {
    return status_OK; // zero is the `Natural` with no digits
  }

  usize chunks = (n + 8) / 9;
  if (chunks > (usize)INT32_MAX) {
    return status_NATURALNUMBEROVERFLOW;
  }
  u32* digits = (u32*)mem->alloc(mem->heap, chunks * sizeof(u32), __func__);
  if (digits == NULL) {
    return status_OUTOFMEMORY;
  }

  usize want = (n % 9 == 0) ? (usize)9 : n % 9; // the first group is the short one
  usize ci = 0;
  usize inChunk = 0;
  u32 cur = 0;
  started = false;
  k = a0;
  while (k < a1) {
    char c = t->data[k];
    if (c != '_') {
      if (started || c != '0') {
        started = true;
        cur = cur * 10 + (u32)(c - '0'); // SAFE(1)
        inChunk++;
        if (inChunk == want) {
          digits[ci] = cur;
          ci++;
          cur = 0;
          inChunk = 0;
          want = 9;
        }
      }
    }
    k++;
  }

  Status st = natural_setVec(mem, digits, (i32)chunks, out);
  (void)mem->free(mem->heap, digits);
  if (st != status_OK) {
    natural_free(mem, *out);
    *out = natural_new();
  }
  return st;
  /* SAFE(1): a group has at most 9 digits, so `cur` is less than 10^9 and
     the whole group fits in a `u32`, and it is a valid digit of a `Natural`.
  */
}

/* True if the decimal digits `t[a0, a1)` (that may have `_`) are all zeros. */
static inline
bool i_literal_isZero(const Str* t, usize a0, usize a1) {
  usize k = a0;
  while (k < a1) {
    if (t->data[k] != '_' && t->data[k] != '0') {
      return false;
    }
    k++;
  }
  return true;
}

/* An integer: a `nativeInt` if it fits, and an `Integer` if it does not. */
static inline
Status i_literal_decInteger(const IAllocator* mem, const Str* t,
                            const i_LiteralShape* sh, Number* out) {
  i64 v;
  if (i_literal_decI64(t, sh->num0, sh->num1, sh->neg, &v)) {
    out->kind = numberKind_nativeInt;
    out->data.nativeInt = v;
    return status_OK;
  }

  Integer* p = (Integer*)mem->alloc(mem->heap, sizeof(Integer), __func__);
  if (p == NULL) {
    return status_OUTOFMEMORY;
  }
  *p = integer_new();
  Status st = i_literal_decNatural(mem, t, sh->num0, sh->num1, &p->abs);
  if (st != status_OK) {
    (void)mem->free(mem->heap, p);
    return st;
  }
  p->sign = (i8)(sh->neg ? -1 : +1);
  out->kind = numberKind_integer;
  out->data.integer = p;
  return status_OK;
}

/* A fraction `a/b`, as it was written: it is not simplified, and `4/1` is
 * still a `Rational`. A zero denominator is `status_DIVISIONBYZERO`. */
static inline
Status i_literal_decRational(const IAllocator* mem, const Str* t,
                             const i_LiteralShape* sh, Number* out) {
  if (i_literal_isZero(t, sh->den0, sh->den1)) {
    return status_DIVISIONBYZERO;
  }

  Rational* p = (Rational*)mem->alloc(mem->heap, sizeof(Rational), __func__);
  if (p == NULL) {
    return status_OUTOFMEMORY;
  }
  *p = rational_new();
  Status st = i_literal_decNatural(mem, t, sh->num0, sh->num1, &p->numerator);
  if (st == status_OK) {
    st = i_literal_decNatural(mem, t, sh->den0, sh->den1, &p->denominator);
  }
  if (st != status_OK) {
    rational_free(mem, *p);
    (void)mem->free(mem->heap, p);
    return st;
  }
  if (sh->neg && !natural_isZero(&p->numerator)) { // NOTE(1)
    p->sign = -1;
  } else {
    p->sign = +1;
  }
  out->kind = numberKind_rational;
  out->data.rational = p;
  return status_OK;
  /* NOTE(1): zero stays positive, as in `rational_neg`. */
}

/* A float, with the suffix `f`: a `double`, read with `strtod` from a copy of
 * the text with no `_` and no suffix. A value that is too big for a `double`
 * is `status_UNSUPPORTEDNUMBER`, and one that is too small is rounded to the
 * closest `double`, that may be zero. */
static inline
Status i_literal_decFloat(const IAllocator* mem, const Str* t,
                          const i_LiteralShape* sh, Number* out) {
  usize last = sh->hasSuffix ? t->length - 1 : t->length;
  char* buf = (char*)mem->alloc(mem->heap, t->length + 1, __func__);
  if (buf == NULL) {
    return status_OUTOFMEMORY;
  }
  usize n = 0;
  usize k = 0;
  while (k < last) {
    if (t->data[k] != '_') {
      buf[n] = t->data[k];
      n++;
    }
    k++;
  }
  buf[n] = '\0';

  char* end = NULL;
  double v = strtod(buf, &end);
  bool whole = (end == buf + n);
  (void)mem->free(mem->heap, buf);
  if (!whole) {
    return status_BADNUMBER; // SAFE(1)
  }
  if (v > DBL_MAX || v < -DBL_MAX) {
    return status_UNSUPPORTEDNUMBER;
  }
  out->kind = numberKind_nativeFloat;
  out->data.nativeFloat = v;
  return status_OK;
  /* SAFE(1): `i_literal_scan` has checked the text, so `strtod` reads all of it.
     UNSAFE(2): `strtod` uses the decimal point of the current locale. It is
     the one of the "C" locale (a '.') as long as the program never calls
     `setlocale`, which this library never does.
  */
}

/* `0x...` and `0b...`: not negative, and with `_` anywhere among the digits.
 * A `nativeInt` if it fits, and an `Integer` if it does not, that is built
 * digit by digit, as `acc = acc * radix + digit`. */
static inline
Status i_literal_radix(const IAllocator* mem, const Str* t, Number* out) {
  u32 radix = (t->data[1] == 'x') ? 16 : 2;
  u64 limit = (u64)INT64_MAX;
  u64 v = 0;
  bool over = false;
  bool real = false;
  usize k = 2;
  while (k < t->length) {
    char c = t->data[k];
    if (c != '_') {
      int d = i_literal_digitValue(c);
      if (d < 0 || (u32)d >= radix) {
        return status_BADNUMBER;
      }
      real = true;
      if (!over) {
        if (v > (limit - (u64)d) / radix) {
          over = true;
        } else {
          v = v * radix + (u64)d;
        }
      }
    }
    k++;
  }
  if (!real) {
    return status_BADNUMBER;
  }
  if (!over) {
    out->kind = numberKind_nativeInt;
    out->data.nativeInt = (i64)v;
    return status_OK;
  }

  Natural acc = natural_new();
  Natural tmp = natural_new();
  Status st = status_OK;
  k = 2;
  while (st == status_OK && k < t->length) {
    if (t->data[k] != '_') {
      st = natural_multDigit(mem, &acc, radix, &tmp); // NOTE(1)
      if (st == status_OK) {
        st = natural_addDigit(mem, &tmp, (u32)i_literal_digitValue(t->data[k]), &acc);
      }
    }
    k++;
  }
  natural_free(mem, tmp);
  if (st != status_OK) {
    natural_free(mem, acc);
    return st;
  }

  Integer* p = (Integer*)mem->alloc(mem->heap, sizeof(Integer), __func__);
  if (p == NULL) {
    natural_free(mem, acc);
    return status_OUTOFMEMORY;
  }
  p->sign = +1;
  p->abs = acc;
  out->kind = numberKind_integer;
  out->data.integer = p;
  return status_OK;
  /* NOTE(1): `natural_multDigit` does not accept `A == out`, so two `Natural`
     are used, one for the product and the other for the sum.
  */
}

/* Converts the text of a `NUM` token (the `num` of the grammar) to a number.
 *   - an integer (decimal, `0x` or `0b`): a `nativeInt` if it fits in an `i64`,
 *     and an `Integer` if it does not;
 *   - a fraction `a/b`: a `Rational`, as written (not simplified);
 *   - a number with the suffix `f` (`3.14f`, `1e3f`, `1f`): a `nativeFloat`;
 *   - the `_` between the digits is ignored.
 * Returns:
 *   status_BADNUMBER          the text is not a `num`;
 *   status_DIVISIONBYZERO     a fraction with zero as the denominator;
 *   status_UNSUPPORTEDNUMBER  it is a `num`, but there is nothing to hold it
 *                             yet: it has a '.' or an exponent and no `f`
 *                             (that is a `Decimal`), it is a fraction with an
 *                             exponent or `f`, or a float too big for a `double`;
 *   status_OUTOFMEMORY        an allocation failed.
 * The `Integer` and the `Rational` (with their digits) are freed with
 * `literal_freeNumber`, using the same `mem`. */
static inline
Status literal_toNumber(const IAllocator* mem, const Str* text, Number* out) {
  #if config_DEBUG
    if (mem == NULL || text == NULL || out == NULL) {
      debug_FATAL("Argument is null.");
    }
    if (text->data == NULL && text->length != 0) {
      debug_FATAL("String has length but no data.");
    }
  #endif
  if (text->length >= 2 && text->data[0] == '0' &&
      (text->data[1] == 'x' || text->data[1] == 'b')) {
    return i_literal_radix(mem, text, out);
  }

  i_LiteralShape sh;
  Status st = i_literal_scan(text, &sh); status_CHECK;
  if (sh.hasDen) {
    if (sh.hasExp || sh.hasSuffix) {
      return status_UNSUPPORTEDNUMBER;
    }
    return i_literal_decRational(mem, text, &sh, out);
  }
  if (sh.hasSuffix) {
    return i_literal_decFloat(mem, text, &sh, out);
  }
  if (sh.hasDot || sh.hasExp) {
    return status_UNSUPPORTEDNUMBER;
  }
  return i_literal_decInteger(mem, text, &sh, out);
}

/* Frees a number made by `literal_toNumber`: the `Integer` or the `Rational`,
 * its digits and the struct. A `nativeInt` and a `nativeFloat` own nothing.
 * The statuses of `free` are ignored, as in `ast_free`. */
static inline
void literal_freeNumber(const IAllocator* mem, Number n) {
  #if config_DEBUG
    if (mem == NULL) {
      debug_FATAL("Allocator is null.");
    }
  #endif
  switch (n.kind) {
    case numberKind_nativeInt:
    case numberKind_nativeFloat:
      break;
    case numberKind_integer:
      integer_free(mem, *n.data.integer);
      (void)mem->free(mem->heap, n.data.integer);
      break;
    case numberKind_rational:
      rational_free(mem, *n.data.rational);
      (void)mem->free(mem->heap, n.data.rational);
      break;
    case numberKind_decimal:
      #if config_DEBUG
        debug_FATAL("Decimal numbers do not exist yet.");
      #endif
      break;
  }
}

/* END: numbers */

/* BEGIN: strings */

/* The decoded byte of the escape `\e`, or -1 if there is no such escape. */
static inline
int i_literal_escape(char e) {
  switch (e) {
    case 'n':  return '\n';
    case 't':  return '\t';
    case '\'': return '\'';
    case '"':  return '"';
    case '\\': return '\\';
    default:   return -1;
  }
}

/* Goes through the text of a `STR` token (the quotes included), checking that
 * it is one, and counts in `length` the bytes that it decodes to. If `buf` is
 * not NULL, it also writes them there (and it must have room for `length`). */
static inline
Status i_literal_strRun(const Str* t, char* buf, usize* length) {
  usize n = 0;
  usize i = 1;
  bool closed = false;
  if (t->length == 0 || t->data[0] != '"') {
    return status_BADCHAR;
  }
  while (!closed && i < t->length) {
    char c = t->data[i];
    u8 b = (u8)c;
    if (c == '"') {
      closed = true;
      i++;
    } else if (c == '\\') {
      if (i + 1 >= t->length || t->data[i + 1] == '\n') {
        return status_UNTERMINATEDSTRING;
      }
      int e = i_literal_escape(t->data[i + 1]);
      if (e < 0) {
        return status_BADESCAPE;
      }
      if (buf != NULL) {
        buf[n] = (char)e;
      }
      n++;
      i += 2;
    } else if (c == '\n') {
      return status_UNTERMINATEDSTRING;
    } else if ((b < 0x20 || b == 0x7f) && c != '\r') {
      return status_BADCHAR;
    } else {
      if (buf != NULL) {
        buf[n] = c;
      }
      n++;
      i++;
    }
  }
  if (!closed) {
    return status_UNTERMINATEDSTRING;
  }
  if (i != t->length) {
    return status_BADCHAR; // NOTE(1)
  }
  *length = n;
  return status_OK;
  /* NOTE(1): something after the closing quote, or a quote in the middle. */
}

/* Converts the text of a `STR` token (the quotes included) to the bytes that
 * it means: `\n`, `\t`, `\'`, `\"` and `\\` become one byte each, and
 * everything else is copied as it is, bytes of UTF-8 included (that the
 * lexer has already checked). `""` becomes the empty `Str`, with no data and
 * nothing allocated. Returns:
 *   status_UNTERMINATEDSTRING  no closing quote, or a '\n' in the text;
 *   status_BADESCAPE           an escape that does not exist;
 *   status_BADCHAR             a control character, or the text is not a string;
 *   status_OUTOFMEMORY         the allocation failed.
 * The result is freed with `str_free`, using the same `mem`. */
static inline
Status literal_toString(const IAllocator* mem, const Str* text, Str* out) {
  #if config_DEBUG
    if (mem == NULL || text == NULL || out == NULL) {
      debug_FATAL("Argument is null.");
    }
    if (text->data == NULL && text->length != 0) {
      debug_FATAL("String has length but no data.");
    }
  #endif
  usize length = 0;
  Status st = i_literal_strRun(text, NULL, &length); status_CHECK;
  if (length == 0) {
    out->data = NULL;
    out->length = 0;
    return status_OK;
  }
  char* data = (char*)mem->alloc(mem->heap, length, __func__);
  if (data == NULL) {
    return status_OUTOFMEMORY;
  }
  st = i_literal_strRun(text, data, &length); // SAFE(1)
  out->data = data;
  out->length = length;
  return st;
  /* SAFE(1): the text was already checked by the run above, so this one
     cannot fail, and writes exactly `length` bytes.
  */
}

/* END: strings */

#endif
