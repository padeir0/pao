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
#include "symbol.h"

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

/* The kind of a node says what the node is, and also what it carries:
 *   `PROGRAM` and `CALL` are composite: they have children (`leafs`) and a
 *   `range`, and no lexeme and no value.
 *   `SYMBOL`, `STRING`, `NUMBER`, `NIL` and `BOOL` are terminal: they have
 *   a lexeme (`tk`) and a value, and no children.
 * Whatever a kind does not carry is left zeroed, and must not be read. */
typedef enum {
  astKind_PROGRAM,
  astKind_CALL,
  astKind_SYMBOL,
  astKind_STRING,
  astKind_NUMBER,
  astKind_NIL,
  astKind_BOOL
} ASTKind;

/* A node of the tree. Every node is separate memory from `mem`, and owns its
 * children, its lexeme and its value, so a tree is freed with `ast_free`.
 * The `AST` is read-only for whoever receives it from the parser.
 *
 * `range` is the part of the source the node covers: for a terminal it is
 * the range of `tk`, and for a composite it is the union of the ranges of
 * the children and of the range it was built with (the brackets of a call
 * are not children, so they come in that range).
 *
 * `value` is limited to Str, Symbol, Number, Bool and Nil (the `Nil` of
 * `AtomValue`, that has the kind `atomKind_NULL`). It is never a List,
 * a Callable or a NativeCallable. `NIL` is a literal `nil`, and it is not
 * the absence of a value: that is told apart by the `kind` of the node.
 *   SYMBOL  `atomKind_SYMBOL`, owned by the `SymbolSpace`.
 *   STRING  `atomKind_STRING`, the decoded bytes, copied with `mem`.
 *   NUMBER  `atomKind_NUMBER`; an `Integer` or a `Rational` and its digits
 *           come from `mem`.
 *   NIL     `atomKind_NULL`.
 *   BOOL    `atomKind_BOOL`.
 *
 * `leafs` has `leafsLength` nodes, in the order of the source, and is NULL
 * when there are none. */
typedef struct i_AST AST;
struct i_AST {
  ASTKind kind;
  Range range;
  Lexeme tk;
  Atom value;
  // Type type; // futuramente, quando eu montar o sistema de inferência.
  AST** leafs;
  usize leafsLength;
};

/* The name of a kind, for printing. */
static inline
const char* astKind_name(ASTKind k) {
  switch (k) {
    case astKind_PROGRAM: return "PROGRAM";
    case astKind_CALL:    return "CALL";
    case astKind_SYMBOL:  return "SYMBOL";
    case astKind_STRING:  return "STRING";
    case astKind_NUMBER:  return "NUMBER";
    case astKind_NIL:     return "NIL";
    case astKind_BOOL:    return "BOOL";
  }
  return "???";
}

/* true for the kinds that have children (`PROGRAM`, `CALL`). */
static inline
bool astKind_isComposite(ASTKind k) {
  return k == astKind_PROGRAM || k == astKind_CALL;
}

/* true for the kinds that have a lexeme and a value: not composite. */
static inline
bool astKind_isTerminal(ASTKind k) {
  return !astKind_isComposite(k);
}

/* Allocates a node of the kind `kind` with nothing in it: no children, a
 * lexeme that is not there and no value. */
static inline
AST* i_ast_alloc(const IAllocator* mem, ASTKind kind, Range range) {
  AST* node = (AST*)mem->alloc(mem->heap, sizeof(AST), __func__);
  if (node == NULL) {
    return NULL;
  }
  node->kind = kind;
  node->range = range;
  node->tk.kind = tokenKind_EOF;
  node->tk.text.data = NULL;
  node->tk.text.length = 0;
  node->tk.range = range_none();
  node->value.kind = atomKind_NULL;
  memset(&node->value.value, 0, sizeof(node->value.value));
  node->leafs = NULL;
  node->leafsLength = 0;
  return node;
}

/* Makes a composite node (`PROGRAM` or `CALL`) of the `n` nodes at
 * `children`, which come in the order of the source. The array is copied, so
 * it stays with the caller (the parser keeps it in a stack of its own), but
 * the nodes now belong to the new node. The range is the union of `range`
 * and the ranges of the children: pass `range_none()` if there is nothing
 * else to cover. The brackets of a call, and the end of the input of an empty
 * program, are what `range` is for.
 * Returns status_OUTOFMEMORY if an allocation fails, and then nothing was
 * taken: the children are still the caller's. Free the node with `ast_free`
 * and the same `mem`. */
static inline
Status ast_newNode(const IAllocator* mem, ASTKind kind, AST* const* children,
                   usize n, Range range, AST** out) {
  #if config_DEBUG
    if (mem == NULL || out == NULL) {
      debug_FATAL("Argument is null.");
    }
    if (!astKind_isComposite(kind)) {
      debug_FATAL("Only a PROGRAM or a CALL has children.");
    }
    if (n > 0 && children == NULL) {
      debug_FATAL("There are children but no array.");
    }
  #endif
  usize i = 0;
  while (i < n) {
    #if config_DEBUG
      if (children[i] == NULL) {
        debug_FATAL("Child is null.");
      }
    #endif
    range = range_merge(range, children[i]->range);
    i++;
  }
  if (n > SIZE_MAX / sizeof(AST*)) { // SAFE(1)
    return status_OUTOFMEMORY;
  }

  AST* node = i_ast_alloc(mem, kind, range);
  if (node == NULL) {
    return status_OUTOFMEMORY;
  }
  if (n > 0) {
    AST** leafs = (AST**)mem->alloc(mem->heap, n * sizeof(AST*), __func__);
    if (leafs == NULL) {
      (void)mem->free(mem->heap, node);
      return status_OUTOFMEMORY;
    }
    memcpy(leafs, children, n * sizeof(AST*));
    node->leafs = leafs;
    node->leafsLength = n;
  }
  *out = node;
  return status_OK;

  /* SAFE(1): the size of the array of children fits in a `usize`. Whatever
   * the allocator does with a size it cannot give is its own business: the
   * answer is always NULL, and then status_OUTOFMEMORY. */
}

/* Makes a terminal node of the kind `kind`, that must be a terminal kind, with
 * the lexeme `tk` and the value `value`, that were checked by the caller. */
static inline
Status i_ast_newTerminal(const IAllocator* mem, ASTKind kind, Lexeme tk,
                         Atom value, AST** out) {
  #if config_DEBUG
    TokenKind want = tokenKind_ID;
    if (mem == NULL || out == NULL) {
      debug_FATAL("Argument is null.");
    }
    if (kind == astKind_STRING) {
      want = tokenKind_STR;
    } else if (kind == astKind_NUMBER) {
      want = tokenKind_NUM;
    }
    if (tk.kind != want) {
      debug_FATAL("The token does not fit the kind of the node.");
    }
  #endif
  AST* node = i_ast_alloc(mem, kind, tk.range);
  if (node == NULL) {
    return status_OUTOFMEMORY;
  }
  node->tk = tk;
  node->value = value;
  *out = node;
  return status_OK;
}

/* A `SYMBOL` node: an identifier, with its symbol. The `tk` is an `ID` token.
 * The lexeme is taken (see `ast_newNode` for what happens when it fails). */
static inline
Status ast_newSymbol(const IAllocator* mem, Lexeme tk, Symbol symbol,
                     AST** out) {
  Atom value;
  value.kind = atomKind_SYMBOL;
  value.value.Symbol = symbol;
  return i_ast_newTerminal(mem, astKind_SYMBOL, tk, value, out);
}

/* A `STRING` node: the `tk` is a `STR` token, and `str` the decoded bytes,
 * that must be a copy made with `mem` (see `str_copy`). Both are taken. */
static inline
Status ast_newString(const IAllocator* mem, Lexeme tk, Str str, AST** out) {
  #if config_DEBUG
    if (str.data == NULL && str.length != 0) {
      debug_FATAL("String has length but no data.");
    }
  #endif
  Atom value;
  value.kind = atomKind_STRING;
  value.value.Str = str;
  return i_ast_newTerminal(mem, astKind_STRING, tk, value, out);
}

/* A `NUMBER` node: the `tk` is a `NUM` token. An `Integer` or a `Rational`
 * in `number` must be allocated with `mem`, the struct and its digits, and is
 * taken with the lexeme. `numberKind_decimal` does not exist yet. */
static inline
Status ast_newNumber(const IAllocator* mem, Lexeme tk, Number number,
                     AST** out) {
  #if config_DEBUG
    if (number.kind == numberKind_decimal) {
      debug_FATAL("Decimal numbers do not exist yet.");
    }
    if ((number.kind == numberKind_integer && number.data.integer == NULL) ||
        (number.kind == numberKind_rational && number.data.rational == NULL)) {
      debug_FATAL("Number has no data.");
    }
  #endif
  Atom value;
  value.kind = atomKind_NUMBER;
  value.value.Number = number;
  return i_ast_newTerminal(mem, astKind_NUMBER, tk, value, out);
}

/* A `NIL` node: a literal `nil`. The `tk` is an `ID` token. */
static inline
Status ast_newNil(const IAllocator* mem, Lexeme tk, AST** out) {
  Atom value;
  value.kind = atomKind_NULL;
  memset(&value.value, 0, sizeof(value.value));
  return i_ast_newTerminal(mem, astKind_NIL, tk, value, out);
}

/* A `BOOL` node: a literal `true` or `false`. The `tk` is an `ID` token. */
static inline
Status ast_newBool(const IAllocator* mem, Lexeme tk, bool b, AST** out) {
  Atom value;
  value.kind = atomKind_BOOL;
  memset(&value.value, 0, sizeof(value.value));
  value.value.Bool = b;
  return i_ast_newTerminal(mem, astKind_BOOL, tk, value, out);
}

/* Frees what a number owns: the `Integer` or the `Rational` and its digits. */
static inline
void i_ast_freeNumber(const IAllocator* mem, Number n) {
  switch (n.kind) {
    case numberKind_nativeInt:
    case numberKind_nativeFloat:
      break;
    case numberKind_integer:
      integer_free(mem, *n.data.integer);
      (void)mem->free(mem->heap, n.data.integer); // NOTE(1)
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

  /* NOTE(1): the statuses of `free` are ignored on purpose, as in
   * `symbolSpace_free`: an arena cannot free, and its memory is released all
   * at once by whoever owns it. */
}

/* Frees the tree: the node, its children, its lexeme and its value. `node`
 * may be NULL, and then nothing happens (that is what a parser that did not
 * find an optional production has). `mem` must be the allocator of the nodes,
 * and the statuses of its `free` are ignored.
 * It recurs as deep as the tree is, which the parser limits. */
static inline
void ast_free(const IAllocator* mem, AST* node) {
  #if config_DEBUG
    if (mem == NULL) {
      debug_FATAL("Allocator is null.");
    }
  #endif
  if (node == NULL) {
    return;
  }
  usize i = 0;
  while (i < node->leafsLength) {
    ast_free(mem, node->leafs[i]);
    i++;
  }
  if (node->leafs != NULL) {
    (void)mem->free(mem->heap, node->leafs);
  }
  if (astKind_isTerminal(node->kind)) {
    (void)lexeme_free(mem, node->tk);
    if (node->kind == astKind_STRING) {
      (void)str_free(mem, node->value.value.Str);
    } else if (node->kind == astKind_NUMBER) {
      i_ast_freeNumber(mem, node->value.value.Number);
    }
  }
  (void)mem->free(mem->heap, node);
}

/* BEGIN: dump
 * The dump is written by one function that is run twice: the first time with
 * no buffer, only to count the bytes, and the second to write them. That is
 * how `ast_snprint` writes everything or nothing. Each function returns the
 * number of bytes it needs, and writes them at `buf` if it is not NULL; they
 * never write more than they return. */

static inline
usize i_ast_put(char* buf, const char* text, usize length) {
  if (buf != NULL) {
    memcpy(buf, text, length);
  }
  return length;
}

static inline
usize i_ast_putChar(char* buf, char c) {
  if (buf != NULL) {
    *buf = c;
  }
  return 1;
}

/* A string between quotes, for reading: `\n`, `\t`, `\"` and `\\` are
 * escaped by name, the other control characters (and DEL) as `\xHH`, and
 * the rest, UTF-8 included, is written as it is. */
static inline
usize i_ast_putQuoted(char* buf, Str s) {
  static const char hex[] = "0123456789abcdef";
  usize n = 0;
  usize i = 0;
  n += i_ast_putChar(buf == NULL ? NULL : buf + n, '"');
  while (i < s.length) {
    u8 c = (u8)s.data[i];
    if (c == '"') {
      n += i_ast_put(buf == NULL ? NULL : buf + n, "\\\"", 2);
    } else if (c == '\\') {
      n += i_ast_put(buf == NULL ? NULL : buf + n, "\\\\", 2);
    } else if (c == '\n') {
      n += i_ast_put(buf == NULL ? NULL : buf + n, "\\n", 2);
    } else if (c == '\t') {
      n += i_ast_put(buf == NULL ? NULL : buf + n, "\\t", 2);
    } else if (c < 0x20 || c == 0x7f) {
      n += i_ast_put(buf == NULL ? NULL : buf + n, "\\x", 2);
      n += i_ast_putChar(buf == NULL ? NULL : buf + n, hex[c >> 4]);
      n += i_ast_putChar(buf == NULL ? NULL : buf + n, hex[c & 15]);
    } else {
      n += i_ast_putChar(buf == NULL ? NULL : buf + n, (char)c);
    }
    i++;
  }
  n += i_ast_putChar(buf == NULL ? NULL : buf + n, '"');
  return n;
}

/* The value of a number: a native integer as `%lld`, a native float as
 * `%.17g`, an `Integer` as `[-]digits` and a `Rational` as `[-]num/den`. */
static inline
usize i_ast_putNumber(char* buf, Number num) {
  char tmp[48]; // SAFE(1)
  int n;
  usize size;
  switch (num.kind) {
    case numberKind_nativeInt:
      n = snprintf(tmp, sizeof(tmp), "%lld", (long long)num.data.nativeInt);
      return i_ast_put(buf, tmp, (usize)n);
    case numberKind_nativeFloat:
      n = snprintf(tmp, sizeof(tmp), "%.17g", num.data.nativeFloat);
      return i_ast_put(buf, tmp, (usize)n);
    case numberKind_integer:
      size = natural_printingSize(&num.data.integer->abs);
      if (num.data.integer->sign == -1) {
        size++;
      }
      if (buf != NULL) {
        (void)integer_snprint(num.data.integer, buf, size);
      }
      return size;
    case numberKind_rational:
      size = natural_printingSize(&num.data.rational->numerator) +
             natural_printingSize(&num.data.rational->denominator) + 1;
      if (num.data.rational->sign == -1) {
        size++;
      }
      if (buf != NULL) {
        (void)rational_snprint(num.data.rational, buf, size);
      }
      return size;
    case numberKind_decimal:
      #if config_DEBUG
        debug_FATAL("Decimal numbers do not exist yet.");
      #endif
      break;
  }
  return 0;

  /* SAFE(1): `%lld` has at most 20 characters and `%.17g` at most 24
   * (`-1.2345678901234567e+308`), so `snprintf` never truncates. */
}

/* What follows the range in the line of a node: a space and the value,
 * for the kinds that have one, and nothing for the others. */
static inline
usize i_ast_putValue(char* buf, const AST* node, const SymbolSpace* sp) {
  usize n = 0;
  if (astKind_isComposite(node->kind)) {
    return 0;
  }
  n += i_ast_putChar(buf, ' ');
  switch (node->kind) {
    case astKind_SYMBOL: {
      Str name = symbolSpace_findSymbol(sp, node->value.value.Symbol);
      n += i_ast_put(buf == NULL ? NULL : buf + n, name.data, name.length);
      break;
    }
    case astKind_STRING:
      n += i_ast_putQuoted(buf == NULL ? NULL : buf + n,
                           node->value.value.Str);
      break;
    case astKind_NUMBER:
      n += i_ast_putNumber(buf == NULL ? NULL : buf + n,
                           node->value.value.Number);
      break;
    case astKind_NIL:
      n += i_ast_put(buf == NULL ? NULL : buf + n, "nil", 3);
      break;
    case astKind_BOOL:
      if (node->value.value.Bool) {
        n += i_ast_put(buf == NULL ? NULL : buf + n, "true", 4);
      } else {
        n += i_ast_put(buf == NULL ? NULL : buf + n, "false", 5);
      }
      break;
    case astKind_PROGRAM:
    case astKind_CALL:
      break;
  }
  return n;
}

/* The line of `node`, and then the lines of its children, one level deeper. */
static inline
usize i_ast_emit(char* buf, const AST* node, const SymbolSpace* sp,
                 usize depth) {
  char tmp[48];
  usize n = 0;
  usize len;
  usize i = 0;
  const char* name = astKind_name(node->kind);
  while (i < depth * 2) {
    n += i_ast_putChar(buf == NULL ? NULL : buf + n, ' ');
    i++;
  }
  n += i_ast_put(buf == NULL ? NULL : buf + n, name, strlen(name));
  n += i_ast_putChar(buf == NULL ? NULL : buf + n, ' ');
  len = range_snprint(&node->range, tmp, sizeof(tmp)); // NOTE(1)
  n += i_ast_put(buf == NULL ? NULL : buf + n, tmp, len);
  n += i_ast_putValue(buf == NULL ? NULL : buf + n, node, sp);
  n += i_ast_putChar(buf == NULL ? NULL : buf + n, '\n');
  i = 0;
  while (i < node->leafsLength) {
    n += i_ast_emit(buf == NULL ? NULL : buf + n, node->leafs[i], sp,
                    depth + 1);
    i++;
  }
  return n;

  /* NOTE(1): a range is at most 43 bytes, so it fits in `tmp`. */
}

/* END: dump */

/* Writes the tree as text, one node per line, in the order of the source:
 *   the indentation, 2 spaces for each level of depth;
 *   the name of the kind (`astKind_name`);
 *   the range (`range_snprint`);
 *   for the terminals, a space and the value: the name of a symbol (from
 *     `sp`), a string between quotes with `\n \t \" \\` and `\xHH` escapes,
 *     a number (`%lld`, `%.17g`, `[-]digits`, `[-]num/den`), `nil`, `true`
 *     or `false`;
 *   a `'\n'`, the last line's included.
 * It has no addresses, so the same tree always gives the same text. All of
 * it is written, without a NUL, and its length returned, or nothing is
 * written and it returns 0 if `size` bytes are not enough (the text is never
 * empty). `sp` is the space of the symbols in the tree.
 * It recurs as deep as the tree is, which the parser limits, and it is meant
 * for debugging. */
static inline
usize ast_snprint(const AST* node, const SymbolSpace* sp, char* buffer,
                  usize size) {
  #if config_DEBUG
    if (node == NULL || sp == NULL || buffer == NULL) {
      debug_FATAL("Argument is null.");
    }
  #endif
  usize needed = i_ast_emit(NULL, node, sp, 0);
  if (needed > size) {
    return 0;
  }
  return i_ast_emit(buffer, node, sp, 0);
}

/* END: AST */

#endif
