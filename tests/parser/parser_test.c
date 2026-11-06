/*
MIT License
Copyright 2026 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#include <stdlib.h>
#include <string.h>
#include "../common.h"
#include "../../lib/alloc/stdAlloc.h"
#include "../../lib/parser.h"

#define LEN(array) ((int)(sizeof(array) / sizeof((array)[0])))

static u8 g_pao_heap[common_HEAPSIZE];

/* BEGIN: helpers */

/* a compact text of a tree, to compare many cases without their ranges:
 *   PROGRAM `P[a b]`; symbols by name; strings between quotes; numbers as
 *   in the dump; a CALL by its form: `(..)` sexpr, `{..}` iexpr, `<..>`
 *   suffix, `?(..)` none; a TUPLE `[..]`. A `,` right after the opening
 *   means `hasCommas`.
 * In the `plain` mode the forms and the commas are not shown, and every
 * CALL is `(..)` and every TUPLE is `T[..]`, so trees that are the same but
 * for the way they were written look the same. */
typedef struct {
  char* buf;
  usize cap;
  usize len;
} Out;

static void put(Out* o, const char* s) {
  usize n = strlen(s);
  if (o->len + n + 1 <= o->cap) {
    memcpy(o->buf + o->len, s, n);
    o->len += n;
    o->buf[o->len] = '\0';
  }
}

static void putChar(Out* o, char c) {
  char s[2];
  s[0] = c;
  s[1] = '\0';
  put(o, s);
}

static void shape(Out* o, const AST* n, const SymbolSpace* sp, bool plain);

static void shapeKids(Out* o, const AST* n, const SymbolSpace* sp, bool plain) {
  usize i = 0;
  while (i < n->leafsLength) {
    if (i > 0) {
      putChar(o, ' ');
    }
    shape(o, n->leafs[i], sp, plain);
    i++;
  }
}

static void shape(Out* o, const AST* n, const SymbolSpace* sp, bool plain) {
  char tmp[256];
  char open = '[';
  char close = ']';
  switch (n->kind) {
    case astKind_PROGRAM:
      put(o, "P[");
      shapeKids(o, n, sp, plain);
      put(o, "]");
      return;
    case astKind_CALL:
    case astKind_TUPLE:
      if (n->kind == astKind_CALL) {
        open = '('; close = ')';
      }
      if (plain) {
        if (n->kind == astKind_TUPLE) {
          putChar(o, 'T');
        }
      } else if (n->form == astForm_IEXPR) {
        open = '{'; close = '}';
      } else if (n->form == astForm_SUFFIX) {
        open = '<'; close = '>';
      } else if (n->kind == astKind_CALL && n->form == astForm_NONE) {
        put(o, "?");
      }
      putChar(o, open);
      if (!plain && n->hasCommas) {
        putChar(o, ',');
      }
      shapeKids(o, n, sp, plain);
      putChar(o, close);
      return;
    case astKind_SYMBOL: {
      Str name = symbolSpace_findSymbol(sp, n->value.value.Symbol);
      usize i = 0;
      while (i < name.length) {
        putChar(o, name.data[i]);
        i++;
      }
      return;
    }
    case astKind_STRING: {
      usize i = 0;
      putChar(o, '"');
      while (i < n->value.value.Str.length) {
        putChar(o, n->value.value.Str.data[i]);
        i++;
      }
      putChar(o, '"');
      return;
    }
    case astKind_NUMBER: {
      /* the value is what follows the range in the dump of the node */
      const char* p;
      usize len = ast_snprint(n, sp, tmp, sizeof(tmp));
      usize i = 0;
      int spaces = 0;
      while (i < len && spaces < 2) {
        if (tmp[i] == ' ') {
          spaces++;
        }
        i++;
      }
      p = tmp + i;
      while (i < len && tmp[i] != '\n') {
        i++;
      }
      tmp[i] = '\0';
      put(o, p);
      return;
    }
    case astKind_NIL:
      put(o, "NIL");
      return;
    case astKind_BOOL:
      put(o, "BOOL");
      return;
  }
}

static usize usedBy(const IAllocator* mem) {
  return mem->info(mem->heap).used;
}

/* a symbol space that allocates from `mem` (and does not fail) */
static void spNew(const IAllocator* mem, SymbolSpace* sp) {
  checkStatus(symbolSpace_new(mem, 8, sp));
}

static Status run(const IAllocator* mem, SymbolSpace* sp, const char* src,
                  usize maxDepth, AST** out, ParseError* err) {
  return parser_parse(mem, sp, (const u8*)src, strlen(src), maxDepth, out, err);
}

/* parses `src` and compares the text of its tree with `want`; nothing may
 * be left allocated afterwards. */
static bool shapeIs(const char* src, const char* want, bool plain) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  SymbolSpace sp;
  AST* tree = NULL;
  ParseError err;
  char buf[2048];
  Out o;
  bool ok;
  spNew(&mem, &sp);
  buf[0] = '\0';
  o.buf = buf;
  o.cap = sizeof(buf);
  o.len = 0;
  if (run(&mem, &sp, src, 64, &tree, &err) != status_OK) {
    printf("  parse of \"%s\" failed with status %d at %u:%u\n", src,
           (int)err.code, err.range.begin.line + 1, err.range.begin.col + 1);
    symbolSpace_free(&sp);
    return false;
  }
  shape(&o, tree, &sp, plain);
  ok = strcmp(buf, want) == 0;
  if (!ok) {
    printf("  \"%s\"\n    got  %s\n    want %s\n", src, buf, want);
  }
  ast_free(&mem, tree);
  symbolSpace_free(&sp);
  return ok && usedBy(&mem) == 0;
}

typedef struct {
  const char* src;
  const char* want;
} ShapeCase;

static bool shapeCases(const ShapeCase* cases, int n, bool plain) {
  bool ok = true;
  int i = 0;
  while (i < n) {
    ok = shapeIs(cases[i].src, cases[i].want, plain) && ok;
    i++;
  }
  return ok;
}

/* the dump of the tree of `src` is exactly `want` */
static bool dumpIs(const char* src, const char* want) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  SymbolSpace sp;
  AST* tree = NULL;
  ParseError err;
  char buf[4096];
  usize n;
  bool ok;
  spNew(&mem, &sp);
  if (run(&mem, &sp, src, 64, &tree, &err) != status_OK) {
    symbolSpace_free(&sp);
    return false;
  }
  n = ast_snprint(tree, &sp, buf, sizeof(buf));
  ok = n == strlen(want) && memcmp(buf, want, n) == 0;
  if (!ok) {
    printf("  \"%s\"\n    got\n%.*s    want\n%s", src, (int)n, buf, want);
  }
  ast_free(&mem, tree);
  symbolSpace_free(&sp);
  return ok && usedBy(&mem) == 0;
}

typedef struct {
  const char* src;
  Status code;
  u32 line;          /* of the error, from 1 */
  u32 col;           /* of the error, from 1 */
  const char* expected;
} ErrorCase;

static bool sameText(const char* a, const char* b) {
  if (a == NULL || b == NULL) {
    return a == b;
  }
  return strcmp(a, b) == 0;
}

/* the parse fails as the case says, `out` is not touched, and nothing is left
 * allocated (but the symbols, which are freed with the space). */
static bool errorCase(const ErrorCase* c, usize maxDepth) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  SymbolSpace sp;
  AST* tree = NULL;
  ParseError err;
  Status st;
  bool ok;
  err.code = status_OK;
  err.range = range_none();
  err.expected = NULL;
  spNew(&mem, &sp);
  st = run(&mem, &sp, c->src, maxDepth, &tree, &err);
  ok = st == c->code && err.code == c->code && tree == NULL &&
       err.range.begin.line + 1 == c->line &&
       err.range.begin.col + 1 == c->col &&
       sameText(err.expected, c->expected);
  if (!ok) {
    printf("  \"%s\": status %d (want %d) at %u:%u (want %u:%u), expected "
           "\"%s\" (want \"%s\")\n", c->src, (int)st, (int)c->code,
           err.range.begin.line + 1, err.range.begin.col + 1, c->line, c->col,
           err.expected == NULL ? "(null)" : err.expected,
           c->expected == NULL ? "(null)" : c->expected);
  }
  if (tree != NULL) {
    ast_free(&mem, tree);
  }
  symbolSpace_free(&sp);
  return ok && usedBy(&mem) == 0;
}

static bool errorCases(const ErrorCase* cases, int n, usize maxDepth) {
  bool ok = true;
  int i = 0;
  while (i < n) {
    ok = errorCase(&cases[i], maxDepth) && ok;
    i++;
  }
  return ok;
}

/* END: helpers */

/* BEGIN: tests */

bool test_atoms(void) {
  ShapeCase cases[] = {
    { "x", "P[x]" },
    { "42", "P[42]" },
    { "\"hi\"", "P[\"hi\"]" },
    { "a\nb\nc", "P[a b c]" },
    { "-3\n0x10\n0b101\n1_000\n22/7\n-1/2\n3f", "P[-3 16 5 1000 22/7 -1/2 3]" },
    { "123456789012345678901234567890", "P[123456789012345678901234567890]" },
    { "9223372036854775808", "P[9223372036854775808]" },
    { "\"a\\nb\\t\\\"\\\\\"", "P[\"a\nb\t\"\\\"]" },
    { "\"\"", "P[\"\"]" },
    { "to-string equal? set!", "P[{to-string equal? set!}]" },
  };
  return shapeCases(cases, LEN(cases), false);
}

/* `nil`, `true` and `false` are symbols, which the evaluator knows */
bool test_builtinNamesAreSymbols(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  SymbolSpace sp;
  AST* tree = NULL;
  ParseError err;
  bool ok;
  spNew(&mem, &sp);
  checkStatus(run(&mem, &sp, "nil\ntrue\nfalse", 64, &tree, &err));
  ok = tree->leafsLength == 3 && tree->leafs[0]->kind == astKind_SYMBOL &&
       tree->leafs[1]->kind == astKind_SYMBOL &&
       tree->leafs[2]->kind == astKind_SYMBOL;
  ast_free(&mem, tree);
  symbolSpace_free(&sp);
  return ok && usedBy(&mem) == 0;
}

bool test_calls(void) {
  ShapeCase cases[] = {
    { "(f x y)", "P[(f x y)]" },
    { "(f)", "P[(f)]" },
    { "(f (g x) y)", "P[(f (g x) y)]" },
    { "((f) x)", "P[((f) x)]" },
    { "f x y", "P[{f x y}]" },
    { "f", "P[f]" },
    { "f x\n  y\n  z", "P[{f x y z}]" },
    { "f\n  g\n  h", "P[{f g h}]" },
    { "f (g x) y", "P[{f (g x) y}]" },
    { "(f x) y", "P[{(f x) y}]" },
    { "f\n  g x\n  h y", "P[{f {g x} {h y}}]" },
    { "f\n  (g x)\n  (h y)", "P[{f (g x) (h y)}]" },
  };
  return shapeCases(cases, LEN(cases), false);
}

/* the equivalent ways to write a call of the spec give the same tree, but
 * for how they were written */
bool test_equivalentCalls(void) {
  ShapeCase cases[] = {
    { "(point (sq 2) 3 4)", "P[(point (sq 2) 3 4)]" },
    { "point (sq 2) 3 4", "P[(point (sq 2) 3 4)]" },
    { "point (sq 2) 3\n  4", "P[(point (sq 2) 3 4)]" },
    { "point (sq 2)\n  3\n  4", "P[(point (sq 2) 3 4)]" },
    { "point\n  (sq 2)\n  3\n  4", "P[(point (sq 2) 3 4)]" },
    { "point\n  sq 2\n  3\n  4", "P[(point (sq 2) 3 4)]" },
    { "point\n  sq\n    2\n  3\n  4", "P[(point (sq 2) 3 4)]" },
    { "point (sq 2) \\\n  3 4", "P[(point (sq 2) 3 4)]" },
  };
  return shapeCases(cases, LEN(cases), true);
}

bool test_suffixes(void) {
  ShapeCase cases[] = {
    { "a.b", "P[<. a b>]" },
    { "a.b.c", "P[<. a b c>]" },
    { "a.b.c.d", "P[<. a b c d>]" },
    { "a : T", "P[<: a T>]" },
    { "a:T", "P[<: a T>]" },
    { "x = 1", "P[<= x 1>]" },
    { "x=1", "P[<= x 1>]" },
    { "a.0", "P[<. a 0>]" },
    { "a.(f x).b", "P[<. a (f x) b>]" },
    { "a.b = c.d", "P[<= <. a b> <. c d>>]" },
    { "a : T = v", "P[<= <: a T> v>]" },
    { "a.b : T", "P[<: <. a b> T>]" },
    { "x = a : T", "P[<= x <: a T>>]" },
    { "x = a.b : T", "P[<= x <: <. a b> T>>]" },
    { "a.b : T.U = c.d", "P[<= <: <. a b> <. T U>> <. c d>>]" },
    { "let a.b = c.d", "P[{let <= <. a b> <. c d>>}]" },
    { "let a = 1", "P[{let <= a 1>}]" },
    { "f x = 1 y = 2", "P[{f <= x 1> <= y 2>}]" },
    { "f a.b c.d", "P[{f <. a b> <. c d>}]" },
    { "(f a.b)", "P[(f <. a b>)]" },
    { "let x : num = 3", "P[{let <= <: x num> 3>}]" },
  };
  return shapeCases(cases, LEN(cases), false);
}

/* only the dot chains: the other suffixes are at most one for each term */
bool test_suffixesDoNotChain(void) {
  ErrorCase cases[] = {
    { "a = b = c", status_UNEXPECTEDTOKEN, 1, 7, "end of line" },
    { "a : b : c", status_UNEXPECTEDTOKEN, 1, 7, "end of line" },
    { "a = b : c = d", status_UNEXPECTEDTOKEN, 1, 11, "end of line" },
    { "a : b.c : d", status_UNEXPECTEDTOKEN, 1, 9, "end of line" },
    { "(a = b = c)", status_UNEXPECTEDTOKEN, 1, 8, "')'" },
  };
  return errorCases(cases, LEN(cases), 64);
}

/* a suffix is a call of the symbol of its operator, and the symbols of the
 * same operator are the same */
bool test_suffixSymbols(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  SymbolSpace sp;
  AST* tree = NULL;
  ParseError err;
  AST* call;
  AST* first;
  AST* second;
  bool ok;
  spNew(&mem, &sp);
  checkStatus(run(&mem, &sp, "a.b x = 1 c : d", 64, &tree, &err));
  call = tree->leafs[0];
  ok = call->kind == astKind_CALL && call->form == astForm_IEXPR &&
       call->leafsLength == 3;
  first = call->leafs[0];    /* a.b */
  ok = ok && first->kind == astKind_CALL && first->form == astForm_SUFFIX &&
       first->leafs[0]->kind == astKind_SYMBOL &&
       first->leafs[0]->tk.kind == tokenKind_DOT &&
       first->leafs[0]->tk.text.length == 1 &&
       first->leafs[0]->tk.text.data[0] == '.';
  second = call->leafs[1];   /* x = 1 */
  ok = ok && second->kind == astKind_CALL && second->form == astForm_SUFFIX &&
       second->leafs[0]->tk.kind == tokenKind_EQUALS &&
       second->leafs[0]->range.begin.col == 6;
  first = call->leafs[2];    /* c : d */
  ok = ok && first->form == astForm_SUFFIX &&
       first->leafs[0]->tk.kind == tokenKind_COLON &&
       first->leafs[0]->value.value.Symbol !=
         second->leafs[0]->value.value.Symbol;
  ast_free(&mem, tree);
  symbolSpace_free(&sp);
  return ok && usedBy(&mem) == 0;
}

bool test_sameSymbols(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  SymbolSpace sp;
  AST* tree = NULL;
  ParseError err;
  AST* call;
  bool ok;
  spNew(&mem, &sp);
  checkStatus(run(&mem, &sp, "a a.b c.d", 64, &tree, &err));
  call = tree->leafs[0];
  ok = call->leafs[0]->value.value.Symbol ==
         call->leafs[1]->leafs[1]->value.value.Symbol &&
       call->leafs[1]->leafs[0]->value.value.Symbol ==
         call->leafs[2]->leafs[0]->value.value.Symbol &&
       call->leafs[1]->leafs[2]->value.value.Symbol !=
         call->leafs[2]->leafs[2]->value.value.Symbol;
  ast_free(&mem, tree);
  symbolSpace_free(&sp);
  return ok && usedBy(&mem) == 0;
}

bool test_tuples(void) {
  ShapeCase cases[] = {
    { "[a]", "P[[a]]" },
    { "[a,]", "P[[,a]]" },
    { "[a, b]", "P[[,a b]]" },
    { "[a b]", "P[[a b]]" },
    { "[a b c,]", "P[[,a b c]]" },
    { "[(f x) [y]]", "P[[(f x) [y]]]" },
    { "f [a b] c", "P[{f [a b] c}]" },
    { "let a = [1, 2, 3]", "P[{let <= a [,1 2 3]>}]" },
    { "let [a, b] = p", "P[{let <= [,a b] p>}]" },
    { "let [p.a] = [p.b]", "P[{let <= [<. p a>] [<. p b>]>}]" },
    { "x = [a.y]", "P[<= x [<. a y>]>]" },
    { "[a.b].c", "P[<. [<. a b>] c>]" },
    { "(f [a, b])", "P[(f [,a b])]" },
    { "[a : T]", "P[[<: a T>]]" },
    { "[[a]]", "P[[[a]]]" },
  };
  return shapeCases(cases, LEN(cases), false);
}

/* the commas are not nodes, but a list that had one says so */
bool test_commas(void) {
  ShapeCase cases[] = {
    { "(a, b)", "P[(,a b)]" },
    { "(a b,)", "P[(,a b)]" },
    { "(a b)", "P[(a b)]" },
    { "(a, b, c)", "P[(,a b c)]" },
    { "(a,b)", "P[(,a b)]" },
    { "f a, b", "P[{,f a b}]" },
    { "f a b", "P[{f a b}]" },
    { "f a,\n  b", "P[{,f a b}]" },
    { "f a, \\\n  b", "P[{,f a b}]" },
    { "f a \\\n  b, c", "P[{,f a b c}]" },
    { "f 1, dimension = 3", "P[{,f 1 <= dimension 3>}]" },
    { "(f x, y)\n(f x y)", "P[(,f x y) (f x y)]" },
    { "f (a, b) c", "P[{f (,a b) c}]" },
    /* a line of one term is that term, and its comma is lost */
    { "x,", "P[x]" },
  };
  return shapeCases(cases, LEN(cases), false);
}

bool test_blocks(void) {
  ShapeCase cases[] = {
    { "a\nb\nc", "P[a b c]" },
    { "a\n  b\n    c\n  d\ne", "P[{a {b c} d} e]" },
    { "a\n  b\n    c\nd", "P[{a {b c}} d]" },
    { "a\n  b\n    c\n    d\n  e", "P[{a {b c d} e}]" },
    { "a\n\n# comment\n  b  # tail\n\nc", "P[{a b} c]" },
    { "  a\n  b", "P[a b]" },
    { "  a\n    b", "P[{a b}]" },
    { "a\n  b\n\n\n  c", "P[{a b c}]" },
    { "a\r\n  b\r\n", "P[{a b}]" },
    { "a b\n  c d\n  e", "P[{a b {c d} e}]" },
    { "case\n  (equal? d 2)\n    mul pi 1\n  (equal? d 3)\n    mul pi 2",
      "P[{case {(equal? d 2) {mul pi 1}} {(equal? d 3) {mul pi 2}}}]" },
    { "a\n   b\n   c", "P[{a b c}]" },
    { "a\n\tb", "" },
  };
  /* the last one is a lexical error: it is in `test_errors` */
  return shapeCases(cases, LEN(cases) - 1, false);
}

/* `;` ends the block of its line, and is not in the tree */
bool test_semicolon(void) {
  ShapeCase cases[] = {
    { "a b;", "P[{a b}]" },
    { "a;", "P[a]" },
    { "a ;  # comment", "P[a]" },
    { "a;\n", "P[a]" },
    { "a\n  b;", "P[{a b}]" },
    { "a\n  b\n    c;\n  d", "P[{a {b c} d}]" },
    { "print \"a\"\n  ball-volume 1;\nprint \"b\"\n  ball-volume 1, dimension = 3;",
      "P[{print \"a\" {ball-volume 1}} "
      "{print \"b\" {,ball-volume 1 <= dimension 3>}}]" },
    { "a\n  b\n    c;\nd", "P[{a {b c}} d]" },
    { "a\n  b;\nc;", "P[{a b} c]" },
    { "f x \\\n  y;", "P[{f x y}]" },
  };
  return shapeCases(cases, LEN(cases), false);
}

bool test_continuation(void) {
  ShapeCase cases[] = {
    { "a \\\n  b", "P[{a b}]" },
    { "a \\\n  b \\\n  c", "P[{a b c}]" },
    { "a \\\n\n  b", "P[{a b}]" },
    { "a \\ # comment\n  b", "P[{a b}]" },
    { "a \\\n  b\n  c", "P[{a b c}]" },
    { "f x\n  g \\\n    y\n  h", "P[{f x {g y} h}]" },
    { "a \\\n  (b\n  )", "" },
  };
  /* the last one is an error: a newline inside brackets */
  return shapeCases(cases, LEN(cases) - 1, false);
}

bool test_empty(void) {
  ShapeCase cases[] = {
    { "", "P[]" },
    { "   ", "P[]" },
    { "\n\n\n", "P[]" },
    { "# only a comment", "P[]" },
    { "# comment\n\n  # another\n", "P[]" },
  };
  return shapeCases(cases, LEN(cases), false) &&
         dumpIs("", "PROGRAM 1:1\n") &&
         dumpIs("  # c\n\n", "PROGRAM 3:1\n") &&
         dumpIs("   ", "PROGRAM 1:4\n");
}

/* the example of the spec, with `3f`, since `3.14` has no `Decimal` yet */
bool test_specExample(void) {
  const char* src =
    "let sq\n"
    "  function (x) (mul x x)\n"
    "\n"
    "let point (struct x y z)\n"
    "\n"
    "let distance\n"
    "  function (a, b)\n"
    "    sqrt\n"
    "      sum (sq (sub a.x b.x))\n"
    "          (sq (sub a.y b.y))\n"
    "          (sq (sub a.z b.z))\n"
    "\n"
    "let origin (point 0 0 0)\n"
    "let magnitude\n"
    "  function (a)\n"
    "    distance a origin\n"
    "    \n"
    "let some-point\n"
    "  point x = 3f\n"
    "        y = 22/7\n"
    "        z = 157/50\n"
    "\n"
    "print-ln \"magnitude: \"\n"
    "         (magnitude some-point)\n";
  const char* want =
    "P["
    "{let sq {function (x) (mul x x)}} "
    "{let point (struct x y z)} "
    "{let distance {function (,a b) {sqrt {sum "
      "(sq (sub <. a x> <. b x>)) "
      "(sq (sub <. a y> <. b y>)) "
      "(sq (sub <. a z> <. b z>))}}}} "
    "{let origin (point 0 0 0)} "
    "{let magnitude {function (a) {distance a origin}}} "
    "{let some-point {point <= x 3> <= y 22/7> <= z 157/50>}} "
    "{print-ln \"magnitude: \" (magnitude some-point)}"
    "]";
  return shapeIs(src, want, false);
}

bool test_specFunctionArguments(void) {
  const char* src =
    "let ball-volume\n"
    "  function (radius, dimension = 2)\n"
    "    case\n"
    "      (equal? dimension 2)\n"
    "        mul pi (sq radius)\n"
    "      (equal? dimension 3)\n"
    "        mul 3/4 pi (cube radius)\n"
    "\n"
    "print \"unit-ball in 2D\"\n"
    "  ball-volume 1;\n"
    "print \"unit-ball in 3D\"\n"
    "  ball-volume 1, dimension = 3;\n";
  const char* want =
    "P["
    "{let ball-volume {function (,radius <= dimension 2>) {case "
      "{(equal? dimension 2) {mul pi (sq radius)}} "
      "{(equal? dimension 3) {mul 3/4 pi (cube radius)}}}}} "
    "{print \"unit-ball in 2D\" {ball-volume 1}} "
    "{print \"unit-ball in 3D\" {,ball-volume 1 <= dimension 3>}}"
    "]";
  return shapeIs(src, want, false);
}

/* the ranges: from the first to the last character of what a node has, with
 * the end excluded, and the lines and columns from 1 */
bool test_ranges(void) {
  return
    dumpIs("(a b)",
           "PROGRAM 1:1-1:6\n"
           "  CALL 1:1-1:6 sexpr\n"
           "    SYMBOL 1:2-1:3 a\n"
           "    SYMBOL 1:4-1:5 b\n") &&
    dumpIs("f x\n  y\n",
           "PROGRAM 1:1-2:4\n"
           "  CALL 1:1-2:4 iexpr\n"
           "    SYMBOL 1:1-1:2 f\n"
           "    SYMBOL 1:3-1:4 x\n"
           "    SYMBOL 2:3-2:4 y\n") &&
    dumpIs("[a, b]",
           "PROGRAM 1:1-1:7\n"
           "  TUPLE 1:1-1:7 commas\n"
           "    SYMBOL 1:2-1:3 a\n"
           "    SYMBOL 1:5-1:6 b\n") &&
    dumpIs("x = a.b",
           "PROGRAM 1:1-1:8\n"
           "  CALL 1:1-1:8 suffix\n"
           "    SYMBOL 1:3-1:4 =\n"
           "    SYMBOL 1:1-1:2 x\n"
           "    CALL 1:5-1:8 suffix\n"
           "      SYMBOL 1:6-1:7 .\n"
           "      SYMBOL 1:5-1:6 a\n"
           "      SYMBOL 1:7-1:8 b\n") &&
    /* the parentheses are part of the range of a call, which has them as
     * its range and not as children */
    dumpIs("( f   x )",
           "PROGRAM 1:1-1:10\n"
           "  CALL 1:1-1:10 sexpr\n"
           "    SYMBOL 1:3-1:4 f\n"
           "    SYMBOL 1:7-1:8 x\n") &&
    /* the columns are of runes, not of bytes */
    dumpIs("\"\xc3\xa9\" x",
           "PROGRAM 1:1-1:6\n"
           "  CALL 1:1-1:6 iexpr\n"
           "    STRING 1:1-1:4 \"\xc3\xa9\"\n"
           "    SYMBOL 1:5-1:6 x\n") &&
    dumpIs("a\n  b\n    c\nd",
           "PROGRAM 1:1-4:2\n"
           "  CALL 1:1-3:6 iexpr\n"
           "    SYMBOL 1:1-1:2 a\n"
           "    CALL 2:3-3:6 iexpr\n"
           "      SYMBOL 2:3-2:4 b\n"
           "      SYMBOL 3:5-3:6 c\n"
           "  SYMBOL 4:1-4:2 d\n") &&
    /* the `;` is not in the range */
    dumpIs("a b;",
           "PROGRAM 1:1-1:4\n"
           "  CALL 1:1-1:4 iexpr\n"
           "    SYMBOL 1:1-1:2 a\n"
           "    SYMBOL 1:3-1:4 b\n");
}

bool test_numbersAndStrings(void) {
  return
    dumpIs("-3 \"x\\ty\" 22/7",
           "PROGRAM 1:1-1:15\n"
           "  CALL 1:1-1:15 iexpr\n"
           "    NUMBER 1:1-1:3 -3\n"
           "    STRING 1:4-1:10 \"x\\ty\"\n"
           "    NUMBER 1:11-1:15 22/7\n");
}

bool test_errors(void) {
  ErrorCase cases[] = {
    /* an opening bracket or parenthesis without the closing one, and the
     * closing one without the opening */
    { "(a", status_UNEXPECTEDTOKEN, 1, 3, "')'" },
    { "[a", status_UNEXPECTEDTOKEN, 1, 3, "']'" },
    { "(a\n)", status_UNEXPECTEDTOKEN, 1, 3, "')'" },
    { "[a\n]", status_UNEXPECTEDTOKEN, 1, 3, "']'" },
    { "(a ]", status_UNEXPECTEDTOKEN, 1, 4, "')'" },
    { "[a )", status_UNEXPECTEDTOKEN, 1, 4, "']'" },
    { "a)", status_UNEXPECTEDTOKEN, 1, 2, "end of line" },
    { "a]", status_UNEXPECTEDTOKEN, 1, 2, "end of line" },
    { ")", status_UNEXPECTEDTOKEN, 1, 1, "term" },
    { "]", status_UNEXPECTEDTOKEN, 1, 1, "term" },
    /* nothing inside, and a newline inside */
    { "()", status_UNEXPECTEDTOKEN, 1, 2, "term" },
    { "[]", status_UNEXPECTEDTOKEN, 1, 2, "term" },
    { "(,)", status_UNEXPECTEDTOKEN, 1, 2, "term" },
    { "(\na)", status_UNEXPECTEDTOKEN, 1, 2, "term" },
    { "f (a\n  b)", status_UNEXPECTEDTOKEN, 1, 5, "')'" },
    { "(f (a)\n)", status_UNEXPECTEDTOKEN, 1, 7, "')'" },
    /* commas */
    { "a ,, b", status_UNEXPECTEDTOKEN, 1, 4, "end of line" },
    { "(a,, b)", status_UNEXPECTEDTOKEN, 1, 4, "')'" },
    { ", a", status_UNEXPECTEDTOKEN, 1, 1, "term" },
    /* a suffix needs both sides */
    { "x =", status_UNEXPECTEDTOKEN, 1, 4, "term" },
    { "x = \n", status_UNEXPECTEDTOKEN, 1, 5, "term" },
    { "a.", status_UNEXPECTEDTOKEN, 1, 3, "term" },
    { "a :", status_UNEXPECTEDTOKEN, 1, 4, "term" },
    { "a.b.", status_UNEXPECTEDTOKEN, 1, 5, "term" },
    { "= a", status_UNEXPECTEDTOKEN, 1, 1, "term" },
    { ". a", status_UNEXPECTEDTOKEN, 1, 1, "term" },
    { ": a", status_UNEXPECTEDTOKEN, 1, 1, "term" },
    { "a = )", status_UNEXPECTEDTOKEN, 1, 5, "term" },
    /* the `;`: only at the end of the line, and it ends the block */
    { ";", status_UNEXPECTEDTOKEN, 1, 1, "term" },
    { "a ; b", status_UNEXPECTEDTOKEN, 1, 5, "end of line" },
    { "a\n;\nb", status_UNEXPECTEDTOKEN, 2, 1, "term" },
    { "a;\nb", status_UNEXPECTEDTOKEN, 2, 1, "end of the block" },
    { "a\n  b;\n  c", status_UNEXPECTEDTOKEN, 3, 3, "end of the block" },
    { "a;\n  b", status_BADINDENT, 2, 3, NULL },
    { "(a;)", status_UNEXPECTEDTOKEN, 1, 3, "')'" },
    /* indentation */
    { "a\n    b\n  c", status_BADDEDENT, 3, 3, NULL },
    { "  a\nb", status_BADDEDENT, 2, 1, NULL },
    { "a\n  b\n c", status_BADDEDENT, 3, 2, NULL },
    { "a\n    b\n      c\n   d", status_BADDEDENT, 4, 4, NULL },
    /* the continuation */
    { "a \\\nb", status_BADINDENT, 2, 1,
      "a column deeper than the line it continues" },
    { "a\n  b \\\n  c", status_BADINDENT, 3, 3,
      "a column deeper than the line it continues" },
    { "a \\ b", status_UNEXPECTEDTOKEN, 1, 5, "end of line" },
    { "a \\", status_UNEXPECTEDTOKEN, 1, 4, "end of line" },
    { "a \\\n", status_UNEXPECTEDTOKEN, 2, 1, "term" },
    { "a \\\n  ", status_UNEXPECTEDTOKEN, 2, 3, "term" },
    { "a \\\n  )", status_UNEXPECTEDTOKEN, 2, 3, "term" },
    { "a \\\n  b \\", status_UNEXPECTEDTOKEN, 2, 6, "end of line" },
    { "\\ a", status_UNEXPECTEDTOKEN, 1, 1, "term" },
    /* the end of the input in the middle of something */
    { "f [", status_UNEXPECTEDTOKEN, 1, 4, "term" },
    { "f (a [b", status_UNEXPECTEDTOKEN, 1, 8, "']'" },
  };
  return errorCases(cases, LEN(cases), 64);
}

/* what the lexer does not accept is the status of the lexer, at the token
 * that is invalid */
bool test_lexicalErrors(void) {
  ErrorCase cases[] = {
    { "f[x]", status_BADCHAR, 1, 1, NULL },
    { "f(x)", status_BADCHAR, 1, 1, NULL },
    { "\"a", status_UNTERMINATEDSTRING, 1, 1, NULL },
    { "\"a\nb\"", status_UNTERMINATEDSTRING, 1, 1, NULL },
    { "\"\\q\"", status_BADESCAPE, 1, 1, NULL },
    { "1x", status_BADNUMBER, 1, 1, NULL },
    { "0x", status_BADNUMBER, 1, 1, NULL },
    { "\tx", status_BADCHAR, 1, 1, NULL },
    { "a\n  \tb", status_BADCHAR, 2, 3, NULL },
    { "a @ b", status_BADCHAR, 1, 3, NULL },
    { "\xff", status_INVALIDUTF8, 1, 1, NULL },
    { "(a b\n  c) \xe2\x82", status_UNEXPECTEDTOKEN, 1, 5, "')'" },
    { "a\n\xc3\xa9", status_BADCHAR, 2, 1, NULL },
    { "x = (a ~)", status_BADCHAR, 1, 8, NULL },
  };
  return errorCases(cases, LEN(cases), 64);
}

/* what the lexer accepts but cannot be converted has the status of the
 * conversion, and the range of the token */
bool test_literalErrors(void) {
  ErrorCase cases[] = {
    { "x = 3.14", status_UNSUPPORTEDNUMBER, 1, 5, NULL },
    { "3.", status_UNSUPPORTEDNUMBER, 1, 1, NULL },
    { "1e5", status_UNSUPPORTEDNUMBER, 1, 1, NULL },
    { "(f 1/0)", status_DIVISIONBYZERO, 1, 4, NULL },
    { "a\n  b 7/0", status_DIVISIONBYZERO, 2, 5, NULL },
    { "1e400f", status_UNSUPPORTEDNUMBER, 1, 1, NULL },
  };
  return errorCases(cases, LEN(cases), 64);
}

bool test_depth(void) {
  ErrorCase brackets[] = {
    { "((a))", status_MAXDEPTH, 1, 2, NULL },
    { "(a)", status_MAXDEPTH, 1, 1, NULL },
    { "[(a)]", status_MAXDEPTH, 1, 2, NULL },
  };
  ErrorCase blocks[] = {
    { "a\n  b\n    c", status_MAXDEPTH, 3, 5, NULL },
    { "a\n  b", status_MAXDEPTH, 2, 3, NULL },
    { "a\n  (b [c])", status_MAXDEPTH, 2, 6, NULL },
    { "a\n  b \\\n    c\n    d", status_MAXDEPTH, 4, 5, NULL },
  };
  ShapeCase fits[] = {
    { "a b", "P[{a b}]" },
    { "a \\\n  b", "P[{a b}]" },
    { "(a)", "P[(a)]" },
  };
  bool ok;
  /* the top level does not count: a flat program needs no depth */
  ok = errorCase(&brackets[1], 0) &&
       errorCase(&blocks[1], 0);
  ok = ok && errorCase(&brackets[0], 1) && errorCase(&brackets[2], 1);
  ok = ok && errorCase(&blocks[0], 1) && errorCase(&blocks[2], 2);
  ok = ok && errorCase(&blocks[3], 1);
  /* the same programs are fine with a bit more */
  ok = ok && shapeIs("((a))", "P[((a))]", false) &&
       shapeIs("a\n  b\n    c", "P[{a {b c}}]", false) &&
       shapeIs("[(a)]", "P[[(a)]]", false);
  return ok && shapeCases(fits, LEN(fits), false);
}

/* a depth that is exactly the limit is accepted, and one more is not */
bool test_depthLimit(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  SymbolSpace sp;
  AST* tree = NULL;
  ParseError err;
  bool ok;
  spNew(&mem, &sp);
  ok = run(&mem, &sp, "(((a)))", 3, &tree, &err) == status_OK;
  if (ok) {
    ast_free(&mem, tree);
  }
  tree = NULL;
  ok = ok && run(&mem, &sp, "(((a)))", 2, &tree, &err) == status_MAXDEPTH &&
       tree == NULL && err.range.begin.col == 2;
  symbolSpace_free(&sp);
  return ok && usedBy(&mem) == 0;
}

/* a lot of nesting is an error and not a stack overflow, with the default
 * depth, brackets, parentheses and blocks */
bool test_deepNesting(void) {
  usize n = 20000;
  usize i;
  char* src = (char*)malloc(2 * n + 2);
  char* blocks = (char*)malloc(1000 * 1001 / 2 + 3 * 1000);
  IAllocator mem = stdAlloc_new();
  SymbolSpace sp;
  AST* tree = NULL;
  ParseError err;
  bool ok;
  if (src == NULL || blocks == NULL) {
    free(src);
    free(blocks);
    return false;
  }
  i = 0;
  while (i < n) {
    src[i] = i % 2 == 0 ? '(' : '[';
    i++;
  }
  src[n] = 'a';
  i = 0;
  while (i < n) {
    src[n + 1 + i] = (n - 1 - i) % 2 == 0 ? ')' : ']';
    i++;
  }
  src[2 * n + 1] = '\0';
  checkStatus(symbolSpace_new(&mem, 8, &sp));
  ok = parser_parse(&mem, &sp, (const u8*)src, 2 * n + 1,
                    parser_DEFAULTMAXDEPTH, &tree, &err) == status_MAXDEPTH &&
       tree == NULL && err.range.begin.col == parser_DEFAULTMAXDEPTH;

  /* the same with blocks, each one more indented than the one before */
  {
    usize lines = 1000;
    usize len = 0;
    usize l = 0;
    while (l < lines) {
      usize k = 0;
      while (k < l) {
        blocks[len] = ' ';
        len++;
        k++;
      }
      blocks[len] = 'a';
      blocks[len + 1] = '\n';
      len += 2;
      l++;
    }
    ok = ok && parser_parse(&mem, &sp, (const u8*)blocks, len,
                            parser_DEFAULTMAXDEPTH, &tree, &err) ==
                 status_MAXDEPTH && tree == NULL &&
         err.range.begin.line == parser_DEFAULTMAXDEPTH + 1;
  }
  symbolSpace_free(&sp);
  free(src);
  free(blocks);
  return ok;
}

/* the parse does not depend on the source after it ends: every lexeme has
 * its own copy of the text */
bool test_sourceIsCopied(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  SymbolSpace sp;
  AST* tree = NULL;
  ParseError err;
  const char* text = "let x = \"a b\"\n  f 22/7, y.z;\n";
  usize len = strlen(text);
  char* copy = (char*)malloc(len);
  char first[512];
  char second[512];
  usize n1;
  usize n2;
  bool ok;
  memcpy(copy, text, len);
  spNew(&mem, &sp);
  checkStatus(parser_parse(&mem, &sp, (const u8*)copy, len, 64, &tree, &err));
  n1 = ast_snprint(tree, &sp, first, sizeof(first));
  memset(copy, 'X', len);
  free(copy);
  n2 = ast_snprint(tree, &sp, second, sizeof(second));
  ok = n1 > 0 && n1 == n2 && memcmp(first, second, n1) == 0 &&
       tree->leafs[0]->leafs[1]->leafs[2]->tk.text.length == 5;
  ast_free(&mem, tree);
  symbolSpace_free(&sp);
  return ok && usedBy(&mem) == 0;
}

/* the same source always gives the same tree */
bool test_deterministic(void) {
  const char* src = "let f\n  function (a, b = 1)\n    add a.x b : T\n";
  char first[1024];
  char second[1024];
  usize n1 = 0;
  usize n2 = 0;
  int round = 0;
  while (round < 2) {
    IAllocator mem = stdAlloc_new();
    SymbolSpace sp;
    AST* tree = NULL;
    ParseError err;
    checkStatus(symbolSpace_new(&mem, 8, &sp));
    checkStatus(run(&mem, &sp, src, 64, &tree, &err));
    if (round == 0) {
      n1 = ast_snprint(tree, &sp, first, sizeof(first));
    } else {
      n2 = ast_snprint(tree, &sp, second, sizeof(second));
    }
    ast_free(&mem, tree);
    symbolSpace_free(&sp);
    round++;
  }
  return n1 > 0 && n1 == n2 && memcmp(first, second, n1) == 0;
}

/* `err` is left alone when it works, and `out` when it does not */
bool test_outputsOnlyOnTheirCase(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  SymbolSpace sp;
  AST* tree = NULL;
  AST* marker = (AST*)&sp;
  ParseError err;
  bool ok;
  spNew(&mem, &sp);
  err.code = status_BADCHAR;
  err.range = range_none();
  err.expected = "untouched";
  ok = run(&mem, &sp, "a b", 64, &tree, &err) == status_OK && tree != NULL &&
       err.code == status_BADCHAR && strcmp(err.expected, "untouched") == 0;
  ast_free(&mem, tree);
  tree = marker;
  ok = ok && run(&mem, &sp, "a ]", 64, &tree, &err) ==
             status_UNEXPECTEDTOKEN && tree == marker &&
       err.code == status_UNEXPECTEDTOKEN;
  symbolSpace_free(&sp);
  return ok && usedBy(&mem) == 0;
}

bool test_badSize(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  SymbolSpace sp;
  AST* tree = NULL;
  ParseError err;
  u8 byte = 0;
  bool ok = true;
  spNew(&mem, &sp);
  if (sizeof(usize) > 4) {
    /* it is not even read */
    ok = parser_parse(&mem, &sp, &byte, (usize)UINT32_MAX + 1, 64, &tree,
                      &err) == status_BADSIZE &&
         err.code == status_BADSIZE && tree == NULL &&
         range_isNone(err.range);
  }
  symbolSpace_free(&sp);
  return ok && usedBy(&mem) == 0;
}

/* a failing allocator: at each point of the parse it is status_OUTOFMEMORY and
 * nothing is left allocated, and the first run that has enough allocations
 * gives the tree. The symbols are interned in the same allocator. */
static bool oomSweep(const char* src, Status wantAtTheEnd, const char* wantShape) {
  SymbolSpace sp;
  int fails = 0;
  int n = 0;
  bool ok = true;
  bool finished = false;
  while (!finished && n < 2000) {
    i_FailAllocHeap fa;
    IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
    AST* tree = NULL;
    ParseError err;
    Status st;
    spNew(&mem, &sp);
    fa.allocsUntilFail = n;
    st = run(&mem, &sp, src, 64, &tree, &err);
    fa.allocsUntilFail = -1;
    if (st == status_OUTOFMEMORY) {
      ok = ok && tree == NULL && err.code == status_OUTOFMEMORY;
      fails++;
    } else {
      finished = true;
      ok = ok && st == wantAtTheEnd;
      if (st == status_OK) {
        char buf[2048];
        Out o;
        buf[0] = '\0';
        o.buf = buf;
        o.cap = sizeof(buf);
        o.len = 0;
        shape(&o, tree, &sp, false);
        ok = ok && strcmp(buf, wantShape) == 0;
        ast_free(&mem, tree);
      } else {
        ok = ok && tree == NULL;
      }
    }
    symbolSpace_free(&sp);
    ok = ok && usedBy(&mem) == 0;
    n++;
  }
  return ok && finished && fails > 30;
}

bool test_outOfMemory(void) {
  return oomSweep(
    "let f\n"
    "  function (a, b = 1)\n"
    "    sqrt a.x : T = [1, \"s\\n\"]\n"
    "print 22/7 0xff -5/3 123456789012345678901234567890 \\\n"
    "  (g 5.0f)\n",
    status_OK,
    "P[{let f {function (,a <= b 1>) {sqrt <= <: <. a x> T> [,1 \"s\n\"]>}}} "
    "{print 22/7 255 -5/3 123456789012345678901234567890 (g 5)}]");
}

/* the same when the parse fails at the end: everything that was made is
 * freed, whatever the failure was */
bool test_outOfMemoryOnError(void) {
  return oomSweep(
    "let f\n"
    "  function (a, b = 1)\n"
    "    sqrt a.x : T = [1, \"s\\n\", 22/7, 99999999999999999999]\n"
    "print (g [h 5]) x.y.z \\\n"
    "  w : U )\n",
    status_UNEXPECTEDTOKEN, "");
}

/* every error of the table frees what it had made, with the stack of the
 * parser in many different states: under a failing allocator that can fail
 * in any place, and the same errors */
bool test_errorsNeverLeak(void) {
  const char* sources[] = {
    "a b c d [e f (g h",
    "a.b.c.d.e = ",
    "(f a.b [c : d] \"s\" 1/3",
    "a\n  b c\n    d e\n  f g\n h",
    "f a, b, \\\n  c \\\n  d \\ e",
    "[a, [b, [c, (d, [e, ,",
    "x = [y = ",
  };
  bool ok = true;
  int s = 0;
  while (s < LEN(sources)) {
    int n = 0;
    bool finished = false;
    while (!finished && n < 500) {
      i_FailAllocHeap fa;
      IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
      SymbolSpace sp;
      AST* tree = NULL;
      ParseError err;
      Status st;
      spNew(&mem, &sp);
      fa.allocsUntilFail = n;
      st = run(&mem, &sp, sources[s], 64, &tree, &err);
      fa.allocsUntilFail = -1;
      ok = ok && st != status_OK && tree == NULL;
      finished = st != status_OUTOFMEMORY;
      symbolSpace_free(&sp);
      ok = ok && usedBy(&mem) == 0;
      n++;
    }
    ok = ok && finished;
    s++;
  }
  return ok;
}
/* END: tests */

static Tester tests[] = {
  {"test_atoms", test_atoms},
  {"test_builtinNamesAreSymbols", test_builtinNamesAreSymbols},
  {"test_calls", test_calls},
  {"test_equivalentCalls", test_equivalentCalls},
  {"test_suffixes", test_suffixes},
  {"test_suffixesDoNotChain", test_suffixesDoNotChain},
  {"test_suffixSymbols", test_suffixSymbols},
  {"test_sameSymbols", test_sameSymbols},
  {"test_tuples", test_tuples},
  {"test_commas", test_commas},
  {"test_blocks", test_blocks},
  {"test_semicolon", test_semicolon},
  {"test_continuation", test_continuation},
  {"test_empty", test_empty},
  {"test_specExample", test_specExample},
  {"test_specFunctionArguments", test_specFunctionArguments},
  {"test_ranges", test_ranges},
  {"test_numbersAndStrings", test_numbersAndStrings},
  {"test_errors", test_errors},
  {"test_lexicalErrors", test_lexicalErrors},
  {"test_literalErrors", test_literalErrors},
  {"test_depth", test_depth},
  {"test_depthLimit", test_depthLimit},
  {"test_deepNesting", test_deepNesting},
  {"test_sourceIsCopied", test_sourceIsCopied},
  {"test_deterministic", test_deterministic},
  {"test_outputsOnlyOnTheirCase", test_outputsOnlyOnTheirCase},
  {"test_badSize", test_badSize},
  {"test_outOfMemory", test_outOfMemory},
  {"test_outOfMemoryOnError", test_outOfMemoryOnError},
  {"test_errorsNeverLeak", test_errorsNeverLeak},
};

int main(void) {
  run_tests("parser", tests, LEN(tests));
  return 0;
}
