/*
MIT License
Copyright 2026 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#include <string.h>
#include "../common.h"
#include "../../lib/ast.h"

#define LEN(array) ((int)(sizeof(array) / sizeof((array)[0])))

static u8 g_pao_heap[common_HEAPSIZE];

static Position pos(u32 line, u32 col) {
  Position p = { line, col };
  return p;
}

static Range rng(u32 bl, u32 bc, u32 el, u32 ec) {
  Range r = { { bl, bc }, { el, ec } };
  return r;
}

static bool sameRange(Range a, Range b) {
  return a.begin.line == b.begin.line && a.begin.col == b.begin.col &&
         a.end.line == b.end.line && a.end.col == b.end.col;
}

/* Lexes `src` up to the token number `index`, which is put in `out`.
 * False if the lexing stops before it. */
static bool tokenAt(const char* src, usize len, int index, Token* out) {
  Lexer l;
  Token t;
  int i = 0;
  if (lexer_new((const u8*)src, len, &l) != status_OK) {
    return false;
  }
  lexer_next(&l, &t);
  while (i < index) {
    if (t.kind == tokenKind_EOF || t.kind == tokenKind_INVALID) {
      return false;
    }
    lexer_next(&l, &t);
    i++;
  }
  *out = t;
  return true;
}

/* the range of the token number `index` of `src` is `want`. */
static bool rangeIs(const char* src, usize len, int index, Range want) {
  Token t;
  Range r = rng(9, 9, 9, 9);
  if (!tokenAt(src, len, index, &t)) {
    return false;
  }
  range_fromToken(&t, (const u8*)src, len, &r);
  if (!sameRange(r, want)) {
    printf("  \"%s\" token %d: got %u:%u-%u:%u\n", src, index,
           r.begin.line, r.begin.col, r.end.line, r.end.col);
    return false;
  }
  return true;
}

static bool rangeOf(const char* src, int index, Range want) {
  return rangeIs(src, strlen(src), index, want);
}

bool test_positionCompare(void) {
  return position_compare(pos(1, 5), pos(1, 5)) == order_EQUAL &&
         position_compare(pos(1, 5), pos(1, 6)) == order_LESS &&
         position_compare(pos(1, 6), pos(1, 5)) == order_GREATER &&
         position_compare(pos(1, 99), pos(2, 0)) == order_LESS &&
         position_compare(pos(2, 0), pos(1, 99)) == order_GREATER &&
         position_compare(pos(0, 0), pos(UINT32_MAX, UINT32_MAX)) == order_LESS;
}

bool test_rangeNewAndPredicates(void) {
  Range a = range_new(pos(0, 0), pos(0, 3));
  Range zero = range_new(pos(2, 4), pos(2, 4));
  Range none = range_none();
  return sameRange(a, rng(0, 0, 0, 3)) &&
         !range_isNone(a) && !range_isEmpty(a) &&
         !range_isNone(zero) && range_isEmpty(zero) &&
         range_isNone(none) && !range_isEmpty(none) &&
         !range_isNone(rng(0, 0, 0, 0)) && range_isEmpty(rng(0, 0, 0, 0));
}

bool test_rangeMerge(void) {
  Range a = rng(1, 2, 1, 6);
  Range b = rng(1, 4, 3, 1);
  Range c = rng(5, 0, 5, 3);
  Range none = range_none();
  Range zero = rng(2, 0, 2, 0);
  return
    /* none is the identity, on both sides */
    sameRange(range_merge(none, a), a) &&
    sameRange(range_merge(a, none), a) &&
    range_isNone(range_merge(none, none)) &&
    /* overlapping */
    sameRange(range_merge(a, b), rng(1, 2, 3, 1)) &&
    sameRange(range_merge(b, a), rng(1, 2, 3, 1)) &&
    /* disjoint: what is between is included */
    sameRange(range_merge(a, c), rng(1, 2, 5, 3)) &&
    sameRange(range_merge(c, a), rng(1, 2, 5, 3)) &&
    /* contained, and itself */
    sameRange(range_merge(rng(0, 0, 9, 9), a), rng(0, 0, 9, 9)) &&
    sameRange(range_merge(a, a), a) &&
    /* a range with no width is a real range, not the identity */
    sameRange(range_merge(zero, a), rng(1, 2, 2, 0)) &&
    sameRange(range_merge(zero, none), zero) &&
    /* associative */
    sameRange(range_merge(range_merge(a, b), c),
              range_merge(a, range_merge(b, c)));
}

bool test_fromTokenAscii(void) {
  const char* s = "foo [bar]";
  return rangeOf(s, 0, rng(0, 0, 0, 3)) &&
         rangeOf(s, 1, rng(0, 4, 0, 5)) &&
         rangeOf(s, 2, rng(0, 5, 0, 8)) &&
         rangeOf(s, 3, rng(0, 8, 0, 9)) &&
         rangeOf(s, 4, rng(0, 9, 0, 9));
}

bool test_fromTokenLines(void) {
  const char* s = "a # c\n  b\n\nd";
  /* a, NL, b, NL, NL, d, EOF */
  return rangeOf(s, 0, rng(0, 0, 0, 1)) &&
         rangeOf(s, 1, rng(0, 5, 1, 0)) &&
         rangeOf(s, 2, rng(1, 2, 1, 3)) &&
         rangeOf(s, 3, rng(1, 3, 2, 0)) &&
         rangeOf(s, 4, rng(2, 0, 3, 0)) &&
         rangeOf(s, 5, rng(3, 0, 3, 1)) &&
         rangeOf(s, 6, rng(3, 1, 3, 1));
}

/* columns count runes, not bytes */
bool test_fromTokenMultibyte(void) {
  /* "é" a\n  : 2 byte rune, then 4 byte rune */
  const char* s = "\"\xc3\xa9\" a \"\xf0\x9f\x98\x80\" b";
  return rangeOf(s, 0, rng(0, 0, 0, 3)) &&
         rangeOf(s, 1, rng(0, 4, 0, 5)) &&
         rangeOf(s, 2, rng(0, 6, 0, 9)) &&
         rangeOf(s, 3, rng(0, 10, 0, 11)) &&
         rangeOf(s, 4, rng(0, 11, 0, 11));
}

bool test_fromTokenCrlf(void) {
  const char* s = "a\r\nb";
  return rangeOf(s, 0, rng(0, 0, 0, 1)) &&
         rangeOf(s, 1, rng(0, 2, 1, 0)) &&
         rangeOf(s, 2, rng(1, 0, 1, 1)) &&
         rangeOf(s, 3, rng(1, 1, 1, 1));
}

bool test_fromTokenEmptySource(void) {
  return rangeIs("", 0, 0, rng(0, 0, 0, 0));
}

/* the offending rune is always in an invalid token, with one column. */
bool test_fromTokenInvalid(void) {
  return
    /* a lone continuation byte */
    rangeIs("\x80", 1, 0, rng(0, 0, 0, 1)) &&
    /* after some good runes of the same lexeme */
    rangeIs("ab\x80", 3, 0, rng(0, 0, 0, 3)) &&
    /* overlong, consumed as 2 bytes */
    rangeIs("\xc0\x80", 2, 0, rng(0, 0, 0, 1)) &&
    /* truncated sequence at the end of a string */
    rangeIs("\"x\xe2\x82", 4, 0, rng(0, 0, 0, 3)) &&
    /* valid rune that is not allowed there */
    rangeIs("\xc3\xa9", 2, 0, rng(0, 0, 0, 1)) &&
    /* tab, after a token */
    rangeIs("a \t", 3, 1, rng(0, 2, 0, 3)) &&
    /* unterminated string, in a later line */
    rangeIs("a\n\"ab", 5, 2, rng(1, 0, 1, 3));
}

bool test_snprint(void) {
  char buf[64];
  struct { Range r; const char* want; } cases[] = {
    { rng(0, 0, 0, 3), "1:1-1:4" },
    { rng(0, 3, 2, 6), "1:4-3:7" },
    { rng(0, 3, 1, 0), "1:4-2:1" },
    { rng(1, 0, 1, 0), "2:1" },
    { rng(0, 0, 0, 0), "1:1" },
    { rng(UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX),
      "4294967296:4294967296" },
    { rng(0, 0, UINT32_MAX, UINT32_MAX), "1:1-4294967296:4294967296" },
  };
  int i = 0;
  while (i < LEN(cases)) {
    usize want = strlen(cases[i].want);
    usize n;
    memset(buf, '#', sizeof(buf));
    n = range_snprint(&cases[i].r, buf, sizeof(buf));
    if (n != want || memcmp(buf, cases[i].want, want) != 0 || buf[n] != '#') {
      return false;
    }
    i++;
  }
  Range none = range_none();
  return range_snprint(&none, buf, sizeof(buf)) == 1 && buf[0] == '?';
}

/* all or nothing */
bool test_snprintSize(void) {
  char buf[8];
  Range r = rng(0, 0, 0, 3);
  memset(buf, '#', sizeof(buf));
  if (range_snprint(&r, buf, 7) != 7 || memcmp(buf, "1:1-1:4", 7) != 0) {
    return false;
  }
  memset(buf, '#', sizeof(buf));
  if (range_snprint(&r, buf, 6) != 0 || buf[0] != '#') {
    return false;
  }
  return range_snprint(&r, buf, 0) == 0;
}

/* the lexeme of the token `index` of `src`, made with `mem`. */
static Status lexemeAt(const IAllocator* mem, const char* src, int index,
                       Lexeme* out) {
  Token t;
  if (!tokenAt(src, strlen(src), index, &t)) {
    return status_EOF;
  }
  return lexeme_new(mem, &t, (const u8*)src, strlen(src), out);
}

bool test_lexemeNewFree(void) {
  char src[] = "foo \"\xc3\xa9\"\n";
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  Lexeme id, str, nl, eof;
  if (lexemeAt(&mem, src, 0, &id) != status_OK ||
      lexemeAt(&mem, src, 1, &str) != status_OK ||
      lexemeAt(&mem, src, 2, &nl) != status_OK ||
      lexemeAt(&mem, src, 3, &eof) != status_OK) {
    return false;
  }
  /* the source is no longer needed */
  memset(src, 'z', sizeof(src) - 1);

  bool ok =
    id.kind == tokenKind_ID && id.text.length == 3 &&
    memcmp(id.text.data, "foo", 3) == 0 &&
    sameRange(id.range, rng(0, 0, 0, 3)) &&
    str.kind == tokenKind_STR && str.text.length == 4 &&
    memcmp(str.text.data, "\"\xc3\xa9\"", 4) == 0 &&
    sameRange(str.range, rng(0, 4, 0, 7)) &&
    nl.kind == tokenKind_NL && nl.text.length == 1 && nl.text.data[0] == '\n' &&
    sameRange(nl.range, rng(0, 7, 1, 0)) &&
    eof.kind == tokenKind_EOF && eof.text.length == 0 && eof.text.data == NULL &&
    sameRange(eof.range, rng(1, 0, 1, 0));
  ok = ok && mem.info(mem.heap).used != 0;
  ok = ok && lexeme_free(&mem, id) == status_OK &&
       lexeme_free(&mem, str) == status_OK &&
       lexeme_free(&mem, nl) == status_OK &&
       lexeme_free(&mem, eof) == status_OK;
  return ok && mem.info(mem.heap).used == 0;
}

/* the text of an invalid token is kept too */
bool test_lexemeInvalid(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  Lexeme lx;
  if (lexemeAt(&mem, "a \"ab", 1, &lx) != status_OK) {
    return false;
  }
  bool ok = lx.kind == tokenKind_INVALID && lx.text.length == 3 &&
            memcmp(lx.text.data, "\"ab", 3) == 0 &&
            sameRange(lx.range, rng(0, 2, 0, 5));
  return ok && lexeme_free(&mem, lx) == status_OK &&
         mem.info(mem.heap).used == 0;
}

bool test_lexemeOutOfMemory(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, 0);
  Lexeme lx;
  lx.kind = tokenKind_EOF;
  lx.text.data = "keep";
  lx.text.length = 4;
  lx.range = rng(7, 7, 7, 7);
  Status st = lexemeAt(&mem, "foo", 0, &lx);
  /* `out` is left alone, and nothing leaks */
  return st == status_OUTOFMEMORY && lx.kind == tokenKind_EOF &&
         lx.text.length == 4 && sameRange(lx.range, rng(7, 7, 7, 7)) &&
         mem.info(mem.heap).used == 0;
}

/* BEGIN: DRIVER CODE */
Tester tests[] = {
  {"test_ast_positionCompare", test_positionCompare},
  {"test_ast_rangeNewAndPredicates", test_rangeNewAndPredicates},
  {"test_ast_rangeMerge", test_rangeMerge},
  {"test_ast_fromTokenAscii", test_fromTokenAscii},
  {"test_ast_fromTokenLines", test_fromTokenLines},
  {"test_ast_fromTokenMultibyte", test_fromTokenMultibyte},
  {"test_ast_fromTokenCrlf", test_fromTokenCrlf},
  {"test_ast_fromTokenEmptySource", test_fromTokenEmptySource},
  {"test_ast_fromTokenInvalid", test_fromTokenInvalid},
  {"test_ast_snprint", test_snprint},
  {"test_ast_snprintSize", test_snprintSize},
  {"test_ast_lexemeNewFree", test_lexemeNewFree},
  {"test_ast_lexemeInvalid", test_lexemeInvalid},
  {"test_ast_lexemeOutOfMemory", test_lexemeOutOfMemory},
};

int main(void) {
  run_tests("ast", tests, LEN(tests));
}
/* END: DRIVER CODE */
