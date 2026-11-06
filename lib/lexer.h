/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_LEXER_H
#define PAO_LEXER_H

#include "basicTypes.h"
#include "status.h"
#include "config.h"
#include "utf8.h"

#if config_DEBUG
  #include "debug.h"
#endif

/* The Ovo lexer. It turns UTF-8 source text into tokens, without
 * allocating and without decoding the contents of the tokens: a token is
 * only a kind and a byte range into the source. Strings keep their quotes
 * and escapes (the parser decodes them), numbers are not converted.
 *
 * Every byte of the source is validated, comments included.
 *
 * Errors are fatal: the first invalid token puts the lexer in a failed
 * state, and every following call returns that very same invalid token.
 * `lexer_next` returns `status_OK` for every valid token (`EOF` included)
 * and a status explaining the reason when, and only when, the token is
 * `tokenKind_INVALID`:
 *   status_INVALIDUTF8         ill-formed UTF-8 anywhere in the source.
 *   status_BADCHAR             a rune that cannot be there: tab, control
 *                              characters, non-ASCII outside strings and
 *                              comments, unknown symbols, `-` that does not
 *                              start a number, or an atom glued to
 *                              something that is not a delimiter.
 *   status_BADESCAPE           unknown escape sequence in a string.
 *   status_UNTERMINATEDSTRING  newline or end of input inside a string.
 *   status_BADNUMBER           malformed number, or a number glued to
 *                              something that is not a delimiter.
 *
 * An invalid token covers the source from the start of the lexeme up to,
 * and including, the offending rune; `\n` and the end of input are never
 * included. A number that lacks digits (`0x`, `1/`, `1e`) covers what was
 * consumed. An invalid rune alone is the whole token.
 *
 * Positions: `start` and `end` are byte offsets (`end` exclusive), `line`
 * and `col` start at 0 and refer to the first rune of the token. `col`
 * counts runes. Only `\n` ends a line.
 *
 * Lexical rules:
 *   whitespace  ' ' and '\r'. Comments go from '#' to the next '\n',
 *               which is not consumed. Tabs are invalid everywhere.
 *   NL          one token for each '\n'. No NL is made up at the end of
 *               the input.
 *   id          [a-zA-Z_][a-zA-Z?!_-]*  (no digits).
 *   num         [-]integer[/integer | .[integer]][e[-]integer][f]
 *               | 0x hexdigits | 0b bindigits
 *               where integer = digit {digit | '_'}, and the hex and binary
 *               digits may also be '_', as long as there is at least one
 *               real digit. The '-' only goes with decimals, and the prefixes
 *               and the 'e' are lowercase.
 *   str         '"' {rune} '"', with the escapes \n \t \' \" \\. A raw
 *               newline, tab or control character is invalid.
 *   atoms       `num`, `id` and `str` must be followed by a delimiter (or
 *               the end of input): whitespace, '\n', '#', ']', ')', ',',
 *               ';', ':', '=', '\\', and also '.', except after a number.
 *               An opening '(' or '[' is not a delimiter, so `f(x)` and
 *               `f[x]` are invalid: a call has to be written `(f x)`, and
 *               what follows a term has to be separated from it: `f (x)`.
 */

typedef enum {
  tokenKind_NUM,
  tokenKind_STR,
  tokenKind_ID,
  tokenKind_LBRACKET,
  tokenKind_RBRACKET,
  tokenKind_LPAREN,
  tokenKind_RPAREN,
  tokenKind_COMMA,
  tokenKind_SEMI,
  tokenKind_BACKSLASH,
  tokenKind_COLON,
  tokenKind_DOT,
  tokenKind_EQUALS,
  tokenKind_NL,
  tokenKind_EOF,
  tokenKind_INVALID
} TokenKind;

typedef struct {
  TokenKind kind;
  u32 start;
  u32 end;
  u32 line;
  u32 col;
} Token;

typedef struct {
  const u8* src;
  u32 len;
  u32 pos;
  u32 line;
  u32 col;
  /* status_OK, or the reason of the invalid token in `failedToken` */
  Status failed;
  Token failedToken;
} Lexer;

/* the name of a token kind, for printing. */
static inline
const char* tokenKind_name(TokenKind k) {
  switch (k) {
    case tokenKind_NUM:       return "NUM";
    case tokenKind_STR:       return "STR";
    case tokenKind_ID:        return "ID";
    case tokenKind_LBRACKET:  return "LBRACKET";
    case tokenKind_RBRACKET:  return "RBRACKET";
    case tokenKind_LPAREN:    return "LPAREN";
    case tokenKind_RPAREN:    return "RPAREN";
    case tokenKind_COMMA:     return "COMMA";
    case tokenKind_SEMI:      return "SEMI";
    case tokenKind_BACKSLASH: return "BACKSLASH";
    case tokenKind_COLON:     return "COLON";
    case tokenKind_DOT:       return "DOT";
    case tokenKind_EQUALS:    return "EQUALS";
    case tokenKind_NL:        return "NL";
    case tokenKind_EOF:       return "EOF";
    case tokenKind_INVALID:   return "INVALID";
  }
  return "?";
}

/* BEGIN: internal helpers */

static inline
bool i_lexer_isDigit(Rune r) {
  return r >= '0' && r <= '9';
}

static inline
bool i_lexer_isLetter(Rune r) {
  return (r >= 'a' && r <= 'z') || (r >= 'A' && r <= 'Z');
}

static inline
bool i_lexer_isIdBegin(Rune r) {
  return i_lexer_isLetter(r) || r == '_';
}

static inline
bool i_lexer_isIdContinue(Rune r) {
  return i_lexer_isIdBegin(r) || r == '?' || r == '-' || r == '!';
}

/* digit of a hexadecimal (`hex`) or binary number, '_' included. */
static inline
bool i_lexer_isRadixDigit(Rune r, bool hex) {
  if (r == '_' || (r >= '0' && r <= '1')) {
    return true;
  }
  return hex && (i_lexer_isDigit(r) ||
                 (r >= 'a' && r <= 'f') ||
                 (r >= 'A' && r <= 'F'));
}

/* ASCII control characters, '\n' and '\r' included. */
static inline
bool i_lexer_isControl(Rune r) {
  return (r >= 0 && r < 0x20) || r == 0x7f;
}

static inline
bool i_lexer_isDelimiter(Rune r, bool allowDot) {
  return r == ' ' || r == '\r' || r == '\n' || r == '#' ||
         r == ']' || r == ')' || r == ',' || r == ';' ||
         r == ':' || r == '=' || r == '\\' ||
         (allowDot && r == '.');
}

/* the kind of the single-rune token `r`, or tokenKind_INVALID if `r` is not
 * one of them. */
static inline
TokenKind i_lexer_punctuation(Rune r) {
  switch (r) {
    case '[':  return tokenKind_LBRACKET;
    case ']':  return tokenKind_RBRACKET;
    case '(':  return tokenKind_LPAREN;
    case ')':  return tokenKind_RPAREN;
    case ',':  return tokenKind_COMMA;
    case ';':  return tokenKind_SEMI;
    case '\\': return tokenKind_BACKSLASH;
    case ':':  return tokenKind_COLON;
    case '.':  return tokenKind_DOT;
    case '=':  return tokenKind_EQUALS;
    default:   return tokenKind_INVALID;
  }
}

static inline
bool i_lexer_isEscape(Rune r) {
  return r == 'n' || r == 't' || r == '\'' || r == '"' || r == '\\';
}

/* the rune under the cursor. */
static inline
UTF8DecodeReturn i_lexer_decode(const Lexer* l) {
  return utf8_decode(l->src + l->pos, (usize)(l->len - l->pos));
}

/* the byte `off` bytes ahead of the cursor, or -1 past the end. */
static inline
int i_lexer_byte(const Lexer* l, usize off) {
  usize p = (usize)l->pos + off;
  if (p >= (usize)l->len) {
    return -1;
  }
  return (int)l->src[p];
}

/* consumes the rune `d`, which must be the one under the cursor and
 * must not be '\n'. */
static inline
void i_lexer_advance(Lexer* l, UTF8DecodeReturn d) {
  l->pos += (u32)d.bytesRead; // SAFE(1)
  l->col += 1;

  /* SAFE(1): `d` was decoded from the bytes at `pos`, so `pos` plus
   * `d.bytesRead` is at most `len`, which fits in a `u32`. `col` counts
   * runes, which are at least a byte each, so it never exceeds `len` either.
   * Neither does `line`, which only counts '\n'. */
}

/* consumes one ASCII byte that is not '\n'. */
static inline
void i_lexer_step(Lexer* l) {
  l->pos += 1; // SAFE(2)
  l->col += 1;

  /* SAFE(2): the callers have just seen the ASCII byte under the cursor,
   * so there is a byte left, and the argument of `i_lexer_advance` holds. */
}

/* the token `t` made valid, of kind `kind`, ending at the cursor. */
static inline
Status i_lexer_emit(const Lexer* l, Token t, TokenKind kind, Token* out) {
  Token r = t;
  r.kind = kind;
  r.end = l->pos;
  *out = r;
  return status_OK;
}

/* the token `t` made invalid, ending at `end`. The lexer stays failed. */
static inline
Status i_lexer_fail(Lexer* l, Token t, u32 end, Status why, Token* out) {
  Token r = t;
  r.kind = tokenKind_INVALID;
  r.end = end;
  l->failed = why;
  l->failedToken = r;
  *out = r;
  return why;
}

/* fails with the offending rune `d` included. */
static inline
Status i_lexer_failAt(Lexer* l, Token t, UTF8DecodeReturn d, Status why, Token* out) {
  return i_lexer_fail(l, t, l->pos + (u32)d.bytesRead, why, out); // SAFE(3)

  /* SAFE(3): when `d` comes from an invalid decoding, `bytesRead` is 1 to 3
   * and the decoder only reports as many bytes as there are in the buffer,
   * so `pos` plus `bytesRead` is at most `len`. */
}

/* `d` is the result of a failed decoding that was not the end of input. */
static inline
Status i_lexer_failDecode(Lexer* l, Token t, UTF8DecodeReturn d, Token* out) {
  return i_lexer_failAt(l, t, d, status_INVALIDUTF8, out);
}

/* an invalid rune that is a token by itself. */
static inline
Status i_lexer_failRune(Lexer* l, UTF8DecodeReturn d, Status why, Token* out) {
  Token t = { tokenKind_INVALID, l->pos, l->pos, l->line, l->col };
  return i_lexer_failAt(l, t, d, why, out);
}

/* the token `t` of kind `kind` is complete. Checks what comes after it. */
static inline
Status i_lexer_finishAtom(Lexer* l, Token t, TokenKind kind, bool allowDot,
                          Status badFollower, Token* out) {
  UTF8DecodeReturn d = i_lexer_decode(l);
  if (d.status == status_EOF) {
    return i_lexer_emit(l, t, kind, out);
  }
  if (d.status != status_OK) {
    return i_lexer_failDecode(l, t, d, out);
  }
  if (i_lexer_isDelimiter(d.r, allowDot)) {
    return i_lexer_emit(l, t, kind, out);
  }
  return i_lexer_failAt(l, t, d, badFollower, out);
}

/* skips whitespace and comments. */
static inline
Status i_lexer_skip(Lexer* l, Token* out) {
  bool inComment = false;
  while (true) {
    UTF8DecodeReturn d = i_lexer_decode(l);
    if (d.status == status_EOF) {
      return status_OK;
    }
    if (d.status != status_OK) {
      return i_lexer_failRune(l, d, status_INVALIDUTF8, out);
    }
    if (inComment) {
      if (d.r == '\n') {
        return status_OK;
      }
      if (i_lexer_isControl(d.r) && d.r != '\r') {
        return i_lexer_failRune(l, d, status_BADCHAR, out);
      }
      i_lexer_advance(l, d);
    } else if (d.r == ' ' || d.r == '\r') {
      i_lexer_advance(l, d);
    } else if (d.r == '#') {
      inComment = true;
      i_lexer_advance(l, d);
    } else {
      return status_OK;
    }
  }
}

/* the cursor is on the opening quote. */
static inline
Status i_lexer_string(Lexer* l, Token t, Token* out) {
  i_lexer_step(l);
  while (true) {
    UTF8DecodeReturn d = i_lexer_decode(l);
    if (d.status == status_EOF) {
      return i_lexer_fail(l, t, l->pos, status_UNTERMINATEDSTRING, out);
    }
    if (d.status != status_OK) {
      return i_lexer_failDecode(l, t, d, out);
    }
    if (d.r == '"') {
      i_lexer_step(l);
      return i_lexer_finishAtom(l, t, tokenKind_STR, true, status_BADCHAR, out);
    }
    if (d.r == '\n') {
      return i_lexer_fail(l, t, l->pos, status_UNTERMINATEDSTRING, out);
    }
    if (d.r == '\\') {
      i_lexer_step(l);
      UTF8DecodeReturn e = i_lexer_decode(l);
      if (e.status == status_EOF || (e.status == status_OK && e.r == '\n')) {
        return i_lexer_fail(l, t, l->pos, status_UNTERMINATEDSTRING, out);
      }
      if (e.status != status_OK) {
        return i_lexer_failDecode(l, t, e, out);
      }
      if (!i_lexer_isEscape(e.r)) {
        return i_lexer_failAt(l, t, e, status_BADESCAPE, out);
      }
      i_lexer_advance(l, e);
    } else if (i_lexer_isControl(d.r) && d.r != '\r') {
      return i_lexer_failAt(l, t, d, status_BADCHAR, out);
    } else {
      i_lexer_advance(l, d);
    }
  }
}

/* consumes `digit {digit | '_'}`. Returns false, consuming nothing, if the
 * cursor is not on a digit. */
static inline
bool i_lexer_integer(Lexer* l) {
  if (!i_lexer_isDigit(i_lexer_byte(l, 0))) {
    return false;
  }
  i_lexer_step(l);
  while (i_lexer_isDigit(i_lexer_byte(l, 0)) || i_lexer_byte(l, 0) == '_') {
    i_lexer_step(l);
  }
  return true;
}

/* the cursor is on a digit, or on a '-' followed by a digit. */
static inline
Status i_lexer_number(Lexer* l, Token t, Token* out) {
  bool neg = i_lexer_byte(l, 0) == '-';
  if (neg) {
    i_lexer_step(l);
  }

  int prefix = i_lexer_byte(l, 1);
  if (!neg && i_lexer_byte(l, 0) == '0' && (prefix == 'x' || prefix == 'b')) {
    bool hex = prefix == 'x';
    bool real = false;
    i_lexer_step(l);
    i_lexer_step(l);
    while (i_lexer_isRadixDigit(i_lexer_byte(l, 0), hex)) {
      if (i_lexer_byte(l, 0) != '_') {
        real = true;
      }
      i_lexer_step(l);
    }
    if (!real) {
      return i_lexer_fail(l, t, l->pos, status_BADNUMBER, out);
    }
    return i_lexer_finishAtom(l, t, tokenKind_NUM, false, status_BADNUMBER, out);
  }

  i_lexer_integer(l);
  if (i_lexer_byte(l, 0) == '/') {
    i_lexer_step(l);
    if (!i_lexer_integer(l)) {
      return i_lexer_fail(l, t, l->pos, status_BADNUMBER, out);
    }
  } else if (i_lexer_byte(l, 0) == '.') {
    i_lexer_step(l);
    i_lexer_integer(l);
  }
  if (i_lexer_byte(l, 0) == 'e') {
    i_lexer_step(l);
    if (i_lexer_byte(l, 0) == '-') {
      i_lexer_step(l);
    }
    if (!i_lexer_integer(l)) {
      return i_lexer_fail(l, t, l->pos, status_BADNUMBER, out);
    }
  }
  if (i_lexer_byte(l, 0) == 'f') {
    i_lexer_step(l);
  }
  return i_lexer_finishAtom(l, t, tokenKind_NUM, false, status_BADNUMBER, out);

  /* NOTE(4): this is deliberately greedy. `3.` is a number (the dot is part
   * of it), so `3.x` is an error and not `NUM DOT ID`. The '-' is only part
   * of decimals, so `-0x1` is `-0` followed by an 'x', which is an error.
   * `3f` and `3.14f` are lexed, but giving them a meaning is up to whoever
   * converts the number. After the 'e' of an exponent there must be an
   * integer, and there is no '+'. */
}

static inline
Status i_lexer_identifier(Lexer* l, Token t, Token* out) {
  i_lexer_step(l);
  while (i_lexer_isIdContinue(i_lexer_byte(l, 0))) {
    i_lexer_step(l);
  }
  return i_lexer_finishAtom(l, t, tokenKind_ID, true, status_BADCHAR, out);
}

static inline
Status i_lexer_scan(Lexer* l, Token* out) {
  if (l->failed != status_OK) {
    *out = l->failedToken;
    return l->failed;
  }

  Status st = i_lexer_skip(l, out);
  if (st != status_OK) {
    return st;
  }

  Token t = { tokenKind_INVALID, l->pos, l->pos, l->line, l->col };
  UTF8DecodeReturn d = i_lexer_decode(l);
  if (d.status == status_EOF) {
    return i_lexer_emit(l, t, tokenKind_EOF, out);
  }
  if (d.status != status_OK) {
    return i_lexer_failDecode(l, t, d, out);
  }

  TokenKind punct = i_lexer_punctuation(d.r);
  if (punct != tokenKind_INVALID) {
    i_lexer_advance(l, d);
    return i_lexer_emit(l, t, punct, out);
  }
  if (d.r == '\n') {
    i_lexer_advance(l, d);
    l->line += 1;
    l->col = 0;
    return i_lexer_emit(l, t, tokenKind_NL, out);
  }
  if (d.r == '"') {
    return i_lexer_string(l, t, out);
  }
  if (i_lexer_isDigit(d.r) ||
      (d.r == '-' && i_lexer_isDigit(i_lexer_byte(l, 1)))) {
    return i_lexer_number(l, t, out);
  }
  if (i_lexer_isIdBegin(d.r)) {
    return i_lexer_identifier(l, t, out);
  }
  return i_lexer_failAt(l, t, d, status_BADCHAR, out);

  /* NOTE(5): `i_lexer_byte` returns -1 past the end of the input, which
   * is not a digit, nor a letter, nor anything else, so the helpers need no
   * bounds checks of their own. */
}

/* END: internal helpers */

/* Starts lexing `len` bytes at `src`, which must not be NULL (even when
 * `len` is 0) and must outlive the lexer. Offsets are `u32`, so it returns
 * `status_BADSIZE` if `len` does not fit in 32 bits. */
static inline
Status lexer_new(const u8* src, usize len, Lexer* out) {
  #if config_DEBUG
    if (src == NULL) {
      debug_FATAL("Source is null.");
    }
    if (out == NULL) {
      debug_FATAL("Output lexer is null.");
    }
  #endif

  if (len > (usize)UINT32_MAX) {
    return status_BADSIZE;
  }
  Token none = { tokenKind_EOF, 0, 0, 0, 0 };
  out->src = src;
  out->len = (u32)len;
  out->pos = 0;
  out->line = 0;
  out->col = 0;
  out->failed = status_OK;
  out->failedToken = none;
  return status_OK;

  /* SAFE(6): `len` was checked to fit in 32 bits. */
}

/* Reads the next token into `out`. See the top of the file for the
 * errors. After `tokenKind_EOF` or `tokenKind_INVALID` it keeps returning
 * the same token. */
static inline
Status lexer_next(Lexer* l, Token* out) {
  #if config_DEBUG
    if (l == NULL) {
      debug_FATAL("Lexer is null.");
    }
    if (out == NULL) {
      debug_FATAL("Output token is null.");
    }
  #endif
  return i_lexer_scan(l, out);
}

/* The token `lexer_next` would return, without consuming it. */
static inline
Status lexer_peek(const Lexer* l, Token* out) {
  #if config_DEBUG
    if (l == NULL) {
      debug_FATAL("Lexer is null.");
    }
    if (out == NULL) {
      debug_FATAL("Output token is null.");
    }
  #endif
  Lexer scratch = *l;
  return i_lexer_scan(&scratch, out); // NOTE(7)

  /* NOTE(7): `scratch` does not escape the function. Copying is cheap (the
   * lexer has no buffer) and makes it impossible for peeking to change the
   * lexer. A peeked invalid token does not make the original lexer fail, but
   * the next `lexer_next` finds the same error again. */
}

#endif
