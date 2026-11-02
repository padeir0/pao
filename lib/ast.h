/*
MIT License
Copyright 2026 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_AST_H
#define PAO_AST_H

#include <stdio.h>
#include <string.h>
#include "basicTypes.h"
#include "status.h"
#include "config.h"
#include "order.h"
#include "str.h"
#include "iallocator.h"
#include "utf8.h"
#include "lexer.h"
#include "list.h"

#if config_DEBUG
  #include "debug.h"
#endif

/* Everything the parser hands over lives here: positions in the source,
 * ranges, lexemes and the `AST`. Once the parse is over the source text may
 * be freed, because a lexeme keeps its own copy of the text of its token.
 *
 * Lines and columns start at 0 internally and are only shown from 1 (see
 * `range_snprint`). A column counts runes, like the lexer's.
 */

/* BEGIN: Position and Range */

typedef struct {
  u32 line;
  u32 col;
} Position;

/* From `begin` up to, and excluding, `end`. A defined range has
 * `begin <= end`. A range with `begin == end` is defined and has no width
 * (the end of the input, for instance). `range_none` is the range that is
 * not defined yet: it has `begin > end` and is the identity of `range_merge`. */
typedef struct {
  Position begin;
  Position end;
} Range;

/* Lexicographic order: by line, then by column. */
static inline
Order position_compare(Position a, Position b) {
  if (a.line != b.line) {
    return a.line < b.line ? order_LESS : order_GREATER;
  }
  if (a.col != b.col) {
    return a.col < b.col ? order_LESS : order_GREATER;
  }
  return order_EQUAL;
}

/* A defined range. `begin` after `end` is bad usage: use `range_none`. */
static inline
Range range_new(Position begin, Position end) {
  #if config_DEBUG
    if (position_compare(begin, end) == order_GREATER) {
      debug_FATAL("Range begins after it ends.");
    }
  #endif
  Range r = { begin, end };
  return r;
}

/* The range that is not defined: `begin` is the greatest position and `end`
 * the least, so `range_merge` with it gives the other range, with no special
 * case. */
static inline
Range range_none(void) {
  Range r = { { UINT32_MAX, UINT32_MAX }, { 0, 0 } };
  return r;
}

/* true if the range is not defined (`begin > end`). */
static inline
bool range_isNone(Range r) {
  return position_compare(r.begin, r.end) == order_GREATER;
}

/* true if the range is defined and has no width (`begin == end`). */
static inline
bool range_isEmpty(Range r) {
  return position_compare(r.begin, r.end) == order_EQUAL;
}

/* The union: the least `begin` and the greatest `end`. It is commutative,
 * associative, and `range_none` is its identity. Whatever lies between two
 * disjoint ranges is part of the result. */
static inline
Range range_merge(Range a, Range b) {
  Range r;
  r.begin = position_compare(a.begin, b.begin) == order_LESS ? a.begin : b.begin;
  r.end = position_compare(a.end, b.end) == order_GREATER ? a.end : b.end;
  return r;
}

/* The range of the token `t`, which came from lexing the `srcLen` bytes at
 * `src`. A token only has its beginning, so the end is found by walking its
 * bytes, one column for each rune. An invalid UTF-8 byte (which is only found
 * in invalid tokens) also takes one column, so the offending rune is always
 * part of the range of an invalid token. `NL` ends at the start of the next
 * line, and the range of `EOF` has no width. */
static inline
void range_fromToken(const Token* t, const u8* src, usize srcLen, Range* out) {
  #if config_DEBUG
    if (t == NULL || src == NULL || out == NULL) {
      debug_FATAL("Argument is null.");
    }
    if (t->start > t->end || (usize)t->end > srcLen) {
      debug_FATAL("Token is out of the source.");
    }
  #endif

  Position begin = { t->line, t->col };
  Position end = begin;
  if (t->kind == tokenKind_NL) {
    end.line = t->line + 1; // SAFE(1)
    end.col = 0;
  } else {
    usize i = t->start;
    usize last = t->end;
    u32 cols = 0;
    while (i < last) {
      UTF8DecodeReturn d = utf8_decode(src + i, last - i);
      i += d.bytesRead; // SAFE(2)
      cols += 1; // SAFE(3)
    }
    end.col = t->col + cols; // SAFE(3)
  }
  out->begin = begin;
  out->end = end;

  /* UNSAFE(0): the token must come from lexing this very `src`: `col` is
   * then the number of runes before it in its line.
   * SAFE(1): the `NL` is a `'\n'` that was counted in the offsets, and the
   * lexer only has sources of at most `UINT32_MAX` bytes, so `line + 1` fits.
   * SAFE(2): the window is `[i, last)`, so `last - i > 0` and the decoder
   * never reports EOF. It reports at least 1 byte read, and never more than
   * the window, so `i` increases and stops at `last` exactly.
   * SAFE(3): `cols` is at most the number of bytes of the token, and `col`
   * is at most the bytes before it, so `col + cols <= end <= UINT32_MAX`. */
}

/* `v` + 1 for display. It is a wider type, so `UINT32_MAX` does not wrap. */
static inline
unsigned long long i_range_show(u32 v) {
  return (unsigned long long)v + 1ULL;
}

/* Writes the range as text, from 1 and with the exclusive end as it is:
 *   `1:1-1:4`   a token of 3 runes in the beginning of the input
 *   `1:4-2:1`   a `NL` in the column 4 of the line 1
 *   `2:1`       a range with no width: only the beginning is shown
 *   `?`         the range that is not defined (`range_none`)
 * It writes the whole text, without a NUL, and returns its length, or it
 * writes nothing and returns 0 if `size` bytes are not enough. */
static inline
usize range_snprint(const Range* r, char* buffer, usize size) {
  #if config_DEBUG
    if (r == NULL || buffer == NULL) {
      debug_FATAL("Argument is null.");
    }
  #endif
  char tmp[48]; // SAFE(1)
  int n;
  if (range_isNone(*r)) {
    n = snprintf(tmp, sizeof(tmp), "?");
  } else if (range_isEmpty(*r)) {
    n = snprintf(tmp, sizeof(tmp), "%llu:%llu",
                 i_range_show(r->begin.line), i_range_show(r->begin.col));
  } else {
    n = snprintf(tmp, sizeof(tmp), "%llu:%llu-%llu:%llu",
                 i_range_show(r->begin.line), i_range_show(r->begin.col),
                 i_range_show(r->end.line), i_range_show(r->end.col));
  }
  if (n <= 0 || (usize)n > size) {
    return 0;
  }
  memcpy(buffer, tmp, (usize)n);
  return (usize)n;

  /* SAFE(1): the longest text is two positions of two 10 digit numbers
   * (`4294967296`), a ':' in each, and a '-': 43 bytes, so it fits in `tmp`
   * and `n` is never truncated. */
}

/* END: Position and Range */

/* BEGIN: Lexeme */

/* A token that does not depend on the source: its kind, a copy of its text
 * (for `NL`, the `"\n"`; for `EOF`, the empty string) and its range. */
typedef struct {
  TokenKind kind;
  Str text;
  Range range;
} Lexeme;

/* Makes the lexeme of the token `t`, which came from lexing the `srcLen`
 * bytes at `src`. The text is copied into memory from `mem`.
 * Returns status_OUTOFMEMORY if the allocation fails, and then `out` is left
 * alone. Free it with `lexeme_free`, and the same `mem`. */
static inline
Status lexeme_new(const IAllocator* mem, const Token* t, const u8* src,
                  usize srcLen, Lexeme* out) {
  #if config_DEBUG
    if (mem == NULL || t == NULL || src == NULL || out == NULL) {
      debug_FATAL("Argument is null.");
    }
  #endif
  Range range;
  range_fromToken(t, src, srcLen, &range); // checks the token is in `src`
  Str view = { (const char*)(src + t->start), (usize)(t->end - t->start) };
  Str text;
  Status st = str_copy(mem, &view, &text);
  if (st != status_OK) {
    return st;
  }
  out->kind = t->kind;
  out->text = text;
  out->range = range;
  return status_OK;
}

/* Frees what `lexeme_new` allocated. Returns what the allocator returns. */
static inline
Status lexeme_free(const IAllocator* mem, Lexeme l) {
  return str_free(mem, l.text);
}

/* END: Lexeme */

/* BEGIN: AST */

typedef enum {
  astKind_List,
  astKind_Terminal,
  // etc...
} ASTKind;

typedef struct _ast {
  Lexeme tk; // guarda informação do código fonte para futuras mensagens de erro
  ASTKind kind;
  Atom value; // os tipos se limitam à: Str, Symbol, Number, Bool, Nil. Não pode ser List, Callable ou NativeCallable.

  // Type type; // futuramente, quando eu montar o sistema de inferência.
  struct _ast** leafs; // Cada elemento dessa array é um AST*, cada nó é independente e forma uma subarvore.
  usize leafsLength;
} AST;

// TODO: AST_print()

/* END: AST */

#endif
