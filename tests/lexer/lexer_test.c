#include <string.h>
#include "../common.h"
#include "../../lib/lexer.h"

/* An expected token, with the status `lexer_next` must return with it. */
typedef struct {
  TokenKind kind;
  u32 start;
  u32 end;
  u32 line;
  u32 col;
  Status status;
} Exp;

Exp tok_ok(TokenKind kind, u32 start, u32 end, u32 line, u32 col) {
  Exp e = { kind, start, end, line, col, status_OK };
  return e;
}

Exp tok_bad(Status status, u32 start, u32 end, u32 line, u32 col) {
  Exp e = { tokenKind_INVALID, start, end, line, col, status };
  return e;
}

#define LEN(array) ((int)(sizeof(array) / sizeof((array)[0])))

bool tok_matches(Token t, Status st, Exp e) {
  /* the status is not OK if, and only if, the token is invalid */
  if ((st != status_OK) != (t.kind == tokenKind_INVALID)) {
    return false;
  }
  return st == e.status && t.kind == e.kind &&
         t.start == e.start && t.end == e.end &&
         t.line == e.line && t.col == e.col;
}

/* Lexes `src` and compares it with the `n` expected tokens. When the last
 * one is EOF or INVALID it also checks that it is repeated forever. */
bool check(const char* src, usize len, const Exp* exp, int n) {
  Lexer l;
  Token t;
  int i = 0;
  if (lexer_new((const u8*)src, len, &l) != status_OK) {
    return false;
  }
  while (i < n) {
    Status st = lexer_next(&l, &t);
    if (!tok_matches(t, st, exp[i])) {
      printf("  \"%.*s\": token %d: got %s [%u,%u) %u:%u status %d\n",
             (int)len, src, i, tokenKind_name(t.kind),
             t.start, t.end, t.line, t.col, (int)st);
      return false;
    }
    i++;
  }
  Exp last = exp[n - 1];
  if (last.kind == tokenKind_EOF || last.kind == tokenKind_INVALID) {
    i = 0;
    while (i < 3) {
      Status st = lexer_next(&l, &t);
      if (!tok_matches(t, st, last)) {
        return false;
      }
      i++;
    }
  }
  return true;
}

bool checkStr(const char* src, const Exp* exp, int n) {
  return check(src, strlen(src), exp, n);
}

/* `src` is ASCII and is a single token of the given kind, covering it all. */
bool checkWhole(const char* src, TokenKind kind) {
  u32 n = (u32)strlen(src);
  Exp e[2] = { tok_ok(kind, 0, n, 0, 0), tok_ok(tokenKind_EOF, n, n, 0, n) };
  return checkStr(src, e, 2);
}

/* `src` is an invalid token that starts at the beginning of the source. */
bool checkBad(const char* src, usize len, Status why, u32 end) {
  Exp e[1] = { tok_bad(why, 0, end, 0, 0) };
  return check(src, len, e, 1);
}

bool checkBadStr(const char* src, Status why, u32 end) {
  return checkBad(src, strlen(src), why, end);
}

bool test_new(void) {
  Lexer l;
  u8 byte = 0;
  if (lexer_new(&byte, 0, &l) != status_OK) {
    return false;
  }
  if (l.pos != 0 || l.line != 0 || l.col != 0) {
    return false;
  }
  if (sizeof(usize) > 4) {
    if (lexer_new(&byte, (usize)UINT32_MAX + 1, &l) != status_BADSIZE) {
      return false;
    }
    /* nothing is read until the first call to `lexer_next` */
    if (lexer_new(&byte, (usize)UINT32_MAX, &l) != status_OK) {
      return false;
    }
  }
  return true;
}

bool test_kindNames(void) {
  int i = 0;
  while (i <= (int)tokenKind_INVALID) {
    const char* a = tokenKind_name((TokenKind)i);
    int j = 0;
    if (a == NULL || a[0] == '\0' || strcmp(a, "?") == 0) {
      return false;
    }
    while (j < i) {
      if (strcmp(a, tokenKind_name((TokenKind)j)) == 0) {
        return false;
      }
      j++;
    }
    i++;
  }
  return true;
}

bool test_empty(void) {
  Exp a[] = { tok_ok(tokenKind_EOF, 0, 0, 0, 0) };
  Exp b[] = { tok_ok(tokenKind_EOF, 3, 3, 0, 3) };
  Exp c[] = { tok_ok(tokenKind_EOF, 4, 4, 0, 4) };
  return checkStr("", a, LEN(a)) &&
         checkStr("   ", b, LEN(b)) &&
         checkStr(" \r \r", c, LEN(c));
}

bool test_punctuation(void) {
  Exp all[] = {
    tok_ok(tokenKind_BACKSLASH, 0, 1, 0, 0),
    tok_ok(tokenKind_SEMI, 1, 2, 0, 1),
    tok_ok(tokenKind_COLON, 2, 3, 0, 2),
    tok_ok(tokenKind_DOT, 3, 4, 0, 3),
    tok_ok(tokenKind_EQUALS, 4, 5, 0, 4),
    tok_ok(tokenKind_COMMA, 5, 6, 0, 5),
    tok_ok(tokenKind_LBRACKET, 6, 7, 0, 6),
    tok_ok(tokenKind_RBRACKET, 7, 8, 0, 7),
    tok_ok(tokenKind_EOF, 8, 8, 0, 8),
  };
  Exp list[] = {
    tok_ok(tokenKind_LBRACKET, 0, 1, 0, 0),
    tok_ok(tokenKind_ID, 1, 2, 0, 1),
    tok_ok(tokenKind_COMMA, 2, 3, 0, 2),
    tok_ok(tokenKind_ID, 4, 5, 0, 4),
    tok_ok(tokenKind_RBRACKET, 5, 6, 0, 5),
    tok_ok(tokenKind_EOF, 6, 6, 0, 6),
  };
  Exp dot[] = {
    tok_ok(tokenKind_ID, 0, 1, 0, 0),
    tok_ok(tokenKind_DOT, 1, 2, 0, 1),
    tok_ok(tokenKind_ID, 2, 3, 0, 2),
    tok_ok(tokenKind_EOF, 3, 3, 0, 3),
  };
  Exp colon[] = {
    tok_ok(tokenKind_ID, 0, 1, 0, 0),
    tok_ok(tokenKind_COLON, 1, 2, 0, 1),
    tok_ok(tokenKind_ID, 2, 3, 0, 2),
    tok_ok(tokenKind_EOF, 3, 3, 0, 3),
  };
  Exp named[] = {
    tok_ok(tokenKind_ID, 0, 1, 0, 0),
    tok_ok(tokenKind_EQUALS, 2, 3, 0, 2),
    tok_ok(tokenKind_NUM, 4, 8, 0, 4),
    tok_ok(tokenKind_EOF, 8, 8, 0, 8),
  };
  Exp tight[] = {
    tok_ok(tokenKind_ID, 0, 1, 0, 0),
    tok_ok(tokenKind_EQUALS, 1, 2, 0, 1),
    tok_ok(tokenKind_NUM, 2, 3, 0, 2),
    tok_ok(tokenKind_EOF, 3, 3, 0, 3),
  };
  /* a number after a dot is a number, whatever that means to the parser */
  Exp field[] = {
    tok_ok(tokenKind_ID, 0, 1, 0, 0),
    tok_ok(tokenKind_DOT, 1, 2, 0, 1),
    tok_ok(tokenKind_NUM, 2, 5, 0, 2),
    tok_ok(tokenKind_EOF, 5, 5, 0, 5),
  };
  Exp spaced[] = {
    tok_ok(tokenKind_NUM, 0, 1, 0, 0),
    tok_ok(tokenKind_DOT, 2, 3, 0, 2),
    tok_ok(tokenKind_ID, 4, 5, 0, 4),
    tok_ok(tokenKind_EOF, 5, 5, 0, 5),
  };
  return checkStr("\\;:.=,[]", all, LEN(all)) &&
         checkStr("[a, b]", list, LEN(list)) &&
         checkStr("a.b", dot, LEN(dot)) &&
         checkStr("a:b", colon, LEN(colon)) &&
         checkStr("x = 3.14", named, LEN(named)) &&
         checkStr("x=3", tight, LEN(tight)) &&
         checkStr("a.0.1", field, LEN(field)) &&
         checkStr("3 . x", spaced, LEN(spaced));
}

/* atoms glued to delimiters, which are valid */
bool test_delimiters(void) {
  Exp call[] = {
    tok_ok(tokenKind_ID, 0, 1, 0, 0),
    tok_ok(tokenKind_LBRACKET, 1, 2, 0, 1),
    tok_ok(tokenKind_ID, 2, 3, 0, 2),
    tok_ok(tokenKind_RBRACKET, 3, 4, 0, 3),
    tok_ok(tokenKind_EOF, 4, 4, 0, 4),
  };
  Exp str[] = {
    tok_ok(tokenKind_STR, 0, 3, 0, 0),
    tok_ok(tokenKind_LBRACKET, 3, 4, 0, 3),
    tok_ok(tokenKind_ID, 4, 5, 0, 4),
    tok_ok(tokenKind_RBRACKET, 5, 6, 0, 5),
    tok_ok(tokenKind_EOF, 6, 6, 0, 6),
  };
  Exp num[] = {
    tok_ok(tokenKind_NUM, 0, 1, 0, 0),
    tok_ok(tokenKind_COLON, 1, 2, 0, 1),
    tok_ok(tokenKind_NUM, 2, 3, 0, 2),
    tok_ok(tokenKind_COMMA, 3, 4, 0, 3),
    tok_ok(tokenKind_NUM, 4, 5, 0, 4),
    tok_ok(tokenKind_SEMI, 5, 6, 0, 5),
    tok_ok(tokenKind_BACKSLASH, 6, 7, 0, 6),
    tok_ok(tokenKind_EOF, 7, 7, 0, 7),
  };
  Exp comment[] = {
    tok_ok(tokenKind_ID, 0, 1, 0, 0),
    tok_ok(tokenKind_EOF, 3, 3, 0, 3),
  };
  return checkStr("f[x]", call, LEN(call)) &&
         checkStr("\"a\"[b]", str, LEN(str)) &&
         checkStr("1:2,3;\\", num, LEN(num)) &&
         checkStr("a#c", comment, LEN(comment));
}

bool test_identifiers(void) {
  const char* valid[] = {
    "a", "_", "__", "a_", "A_B", "to-string", "equal?", "set!", "a?b-c!_",
    "less-or-equals?", "true", "nil",
  };
  int i = 0;
  while (i < LEN(valid)) {
    if (!checkWhole(valid[i], tokenKind_ID)) {
      return false;
    }
    i++;
  }
  Exp args[] = {
    tok_ok(tokenKind_LBRACKET, 0, 1, 0, 0),
    tok_ok(tokenKind_ID, 1, 4, 0, 1),
    tok_ok(tokenKind_ID, 5, 6, 0, 5),
    tok_ok(tokenKind_NUM, 7, 9, 0, 7),
    tok_ok(tokenKind_RBRACKET, 9, 10, 0, 9),
    tok_ok(tokenKind_EOF, 10, 10, 0, 10),
  };
  Exp glued[] = {
    tok_ok(tokenKind_ID, 0, 1, 0, 0),
    tok_ok(tokenKind_EQUALS, 1, 2, 0, 1),
    tok_ok(tokenKind_NUM, 2, 4, 0, 2),
    tok_ok(tokenKind_EOF, 4, 4, 0, 4),
  };
  return checkStr("[sub a -1]", args, LEN(args)) &&
         checkStr("x=-3", glued, LEN(glued));
}

/* identifiers have no digits, and are not glued to anything */
bool test_badIdentifiers(void) {
  return checkBadStr("a-1", status_BADCHAR, 3) &&
         checkBadStr("x1", status_BADCHAR, 2) &&
         checkBadStr("_1", status_BADCHAR, 2) &&
         checkBadStr("a\"b\"", status_BADCHAR, 2) &&
         checkBadStr("a@", status_BADCHAR, 2) &&
         checkBadStr("?a", status_BADCHAR, 1) &&
         checkBadStr("!x", status_BADCHAR, 1) &&
         checkBadStr("-a", status_BADCHAR, 1);
}

bool test_numbers(void) {
  const char* valid[] = {
    "0", "42", "-5", "-0", "007",
    "1_000", "1_", "1__0", "1_000.5_0", "1_.5",
    "0xff", "0xFF_ff", "0x0_", "0x_0", "0b1010", "0b_1", "0b1_",
    "0xffffffffffffffffffffffff",
    "0b11111111111111111111111111111111111111111111111111111111111111111",
    "3.14", "3.", "3.0", "3.e5", "-3.14",
    "1/2", "-3/4", "22/7",
    "1e5", "1e-5", "1.5e-3", "1/2e3", "1e05",
    "3.14f", "3f", "-1.5e-3f", "1e5f",
  };
  int i = 0;
  while (i < LEN(valid)) {
    if (!checkWhole(valid[i], tokenKind_NUM)) {
      return false;
    }
    i++;
  }
  return true;
}

bool test_badNumbers(void) {
  /* missing digits: covers what was consumed */
  bool missing =
    checkBadStr("0x", status_BADNUMBER, 2) &&
    checkBadStr("0x_", status_BADNUMBER, 3) &&
    checkBadStr("0b", status_BADNUMBER, 2) &&
    checkBadStr("0b2", status_BADNUMBER, 2) &&
    checkBadStr("0xg", status_BADNUMBER, 2) &&
    checkBadStr("1/", status_BADNUMBER, 2) &&
    checkBadStr("1/a", status_BADNUMBER, 2) &&
    checkBadStr("1/-2", status_BADNUMBER, 2) &&
    checkBadStr("1e", status_BADNUMBER, 2) &&
    checkBadStr("1e-", status_BADNUMBER, 3) &&
    checkBadStr("1e+5", status_BADNUMBER, 2);
  /* glued to something else: covers up to the offending rune */
  bool glued =
    checkBadStr("0b12", status_BADNUMBER, 4) &&
    checkBadStr("0b1f", status_BADNUMBER, 4) &&
    checkBadStr("0b1e5", status_BADNUMBER, 4) &&
    checkBadStr("0x1g", status_BADNUMBER, 4) &&
    checkBadStr("0x1.5", status_BADNUMBER, 4) &&
    checkBadStr("-0x1", status_BADNUMBER, 3) &&
    checkBadStr("1E5", status_BADNUMBER, 2) &&
    checkBadStr("3x", status_BADNUMBER, 2) &&
    checkBadStr("3ff", status_BADNUMBER, 3) &&
    checkBadStr("3.14fx", status_BADNUMBER, 6) &&
    checkBadStr("1_a", status_BADNUMBER, 3) &&
    checkBadStr("1-2", status_BADNUMBER, 2) &&
    checkBadStr("1\"a\"", status_BADNUMBER, 2) &&
    checkBadStr("1\t", status_BADNUMBER, 2) &&
    checkBadStr("3.x", status_BADNUMBER, 3) &&
    checkBadStr("3.0.1", status_BADNUMBER, 4) &&
    checkBadStr("1.5/2", status_BADNUMBER, 4) &&
    checkBadStr("1/2.", status_BADNUMBER, 4) &&
    checkBadStr("1/2/3", status_BADNUMBER, 4);
  return missing && glued;
}

/* the minus only starts a token when it starts a number */
bool test_minus(void) {
  Exp space[] = {
    tok_ok(tokenKind_ID, 0, 1, 0, 0),
    tok_ok(tokenKind_NUM, 2, 4, 0, 2),
    tok_ok(tokenKind_EOF, 4, 4, 0, 4),
  };
  return checkBadStr("-", status_BADCHAR, 1) &&
         checkBadStr("-x", status_BADCHAR, 1) &&
         checkBadStr("- 1", status_BADCHAR, 1) &&
         checkBadStr("--1", status_BADCHAR, 1) &&
         checkBadStr("-.5", status_BADCHAR, 1) &&
         checkStr("a -1", space, LEN(space));
}

bool test_strings(void) {
  const char* valid[] = {
    "\"abc\"",
    "\"\"",
    "\"a b # not a comment [ ]\"",
    "\"a\\nb\\t\\\"\\\\\\'\"",
    "\"\\\\\"",
    "\"a\rb\"",
  };
  int i = 0;
  while (i < LEN(valid)) {
    if (!checkWhole(valid[i], tokenKind_STR)) {
      return false;
    }
    i++;
  }
  /* 13 bytes, but 9 runes */
  Exp unicode[] = {
    tok_ok(tokenKind_STR, 0, 13, 0, 0),
    tok_ok(tokenKind_EOF, 13, 13, 0, 9),
  };
  return checkStr("\"h\xc3\xa9llo \xf0\x9f\x98\x80\"", unicode, LEN(unicode));
}

bool test_badStrings(void) {
  bool unterminated =
    checkBadStr("\"abc", status_UNTERMINATEDSTRING, 4) &&
    checkBadStr("\"", status_UNTERMINATEDSTRING, 1) &&
    checkBadStr("\"abc\nxyz", status_UNTERMINATEDSTRING, 4) &&
    checkBadStr("\"abc\\", status_UNTERMINATEDSTRING, 5) &&
    checkBadStr("\"abc\\\n\"", status_UNTERMINATEDSTRING, 5);
  bool escapes =
    checkBadStr("\"a\\qb\"", status_BADESCAPE, 4) &&
    checkBadStr("\"a\\rb\"", status_BADESCAPE, 4) &&
    checkBadStr("\"\\\t\"", status_BADESCAPE, 3) &&
    checkBadStr("\"\\\xc3\xa9\"", status_BADESCAPE, 4);
  bool chars =
    checkBadStr("\"a\tb\"", status_BADCHAR, 3) &&
    checkBadStr("\"a\x01" "b\"", status_BADCHAR, 3) &&
    checkBadStr("\"a\x7f" "b\"", status_BADCHAR, 3) &&
    checkBad("\"a\0b\"", 5, status_BADCHAR, 3) &&
    checkBadStr("\"a\"\"b\"", status_BADCHAR, 4);
  bool utf8 =
    checkBadStr("\"a\xff" "b\"", status_INVALIDUTF8, 3);

  /* the error is on the second line */
  Exp second[] = {
    tok_ok(tokenKind_ID, 0, 1, 0, 0),
    tok_ok(tokenKind_NL, 1, 2, 0, 1),
    tok_bad(status_UNTERMINATEDSTRING, 4, 6, 1, 2),
  };
  return unterminated && escapes && chars && utf8 &&
         checkStr("a\n  \"x", second, LEN(second));
}

bool test_comments(void) {
  Exp simple[] = {
    tok_ok(tokenKind_ID, 0, 1, 0, 0),
    tok_ok(tokenKind_NL, 6, 7, 0, 6),
    tok_ok(tokenKind_ID, 7, 8, 1, 0),
    tok_ok(tokenKind_EOF, 8, 8, 1, 1),
  };
  Exp atEOF[] = {
    tok_ok(tokenKind_ID, 0, 1, 0, 0),
    tok_ok(tokenKind_EOF, 4, 4, 0, 4),
  };
  /* the comment has a two bytes rune */
  Exp unicode[] = {
    tok_ok(tokenKind_NL, 4, 5, 0, 3),
    tok_ok(tokenKind_ID, 5, 6, 1, 0),
    tok_ok(tokenKind_EOF, 6, 6, 1, 1),
  };
  Exp empty[] = {
    tok_ok(tokenKind_NL, 1, 2, 0, 1),
    tok_ok(tokenKind_EOF, 2, 2, 1, 0),
  };
  Exp cr[] = {
    tok_ok(tokenKind_NL, 4, 5, 0, 4),
    tok_ok(tokenKind_ID, 5, 6, 1, 0),
  };
  Exp quote[] = {
    tok_ok(tokenKind_EOF, 6, 6, 0, 6),
  };
  Exp tab[] = { tok_bad(status_BADCHAR, 3, 4, 0, 3) };
  Exp control[] = { tok_bad(status_BADCHAR, 2, 3, 0, 2) };
  Exp bad[] = { tok_bad(status_INVALIDUTF8, 2, 3, 0, 2) };
  return checkStr("a # hi\nb", simple, LEN(simple)) &&
         checkStr("a #x", atEOF, LEN(atEOF)) &&
         checkStr("# \xc3\xa9\nb", unicode, LEN(unicode)) &&
         checkStr("#\n", empty, LEN(empty)) &&
         checkStr("# a\r\nb", cr, LEN(cr)) &&
         checkStr("# \"abc", quote, LEN(quote)) &&
         checkStr("# a\tb", tab, LEN(tab)) &&
         checkStr("# \x01", control, LEN(control)) &&
         checkStr("# \xff", bad, LEN(bad));
}

bool test_lines(void) {
  Exp blank[] = {
    tok_ok(tokenKind_ID, 0, 1, 0, 0),
    tok_ok(tokenKind_NL, 1, 2, 0, 1),
    tok_ok(tokenKind_NL, 2, 3, 1, 0),
    tok_ok(tokenKind_ID, 5, 6, 2, 2),
    tok_ok(tokenKind_EOF, 6, 6, 2, 3),
  };
  Exp crlf[] = {
    tok_ok(tokenKind_ID, 0, 1, 0, 0),
    tok_ok(tokenKind_NL, 2, 3, 0, 2),
    tok_ok(tokenKind_ID, 3, 4, 1, 0),
    tok_ok(tokenKind_EOF, 4, 4, 1, 1),
  };
  Exp newlines[] = {
    tok_ok(tokenKind_NL, 0, 1, 0, 0),
    tok_ok(tokenKind_NL, 1, 2, 1, 0),
    tok_ok(tokenKind_EOF, 2, 2, 2, 0),
  };
  /* a lone '\r' is a space, and takes a column */
  Exp cr[] = {
    tok_ok(tokenKind_ID, 0, 1, 0, 0),
    tok_ok(tokenKind_ID, 4, 5, 0, 4),
    tok_ok(tokenKind_EOF, 5, 5, 0, 5),
  };
  /* columns count runes, not bytes */
  Exp runes[] = {
    tok_ok(tokenKind_STR, 0, 4, 0, 0),
    tok_ok(tokenKind_ID, 5, 6, 0, 4),
    tok_ok(tokenKind_EOF, 6, 6, 0, 5),
  };
  Exp indent[] = {
    tok_ok(tokenKind_ID, 0, 3, 0, 0),
    tok_ok(tokenKind_NL, 3, 4, 0, 3),
    tok_ok(tokenKind_ID, 6, 7, 1, 2),
    tok_ok(tokenKind_NL, 7, 8, 1, 3),
    tok_ok(tokenKind_ID, 12, 13, 2, 4),
    tok_ok(tokenKind_EOF, 13, 13, 2, 5),
  };
  return checkStr("a\n\n  b", blank, LEN(blank)) &&
         checkStr("a\r\nb", crlf, LEN(crlf)) &&
         checkStr("\n\n", newlines, LEN(newlines)) &&
         checkStr("a \r b", cr, LEN(cr)) &&
         checkStr("\"\xc3\xa9\" a", runes, LEN(runes)) &&
         checkStr("let\n  x\n    y", indent, LEN(indent));
}

bool test_badChars(void) {
  const char* symbols[] = {
    "@", "{", "}", "+", "'", "~", "(", ")", "$", "|", "/", "<", ">", "*",
    "%", "&", "^", "`", "\t", "\x01", "\x1f", "\x7f", "?", "!",
  };
  int i = 0;
  while (i < LEN(symbols)) {
    if (!checkBadStr(symbols[i], status_BADCHAR, 1)) {
      return false;
    }
    i++;
  }
  Exp nonAscii[] = {
    tok_ok(tokenKind_ID, 0, 1, 0, 0),
    tok_bad(status_BADCHAR, 2, 4, 0, 2),
  };
  Exp afterSpace[] = { tok_bad(status_BADCHAR, 1, 3, 0, 1) };
  return checkBad("\0", 1, status_BADCHAR, 1) &&
         checkBadStr("\xc3\xa9", status_BADCHAR, 2) &&
         checkBadStr("\xf0\x9f\x98\x80", status_BADCHAR, 4) &&
         checkBadStr("\ta", status_BADCHAR, 1) &&
         /* the tab is the offending rune, so it is covered along with `a` */
         checkBadStr("a\tb", status_BADCHAR, 2) &&
         checkStr("a \xc3\xa9", nonAscii, LEN(nonAscii)) &&
         checkStr(" \xc3\xa9", afterSpace, LEN(afterSpace));
}

bool test_badUTF8(void) {
  Exp afterSpace[] = { tok_bad(status_INVALIDUTF8, 1, 2, 0, 1) };
  return checkBadStr("\xff", status_INVALIDUTF8, 1) &&
         checkBadStr("\x80", status_INVALIDUTF8, 1) &&
         /* truncated sequence */
         checkBadStr("\xe2\x82", status_INVALIDUTF8, 1) &&
         /* the whole ill-formed sequence, when the decoder reads it */
         checkBadStr("\xc0\xaf", status_INVALIDUTF8, 2) &&
         checkBadStr("\xed\xa0\x80", status_INVALIDUTF8, 3) &&
         /* after an atom */
         checkBadStr("a\xff", status_INVALIDUTF8, 2) &&
         checkBadStr("1\xff", status_INVALIDUTF8, 2) &&
         checkBadStr("\"\xff\"", status_INVALIDUTF8, 2) &&
         checkStr(" \xff", afterSpace, LEN(afterSpace));
}

bool test_idempotent(void) {
  Exp eof[] = {
    tok_ok(tokenKind_ID, 0, 1, 0, 0),
    tok_ok(tokenKind_EOF, 1, 1, 0, 1),
  };
  /* the `b` is never reached */
  Exp invalid[] = {
    tok_ok(tokenKind_ID, 0, 1, 0, 0),
    tok_bad(status_BADCHAR, 2, 3, 0, 2),
  };
  return checkStr("a", eof, LEN(eof)) &&
         checkStr("a @ b", invalid, LEN(invalid));
}

bool sameToken(Token a, Token b) {
  return a.kind == b.kind && a.start == b.start && a.end == b.end &&
         a.line == b.line && a.col == b.col;
}

bool test_peek(void) {
  Lexer l;
  Token p;
  Token q;
  Token n;
  Status sp;
  Status sn;
  const char* src = "a b";
  if (lexer_new((const u8*)src, 3, &l) != status_OK) {
    return false;
  }

  sp = lexer_peek(&l, &p);
  if (sp != status_OK || !tok_matches(p, sp, tok_ok(tokenKind_ID, 0, 1, 0, 0))) {
    return false;
  }
  /* peeking does not consume */
  sp = lexer_peek(&l, &q);
  if (sp != status_OK || !sameToken(p, q) || l.pos != 0 || l.col != 0) {
    return false;
  }
  sn = lexer_next(&l, &n);
  if (sn != status_OK || !sameToken(p, n)) {
    return false;
  }
  sp = lexer_peek(&l, &p);
  sn = lexer_next(&l, &n);
  if (!tok_matches(n, sn, tok_ok(tokenKind_ID, 2, 3, 0, 2)) ||
      !sameToken(p, n)) {
    return false;
  }
  sp = lexer_peek(&l, &p);
  sn = lexer_next(&l, &n);
  return tok_matches(n, sn, tok_ok(tokenKind_EOF, 3, 3, 0, 3)) &&
         sameToken(p, n);
}

bool test_peekInvalid(void) {
  Lexer l;
  Token p;
  Token n;
  Status sp;
  Status sn;
  const char* src = "a @";
  if (lexer_new((const u8*)src, 3, &l) != status_OK) {
    return false;
  }
  sn = lexer_next(&l, &n);
  sp = lexer_peek(&l, &p);
  if (sp != status_BADCHAR || p.kind != tokenKind_INVALID) {
    return false;
  }
  /* the lexer was not touched by the peek, and finds the error again */
  if (l.pos != 1 || l.col != 1) {
    return false;
  }
  sn = lexer_next(&l, &n);
  return sn == status_BADCHAR && sameToken(p, n);
}

/* lexes it all. Returns the number of tokens, or -1 on an error. */
int lexAll(const char* src) {
  Lexer l;
  Token t;
  int count = 0;
  if (lexer_new((const u8*)src, strlen(src), &l) != status_OK) {
    return -1;
  }
  while (true) {
    Status st = lexer_next(&l, &t);
    if (st != status_OK || t.kind == tokenKind_INVALID) {
      printf("  invalid token at %u:%u, status %d\n", t.line, t.col, (int)st);
      return -1;
    }
    if (t.kind == tokenKind_EOF) {
      return count;
    }
    count++;
  }
}

/* every example in docs/language/language-spec.md */
bool test_specExamples(void) {
  const char* program =
    "let sq\n"
    "  function [x] [mul x x]\n"
    "\n"
    "let point [struct x y z]\n"
    "\n"
    "let distance\n"
    "  function [a, b]\n"
    "    sqrt\n"
    "      sum [sq [sub a.x b.x]]\n"
    "          [sq [sub a.y b.y]]\n"
    "          [sq [sub a.z b.z]]\n"
    "\n"
    "let origin [point 0 0 0]\n"
    "let magnitude\n"
    "  function [a]\n"
    "    distance a origin\n"
    "    \n"
    "let some-point\n"
    "  point x = 3.14\n"
    "        y = 22/7\n"
    "        z = 157/50\n"
    "\n"
    "print-ln \"magnitude: \"\n"
    "         [magnitude some-point]\n";
  const char* optionals =
    "let ball-volume\n"
    "  function [radius, dimension = 2]\n"
    "    case\n"
    "      [equal? dimension 2]\n"
    "        mul pi [sq radius]\n"
    "      [equal? dimension 3]\n"
    "        mul 3/4 pi [cube radius]\n"
    "\n"
    "print \"unit-ball in 2D\"\n"
    "  ball-volume 1;\n"
    "print \"unit-ball in 3D\"\n"
    "  ball-volume 1, dimension = 3;\n";
  const char* calls[] = {
    "[point [sq 2] 3 4]\n",
    "point [sq 2] 3 4\n",
    "point [sq 2] 3\n  4\n",
    "point [sq 2]\n  3\n  4\n",
    "point\n  [sq 2]\n  3\n  4\n",
    "point\n  sq 2\n  3\n  4\n",
    "point\n  sq\n    2\n  3\n  4\n",
    "point [sq 2] \\\n  3 4\n",
  };
  int i = 0;
  if (lexAll(program) <= 0 || lexAll(optionals) <= 0) {
    return false;
  }
  while (i < LEN(calls)) {
    if (lexAll(calls[i]) <= 0) {
      return false;
    }
    i++;
  }

  Exp continued[] = {
    tok_ok(tokenKind_ID, 0, 5, 0, 0),
    tok_ok(tokenKind_LBRACKET, 6, 7, 0, 6),
    tok_ok(tokenKind_ID, 7, 9, 0, 7),
    tok_ok(tokenKind_NUM, 10, 11, 0, 10),
    tok_ok(tokenKind_RBRACKET, 11, 12, 0, 11),
    tok_ok(tokenKind_BACKSLASH, 13, 14, 0, 13),
    tok_ok(tokenKind_NL, 14, 15, 0, 14),
    tok_ok(tokenKind_NUM, 17, 18, 1, 2),
    tok_ok(tokenKind_NUM, 19, 20, 1, 4),
    tok_ok(tokenKind_EOF, 20, 20, 1, 5),
  };
  return checkStr("point [sq 2] \\\n  3 4", continued, LEN(continued));
}

/* BEGIN: DRIVER CODE */
Tester tests[] = {
  {"test_new", test_new},
  {"test_kindNames", test_kindNames},
  {"test_empty", test_empty},
  {"test_punctuation", test_punctuation},
  {"test_delimiters", test_delimiters},
  {"test_identifiers", test_identifiers},
  {"test_badIdentifiers", test_badIdentifiers},
  {"test_numbers", test_numbers},
  {"test_badNumbers", test_badNumbers},
  {"test_minus", test_minus},
  {"test_strings", test_strings},
  {"test_badStrings", test_badStrings},
  {"test_comments", test_comments},
  {"test_lines", test_lines},
  {"test_badChars", test_badChars},
  {"test_badUTF8", test_badUTF8},
  {"test_idempotent", test_idempotent},
  {"test_peek", test_peek},
  {"test_peekInvalid", test_peekInvalid},
  {"test_specExamples", test_specExamples},
};

int main(void) {
  run_tests("lexer", tests, LEN(tests));
}
/* END: DRIVER CODE */
