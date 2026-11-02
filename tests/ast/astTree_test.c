/*
MIT License
Copyright 2026 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#include <string.h>
#include "../common.h"
#include "../../lib/alloc/stdAlloc.h"
#include "../../lib/ast.h"

#define LEN(array) ((int)(sizeof(array) / sizeof((array)[0])))

static u8 g_pao_heap[common_HEAPSIZE];

/* BEGIN: helpers */
static Range rng(u32 bl, u32 bc, u32 el, u32 ec) {
  Range r = { { bl, bc }, { el, ec } };
  return r;
}

static bool sameRange(Range a, Range b) {
  return a.begin.line == b.begin.line && a.begin.col == b.begin.col &&
         a.end.line == b.end.line && a.end.col == b.end.col;
}

static usize usedBy(const IAllocator* mem) {
  return mem->info(mem->heap).used;
}

static void spNew(SymbolSpace* sp) {
  IAllocator std = stdAlloc_new();
  checkStatus(symbolSpace_new(&std, 8, sp));
}

/* the lexeme of `text`, as if it were a token of that kind starting at
 * the line `line` and the column `col`. */
static Status mkLexeme(IAllocator* mem, TokenKind kind, const char* text,
                       u32 line, u32 col, Lexeme* out) {
  Token t;
  t.kind = kind;
  t.start = 0;
  t.end = (u32)strlen(text);
  t.line = line;
  t.col = col;
  return lexeme_new(mem, &t, (const u8*)text, strlen(text), out);
}

/* The `mk*` functions make a terminal node whole, with its lexeme and value,
 * and leave nothing allocated if they fail. */
static Status mkSym(IAllocator* mem, SymbolSpace* sp, const char* name,
                    u32 line, u32 col, AST** out) {
  Lexeme tk;
  Symbol sym;
  Str s;
  Status st = mkLexeme(mem, tokenKind_ID, name, line, col, &tk);
  if (st != status_OK) {
    return st;
  }
  s.data = name;
  s.length = strlen(name);
  checkStatus(symbolSpace_intern(sp, &s, &sym));
  st = ast_newSymbol(mem, tk, sym, out);
  if (st != status_OK) {
    (void)lexeme_free(mem, tk);
  }
  return st;
}

/* `raw` is the text of the token, `decoded` its bytes after the escapes. */
static Status mkStr(IAllocator* mem, const char* raw, const char* decoded,
                    usize decodedLen, u32 line, u32 col, AST** out) {
  Lexeme tk;
  Str view;
  Str copy;
  Status st = mkLexeme(mem, tokenKind_STR, raw, line, col, &tk);
  if (st != status_OK) {
    return st;
  }
  view.data = decoded;
  view.length = decodedLen;
  st = str_copy(mem, &view, &copy);
  if (st != status_OK) {
    (void)lexeme_free(mem, tk);
    return st;
  }
  st = ast_newString(mem, tk, copy, out);
  if (st != status_OK) {
    (void)str_free(mem, copy);
    (void)lexeme_free(mem, tk);
  }
  return st;
}

static Status mkNumber(IAllocator* mem, const char* raw, Number n, u32 line,
                       u32 col, AST** out) {
  Lexeme tk;
  Status st = mkLexeme(mem, tokenKind_NUM, raw, line, col, &tk);
  if (st != status_OK) {
    return st;
  }
  st = ast_newNumber(mem, tk, n, out);
  if (st != status_OK) {
    (void)lexeme_free(mem, tk);
  }
  return st;
}

static Status mkInt(IAllocator* mem, const char* raw, i64 v, u32 line,
                    u32 col, AST** out) {
  Number n;
  n.kind = numberKind_nativeInt;
  n.data.nativeInt = v;
  return mkNumber(mem, raw, n, line, col, out);
}

static Status mkFloat(IAllocator* mem, const char* raw, double v, u32 line,
                      u32 col, AST** out) {
  Number n;
  n.kind = numberKind_nativeFloat;
  n.data.nativeFloat = v;
  return mkNumber(mem, raw, n, line, col, out);
}

static Status mkInteger(IAllocator* mem, const char* raw, i32 v, u32 line,
                        u32 col, AST** out) {
  Number n;
  Integer* p = (Integer*)mem->alloc(mem->heap, sizeof(Integer), __func__);
  Status st;
  if (p == NULL) {
    return status_OUTOFMEMORY;
  }
  *p = integer_new();
  st = integer_set(mem, v, p);
  if (st == status_OK) {
    n.kind = numberKind_integer;
    n.data.integer = p;
    st = mkNumber(mem, raw, n, line, col, out);
  }
  if (st != status_OK) {
    integer_free(mem, *p);
    (void)mem->free(mem->heap, p);
  }
  return st;
}

static Status mkRational(IAllocator* mem, const char* raw, i8 sign, u32 num,
                         u32 den, u32 line, u32 col, AST** out) {
  Number n;
  Rational* p = (Rational*)mem->alloc(mem->heap, sizeof(Rational), __func__);
  Status st;
  if (p == NULL) {
    return status_OUTOFMEMORY;
  }
  *p = rational_new();
  st = rational_set(mem, sign, num, den, p);
  if (st == status_OK) {
    n.kind = numberKind_rational;
    n.data.rational = p;
    st = mkNumber(mem, raw, n, line, col, out);
  }
  if (st != status_OK) {
    rational_free(mem, *p);
    (void)mem->free(mem->heap, p);
  }
  return st;
}

static Status mkNil(IAllocator* mem, u32 line, u32 col, AST** out) {
  Lexeme tk;
  Status st = mkLexeme(mem, tokenKind_ID, "nil", line, col, &tk);
  if (st != status_OK) {
    return st;
  }
  st = ast_newNil(mem, tk, out);
  if (st != status_OK) {
    (void)lexeme_free(mem, tk);
  }
  return st;
}

static Status mkBool(IAllocator* mem, const char* raw, bool b, u32 line,
                     u32 col, AST** out) {
  Lexeme tk;
  Status st = mkLexeme(mem, tokenKind_ID, raw, line, col, &tk);
  if (st != status_OK) {
    return st;
  }
  st = ast_newBool(mem, tk, b, out);
  if (st != status_OK) {
    (void)lexeme_free(mem, tk);
  }
  return st;
}

/* Keeps `node` in `kids` if `st` is good. */
static Status keep(Status st, AST* node, AST** kids, usize* count) {
  if (st == status_OK) {
    kids[*count] = node;
    *count += 1;
  }
  return st;
}

static void dropAll(IAllocator* mem, AST** kids, usize count) {
  while (count > 0) {
    count--;
    ast_free(mem, kids[count]);
  }
}

/* The tree of
 *   [foo "a\nb" 12 -123 22/7]
 *   nil true
 * Whatever fails, nothing stays allocated. */
static Status buildTree(IAllocator* mem, SymbolSpace* sp, AST** out) {
  AST* inner[5];
  AST* top[3];
  AST* node = NULL;
  usize ni = 0;
  usize nt = 0;
  Status st;
  st = mkSym(mem, sp, "foo", 0, 1, &node);
  st = keep(st, node, inner, &ni);
  if (st == status_OK) {
    st = mkStr(mem, "\"a\\nb\"", "a\nb", 3, 0, 5, &node);
    st = keep(st, node, inner, &ni);
  }
  if (st == status_OK) {
    st = mkInt(mem, "12", 12, 0, 12, &node);
    st = keep(st, node, inner, &ni);
  }
  if (st == status_OK) {
    st = mkInteger(mem, "-123", -123, 0, 15, &node);
    st = keep(st, node, inner, &ni);
  }
  if (st == status_OK) {
    st = mkRational(mem, "22/7", +1, 22, 7, 0, 20, &node);
    st = keep(st, node, inner, &ni);
  }
  if (st == status_OK) {
    st = ast_newNode(mem, astKind_CALL, inner, ni, rng(0, 0, 0, 25), &node);
    if (st == status_OK) {
      ni = 0; // taken
      st = keep(st, node, top, &nt);
    }
  }
  if (st == status_OK) {
    st = mkNil(mem, 1, 0, &node);
    st = keep(st, node, top, &nt);
  }
  if (st == status_OK) {
    st = mkBool(mem, "true", true, 1, 4, &node);
    st = keep(st, node, top, &nt);
  }
  if (st == status_OK) {
    st = ast_newNode(mem, astKind_PROGRAM, top, nt, range_none(), out);
  }
  if (st != status_OK) {
    dropAll(mem, inner, ni);
    dropAll(mem, top, nt);
  }
  return st;
}

#define TREE_DUMP \
  "PROGRAM 1:1-2:9\n" \
  "  CALL 1:1-1:26\n" \
  "    SYMBOL 1:2-1:5 foo\n" \
  "    STRING 1:6-1:12 \"a\\nb\"\n" \
  "    NUMBER 1:13-1:15 12\n" \
  "    NUMBER 1:16-1:20 -123\n" \
  "    NUMBER 1:21-1:25 22/7\n" \
  "  NIL 2:1-2:4 nil\n" \
  "  BOOL 2:5-2:9 true\n"

/* the dump of `node` is exactly `want`, and it needs exactly that room. */
static bool dumpIs(const AST* node, const SymbolSpace* sp, const char* want) {
  char buf[1024];
  usize len = strlen(want);
  usize n = ast_snprint(node, sp, buf, sizeof(buf));
  if (n != len || memcmp(buf, want, len) != 0) {
    printf("  dump has %zu bytes, wanted %zu:\n%.*s", n, len, (int)n, buf);
    return false;
  }
  return true;
}
/* END: helpers */

/* BEGIN: tests */
bool test_astKinds(void) {
  bool ok = true;
  ok = ok && strcmp(astKind_name(astKind_PROGRAM), "PROGRAM") == 0;
  ok = ok && strcmp(astKind_name(astKind_CALL), "CALL") == 0;
  ok = ok && strcmp(astKind_name(astKind_SYMBOL), "SYMBOL") == 0;
  ok = ok && strcmp(astKind_name(astKind_STRING), "STRING") == 0;
  ok = ok && strcmp(astKind_name(astKind_NUMBER), "NUMBER") == 0;
  ok = ok && strcmp(astKind_name(astKind_NIL), "NIL") == 0;
  ok = ok && strcmp(astKind_name(astKind_BOOL), "BOOL") == 0;
  ok = ok && astKind_isComposite(astKind_PROGRAM) &&
       astKind_isComposite(astKind_CALL);
  ok = ok && astKind_isTerminal(astKind_SYMBOL) &&
       astKind_isTerminal(astKind_STRING) && astKind_isTerminal(astKind_NUMBER) &&
       astKind_isTerminal(astKind_NIL) && astKind_isTerminal(astKind_BOOL);
  ok = ok && !astKind_isTerminal(astKind_CALL) &&
       !astKind_isComposite(astKind_NIL);
  return ok;
}

/* each terminal keeps its kind, value, lexeme and range. */
bool test_astTerminals(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  SymbolSpace sp;
  AST* sym; AST* str; AST* num; AST* flt; AST* integer; AST* rat; AST* nil;
  AST* t; AST* f;
  Str name;
  bool ok = true;
  spNew(&sp);

  checkStatus(mkSym(&mem, &sp, "ab", 3, 4, &sym));
  checkStatus(mkStr(&mem, "\"x\\ty\"", "x\ty", 3, 3, 0, &str));
  checkStatus(mkInt(&mem, "-7", -7, 1, 2, &num));
  checkStatus(mkFloat(&mem, "2.5", 2.5, 1, 2, &flt));
  checkStatus(mkInteger(&mem, "99", 99, 1, 2, &integer));
  checkStatus(mkRational(&mem, "-1/3", -1, 1, 3, 1, 2, &rat));
  checkStatus(mkNil(&mem, 5, 6, &nil));
  checkStatus(mkBool(&mem, "true", true, 0, 0, &t));
  checkStatus(mkBool(&mem, "false", false, 0, 0, &f));

  name = symbolSpace_findSymbol(&sp, sym->value.value.Symbol);
  ok = ok && sym->kind == astKind_SYMBOL && sym->value.kind == atomKind_SYMBOL &&
       name.length == 2 && memcmp(name.data, "ab", 2) == 0 &&
       sameRange(sym->range, rng(3, 4, 3, 6)) &&
       sameRange(sym->tk.range, sym->range) && sym->tk.kind == tokenKind_ID &&
       sym->tk.text.length == 2 && sym->leafs == NULL && sym->leafsLength == 0;
  ok = ok && str->kind == astKind_STRING && str->value.kind == atomKind_STRING &&
       str->value.value.Str.length == 3 &&
       memcmp(str->value.value.Str.data, "x\ty", 3) == 0 &&
       str->tk.text.length == 6 && sameRange(str->range, rng(3, 0, 3, 6)) &&
       str->leafs == NULL;
  ok = ok && num->kind == astKind_NUMBER && num->value.kind == atomKind_NUMBER &&
       num->value.value.Number.kind == numberKind_nativeInt &&
       num->value.value.Number.data.nativeInt == -7;
  ok = ok && flt->value.value.Number.kind == numberKind_nativeFloat &&
       flt->value.value.Number.data.nativeFloat == 2.5;
  ok = ok && integer->value.value.Number.kind == numberKind_integer &&
       integer->value.value.Number.data.integer != NULL;
  ok = ok && rat->value.value.Number.kind == numberKind_rational &&
       rat->value.value.Number.data.rational->sign == -1;
  ok = ok && nil->kind == astKind_NIL && nil->value.kind == atomKind_NULL &&
       nil->tk.text.length == 3 && sameRange(nil->range, rng(5, 6, 5, 9));
  ok = ok && t->kind == astKind_BOOL && t->value.kind == atomKind_BOOL &&
       t->value.value.Bool == true && f->value.value.Bool == false;
  ok = ok && usedBy(&mem) > 0;

  ast_free(&mem, sym); ast_free(&mem, str); ast_free(&mem, num);
  ast_free(&mem, flt); ast_free(&mem, integer); ast_free(&mem, rat);
  ast_free(&mem, nil); ast_free(&mem, t); ast_free(&mem, f);
  symbolSpace_free(&sp);
  return ok && usedBy(&mem) == 0;
}

/* the children keep their order, the array is copied, and the range is the
 * union of the ranges of the children and the one that was given. */
bool test_astNodeOrderAndRange(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  SymbolSpace sp;
  AST* a; AST* b; AST* c;
  AST* kids[3];
  AST* call;
  AST* bare;
  AST* wide;
  bool ok = true;
  spNew(&sp);
  checkStatus(mkSym(&mem, &sp, "a", 0, 1, &a));
  checkStatus(mkSym(&mem, &sp, "b", 0, 3, &b));
  checkStatus(mkSym(&mem, &sp, "c", 1, 2, &c));
  kids[0] = a; kids[1] = b; kids[2] = c;

  /* with the brackets */
  checkStatus(ast_newNode(&mem, astKind_CALL, kids, 3, rng(0, 0, 1, 4), &call));
  kids[0] = NULL; kids[1] = NULL; kids[2] = NULL; // the copy is the node's
  ok = ok && call->kind == astKind_CALL && call->leafsLength == 3 &&
       call->leafs != NULL && call->leafs[0] == a && call->leafs[1] == b &&
       call->leafs[2] == c;
  ok = ok && sameRange(call->range, rng(0, 0, 1, 4));
  ok = ok && call->tk.text.data == NULL && call->tk.text.length == 0;

  /* no range but the children's: from the first begin to the last end */
  kids[0] = c; kids[1] = a;
  checkStatus(ast_newNode(&mem, astKind_CALL, kids, 2, range_none(), &bare));
  ok = ok && bare->leafs[0] == c && bare->leafs[1] == a &&
       sameRange(bare->range, rng(0, 1, 1, 3));

  /* a range inside the children's changes nothing */
  kids[0] = b;
  checkStatus(ast_newNode(&mem, astKind_PROGRAM, kids, 1, rng(0, 3, 0, 3), &wide));
  ok = ok && sameRange(wide->range, rng(0, 3, 0, 4));

  /* the nodes of `bare` and `wide` are borrowed: only `call` owns them */
  bare->leafs[0] = NULL; bare->leafs[1] = NULL;
  wide->leafs[0] = NULL;
  ast_free(&mem, bare);
  ast_free(&mem, wide);
  ast_free(&mem, call);
  symbolSpace_free(&sp);
  return ok && usedBy(&mem) == 0;
}

/* a composite with no children has no array, and has the range it was given. */
bool test_astEmpty(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  AST* program;
  AST* call;
  bool ok;
  checkStatus(ast_newNode(&mem, astKind_PROGRAM, NULL, 0, rng(3, 0, 3, 0), &program));
  checkStatus(ast_newNode(&mem, astKind_CALL, NULL, 0, range_none(), &call));
  ok = program->leafs == NULL && program->leafsLength == 0 &&
       sameRange(program->range, rng(3, 0, 3, 0)) &&
       range_isEmpty(program->range);
  ok = ok && call->leafs == NULL && call->leafsLength == 0 &&
       range_isNone(call->range);
  ast_free(&mem, program);
  ast_free(&mem, call);
  return ok && usedBy(&mem) == 0;
}

/* a `nil` that was written, a node that is only structure, and a node with no
 * value are different things: it is the kind that tells. */
bool test_astNilIsNotAbsence(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  AST* nil;
  AST* call;
  usize i = 0;
  bool zero = true;
  bool ok;
  checkStatus(mkNil(&mem, 0, 0, &nil));
  checkStatus(ast_newNode(&mem, astKind_CALL, NULL, 0, range_none(), &call));
  while (i < sizeof(call->value.value)) {
    zero = zero && ((const u8*)&call->value.value)[i] == 0;
    i++;
  }
  ok = nil->kind != call->kind && astKind_isTerminal(nil->kind) &&
       !astKind_isTerminal(call->kind);
  ok = ok && nil->tk.kind == tokenKind_ID && nil->tk.text.length == 3;
  ok = ok && call->tk.text.data == NULL && call->tk.text.length == 0 && zero;
  ast_free(&mem, nil);
  ast_free(&mem, call);
  return ok && usedBy(&mem) == 0;
}

bool test_astFreeNull(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  ast_free(&mem, NULL);
  return usedBy(&mem) == 0;
}

/* a whole tree is freed without leaving anything behind. */
bool test_astNoLeak(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  SymbolSpace sp;
  AST* tree;
  bool ok;
  spNew(&sp);
  checkStatus(buildTree(&mem, &sp, &tree));
  ok = usedBy(&mem) > 0 && tree->leafsLength == 3 &&
       tree->leafs[0]->leafsLength == 5;
  ast_free(&mem, tree);
  symbolSpace_free(&sp);
  return ok && usedBy(&mem) == 0;
}

/* the allocation fails at each point of building the tree: it is always
 * status_OUTOFMEMORY and nothing leaks, and the first run that has enough
 * allocations builds it. */
bool test_astOutOfMemory(void) {
  SymbolSpace sp;
  int fails = 0;
  int n = 0;
  bool ok = true;
  bool built = false;
  spNew(&sp);
  while (!built && n < 200) {
    i_FailAllocHeap fa;
    IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, n);
    AST* tree = NULL;
    Status st = buildTree(&mem, &sp, &tree);
    if (st == status_OK) {
      built = tree != NULL && dumpIs(tree, &sp, TREE_DUMP);
      ast_free(&mem, tree);
    } else {
      ok = ok && st == status_OUTOFMEMORY;
      fails++;
    }
    ok = ok && usedBy(&mem) == 0;
    n++;
  }
  symbolSpace_free(&sp);
  return ok && built && fails > 20;
}

/* when the array of the children cannot be allocated, the children are still
 * the caller's and nothing is left of the node. */
bool test_astNodeOutOfMemory(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  SymbolSpace sp;
  AST* a; AST* kids[1];
  AST* out = NULL;
  usize before;
  bool ok;
  spNew(&sp);
  checkStatus(mkSym(&mem, &sp, "a", 0, 0, &a));
  kids[0] = a;
  before = usedBy(&mem);

  fa.allocsUntilFail = 0; // not even the node
  ok = ast_newNode(&mem, astKind_CALL, kids, 1, range_none(), &out) ==
       status_OUTOFMEMORY && out == NULL && usedBy(&mem) == before;
  fa.allocsUntilFail = 1; // the node, but not the array
  ok = ok && ast_newNode(&mem, astKind_CALL, kids, 1, range_none(), &out) ==
       status_OUTOFMEMORY && out == NULL && usedBy(&mem) == before;
  fa.allocsUntilFail = -1;
  ast_free(&mem, a);
  symbolSpace_free(&sp);
  return ok && usedBy(&mem) == 0;
}

/* the same for a terminal: it takes nothing when it fails. */
bool test_astTerminalOutOfMemory(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  Lexeme tk;
  Str s = { "x", 1 };
  Str copy;
  AST* out = NULL;
  bool ok;
  checkStatus(mkLexeme(&mem, tokenKind_STR, "\"x\"", 0, 0, &tk));
  checkStatus(str_copy(&mem, &s, &copy));
  fa.allocsUntilFail = 0;
  ok = ast_newString(&mem, tk, copy, &out) == status_OUTOFMEMORY && out == NULL;
  fa.allocsUntilFail = -1;
  checkStatus(ast_newString(&mem, tk, copy, &out)); // it was all still ours
  ok = ok && out != NULL;
  ast_free(&mem, out);
  return ok && usedBy(&mem) == 0;
}

bool test_astSnprintTree(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  SymbolSpace sp;
  AST* tree;
  char buf[1024];
  usize want = strlen(TREE_DUMP);
  usize i = 0;
  bool ok;
  spNew(&sp);
  checkStatus(buildTree(&mem, &sp, &tree));
  ok = dumpIs(tree, &sp, TREE_DUMP);

  /* it fits exactly in `want` bytes, and it has no NUL */
  memset(buf, 'X', sizeof(buf));
  ok = ok && ast_snprint(tree, &sp, buf, want) == want &&
       memcmp(buf, TREE_DUMP, want) == 0 && buf[want] == 'X';

  /* one byte less, or nothing: nothing is written */
  memset(buf, 'X', sizeof(buf));
  ok = ok && ast_snprint(tree, &sp, buf, want - 1) == 0 &&
       ast_snprint(tree, &sp, buf, 0) == 0;
  while (i < sizeof(buf)) {
    ok = ok && buf[i] == 'X';
    i++;
  }

  /* the same text every time */
  ok = ok && dumpIs(tree, &sp, TREE_DUMP);
  ast_free(&mem, tree);
  symbolSpace_free(&sp);
  return ok && usedBy(&mem) == 0;
}

/* strings are written escaped, and in a single line. */
bool test_astSnprintEscapes(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  SymbolSpace sp;
  AST* str;
  AST* empty;
  const char decoded[] = "a\"b\\c\t\n\x01\x7f\r\xc3\xa9";
  bool ok;
  spNew(&sp);
  checkStatus(mkStr(&mem, "\"x\"", decoded, sizeof(decoded) - 1, 0, 0, &str));
  checkStatus(mkStr(&mem, "\"\"", "", 0, 4, 2, &empty));
  ok = dumpIs(str, &sp,
              "STRING 1:1-1:4 \"a\\\"b\\\\c\\t\\n\\x01\\x7f\\x0d\xc3\xa9\"\n");
  ok = ok && dumpIs(empty, &sp, "STRING 5:3-5:5 \"\"\n");
  ast_free(&mem, str);
  ast_free(&mem, empty);
  symbolSpace_free(&sp);
  return ok && usedBy(&mem) == 0;
}

bool test_astSnprintNumbers(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  SymbolSpace sp;
  AST* n[8];
  bool ok = true;
  int i = 0;
  spNew(&sp);
  checkStatus(mkInt(&mem, "0", 0, 0, 0, &n[0]));
  checkStatus(mkInt(&mem, "-42", -42, 0, 0, &n[1]));
  checkStatus(mkInt(&mem, "9", INT64_MIN, 0, 0, &n[2]));
  checkStatus(mkFloat(&mem, "1", 1.5, 0, 0, &n[3]));
  checkStatus(mkFloat(&mem, "1", 0.1, 0, 0, &n[4]));
  checkStatus(mkInteger(&mem, "1", -123, 0, 0, &n[5]));
  checkStatus(mkRational(&mem, "1", +1, 22, 7, 0, 0, &n[6]));
  checkStatus(mkRational(&mem, "1", -1, 1, 3, 0, 0, &n[7]));
  ok = ok && dumpIs(n[0], &sp, "NUMBER 1:1-1:2 0\n");
  ok = ok && dumpIs(n[1], &sp, "NUMBER 1:1-1:4 -42\n");
  ok = ok && dumpIs(n[2], &sp, "NUMBER 1:1-1:2 -9223372036854775808\n");
  ok = ok && dumpIs(n[3], &sp, "NUMBER 1:1-1:2 1.5\n");
  ok = ok && dumpIs(n[4], &sp, "NUMBER 1:1-1:2 0.10000000000000001\n");
  ok = ok && dumpIs(n[5], &sp, "NUMBER 1:1-1:2 -123\n");
  ok = ok && dumpIs(n[6], &sp, "NUMBER 1:1-1:2 22/7\n");
  ok = ok && dumpIs(n[7], &sp, "NUMBER 1:1-1:2 -1/3\n");
  while (i < 8) {
    ast_free(&mem, n[i]);
    i++;
  }
  symbolSpace_free(&sp);
  return ok && usedBy(&mem) == 0;
}

/* ranges with no width, ranges that are not defined, and empty composites. */
bool test_astSnprintComposites(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  SymbolSpace sp;
  AST* program;
  AST* call;
  AST* kids[1];
  AST* outer;
  bool ok;
  spNew(&sp);
  checkStatus(ast_newNode(&mem, astKind_PROGRAM, NULL, 0, rng(3, 0, 3, 0), &program));
  checkStatus(ast_newNode(&mem, astKind_CALL, NULL, 0, range_none(), &call));
  ok = dumpIs(program, &sp, "PROGRAM 4:1\n") && dumpIs(call, &sp, "CALL ?\n");
  kids[0] = call;
  checkStatus(ast_newNode(&mem, astKind_CALL, kids, 1, rng(0, 0, 0, 2), &outer));
  ok = ok && dumpIs(outer, &sp, "CALL 1:1-1:3\n  CALL ?\n");
  ast_free(&mem, outer);
  ast_free(&mem, program);
  symbolSpace_free(&sp);
  return ok && usedBy(&mem) == 0;
}
/* END: tests */

static Tester tests[] = {
  {"test_astKinds", test_astKinds},
  {"test_astTerminals", test_astTerminals},
  {"test_astNodeOrderAndRange", test_astNodeOrderAndRange},
  {"test_astEmpty", test_astEmpty},
  {"test_astNilIsNotAbsence", test_astNilIsNotAbsence},
  {"test_astFreeNull", test_astFreeNull},
  {"test_astNoLeak", test_astNoLeak},
  {"test_astOutOfMemory", test_astOutOfMemory},
  {"test_astNodeOutOfMemory", test_astNodeOutOfMemory},
  {"test_astTerminalOutOfMemory", test_astTerminalOutOfMemory},
  {"test_astSnprintTree", test_astSnprintTree},
  {"test_astSnprintEscapes", test_astSnprintEscapes},
  {"test_astSnprintNumbers", test_astSnprintNumbers},
  {"test_astSnprintComposites", test_astSnprintComposites},
};

int main(void) {
  run_tests("astTree", tests, LEN(tests));
  return 0;
}
