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
static usize usedBy(const IAllocator* mem) {
  return mem->info(mem->heap).used;
}

static void spNew(SymbolSpace* sp) {
  IAllocator std = stdAlloc_new();
  checkStatus(symbolSpace_new(&std, 8, sp));
}

/* a terminal symbol of the token of the kind `kind`, with the text `text`, at
 * the line 0 and the column `col`. */
static Status mkSym(IAllocator* mem, SymbolSpace* sp, TokenKind kind,
                    const char* text, u32 col, AST** out) {
  Token t;
  Lexeme tk;
  Symbol sym;
  Str s;
  Status st;
  t.kind = kind;
  t.start = 0;
  t.end = (u32)strlen(text);
  t.line = 0;
  t.col = col;
  st = lexeme_new(mem, &t, (const u8*)text, strlen(text), &tk);
  if (st != status_OK) {
    return st;
  }
  s.data = text;
  s.length = strlen(text);
  checkStatus(symbolSpace_intern(sp, &s, &sym));
  st = ast_newSymbol(mem, tk, sym, out);
  if (st != status_OK) {
    (void)lexeme_free(mem, tk);
  }
  return st;
}

static bool dumpIs(const AST* node, const SymbolSpace* sp, const char* want) {
  char buf[1024];
  usize n = ast_snprint(node, sp, buf, sizeof(buf));
  return n == strlen(want) && memcmp(buf, want, n) == 0;
}

/* `x . a b`, like the parser makes `x.a.b`; three symbols and the call. */
static Status buildSuffix(IAllocator* mem, SymbolSpace* sp, AST** out) {
  AST* kids[3];
  Status st = mkSym(mem, sp, tokenKind_DOT, ".", 1, &kids[0]);
  if (st != status_OK) {
    return st;
  }
  st = mkSym(mem, sp, tokenKind_ID, "x", 0, &kids[1]);
  if (st != status_OK) {
    ast_free(mem, kids[0]);
    return st;
  }
  st = mkSym(mem, sp, tokenKind_ID, "a", 2, &kids[2]);
  if (st != status_OK) {
    ast_free(mem, kids[0]);
    ast_free(mem, kids[1]);
    return st;
  }
  st = ast_newFormNode(mem, astKind_CALL, astForm_SUFFIX, false, kids, 3,
                       range_none(), out);
  if (st != status_OK) {
    ast_free(mem, kids[0]);
    ast_free(mem, kids[1]);
    ast_free(mem, kids[2]);
  }
  return st;
}
/* END: helpers */

/* BEGIN: tests */
bool test_tupleKind(void) {
  return strcmp(astKind_name(astKind_TUPLE), "TUPLE") == 0 &&
         astKind_isComposite(astKind_TUPLE) &&
         !astKind_isTerminal(astKind_TUPLE) &&
         astKind_isComposite(astKind_CALL) &&
         astKind_isTerminal(astKind_SYMBOL);
}

bool test_formNames(void) {
  return strcmp(astForm_name(astForm_NONE), "") == 0 &&
         strcmp(astForm_name(astForm_SEXPR), "sexpr") == 0 &&
         strcmp(astForm_name(astForm_IEXPR), "iexpr") == 0 &&
         strcmp(astForm_name(astForm_SUFFIX), "suffix") == 0;
}

/* `ast_newNode` makes the nodes that were never written in a form. */
bool test_newNodeDefaults(void) {
  IAllocator mem = stdAlloc_new();
  SymbolSpace sp;
  AST* program;
  AST* call;
  AST* tuple;
  AST* sym;
  bool ok;
  spNew(&sp);
  checkStatus(ast_newNode(&mem, astKind_PROGRAM, NULL, 0, range_none(), &program));
  checkStatus(ast_newNode(&mem, astKind_CALL, NULL, 0, range_none(), &call));
  checkStatus(ast_newNode(&mem, astKind_TUPLE, NULL, 0, range_none(), &tuple));
  checkStatus(mkSym(&mem, &sp, tokenKind_ID, "a", 0, &sym));
  ok = program->form == astForm_NONE && !program->hasCommas &&
       call->form == astForm_NONE && !call->hasCommas &&
       tuple->form == astForm_NONE && !tuple->hasCommas &&
       sym->form == astForm_NONE && !sym->hasCommas;
  ast_free(&mem, program);
  ast_free(&mem, call);
  ast_free(&mem, tuple);
  ast_free(&mem, sym);
  symbolSpace_free(&sp);
  return ok;
}

bool test_newFormNode(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  SymbolSpace sp;
  AST* kids[2];
  AST* call;
  AST* tuple;
  AST* kids2[1];
  bool ok;
  spNew(&sp);
  checkStatus(mkSym(&mem, &sp, tokenKind_ID, "a", 1, &kids[0]));
  checkStatus(mkSym(&mem, &sp, tokenKind_ID, "b", 4, &kids[1]));
  checkStatus(ast_newFormNode(&mem, astKind_CALL, astForm_SEXPR, true, kids, 2,
                              range_none(), &call));
  ok = call->kind == astKind_CALL && call->form == astForm_SEXPR &&
       call->hasCommas && call->leafsLength == 2 &&
       call->leafs[0] == kids[0] && call->leafs[1] == kids[1];
  kids2[0] = call;
  checkStatus(ast_newFormNode(&mem, astKind_TUPLE, astForm_NONE, true, kids2, 1,
                              range_none(), &tuple));
  ok = ok && tuple->kind == astKind_TUPLE && tuple->form == astForm_NONE &&
       tuple->hasCommas && tuple->leafs[0] == call;
  ast_free(&mem, tuple);
  symbolSpace_free(&sp);
  return ok && usedBy(&mem) == 0;
}

/* the operators of the suffixes are symbols, and their lexeme is the token of
 * the operator. */
bool test_operatorSymbols(void) {
  IAllocator mem = stdAlloc_new();
  SymbolSpace sp;
  AST* dot;
  AST* colon;
  AST* equals;
  bool ok;
  spNew(&sp);
  checkStatus(mkSym(&mem, &sp, tokenKind_DOT, ".", 3, &dot));
  checkStatus(mkSym(&mem, &sp, tokenKind_COLON, ":", 0, &colon));
  checkStatus(mkSym(&mem, &sp, tokenKind_EQUALS, "=", 5, &equals));
  ok = dot->kind == astKind_SYMBOL && dot->tk.kind == tokenKind_DOT &&
       colon->kind == astKind_SYMBOL && colon->tk.kind == tokenKind_COLON &&
       equals->kind == astKind_SYMBOL && equals->tk.kind == tokenKind_EQUALS &&
       dot->value.kind == atomKind_SYMBOL &&
       dot->value.value.Symbol != colon->value.value.Symbol &&
       colon->value.value.Symbol != equals->value.value.Symbol;
  ok = ok && dumpIs(dot, &sp, "SYMBOL 1:4-1:5 .\n") &&
       dumpIs(colon, &sp, "SYMBOL 1:1-1:2 :\n") &&
       dumpIs(equals, &sp, "SYMBOL 1:6-1:7 =\n");
  ast_free(&mem, dot);
  ast_free(&mem, colon);
  ast_free(&mem, equals);
  symbolSpace_free(&sp);
  return ok;
}

bool test_formDump(void) {
  IAllocator mem = stdAlloc_new();
  SymbolSpace sp;
  AST* a; AST* b; AST* kids[2];
  AST* sexpr;
  AST* iexpr;
  AST* tuple;
  AST* plain;
  AST* suffix;
  bool ok;
  spNew(&sp);

  checkStatus(mkSym(&mem, &sp, tokenKind_ID, "a", 0, &a));
  checkStatus(mkSym(&mem, &sp, tokenKind_ID, "b", 2, &b));
  kids[0] = a; kids[1] = b;
  checkStatus(ast_newFormNode(&mem, astKind_CALL, astForm_SEXPR, false, kids, 2,
                              range_none(), &sexpr));
  ok = dumpIs(sexpr, &sp,
              "CALL 1:1-1:4 sexpr\n"
              "  SYMBOL 1:1-1:2 a\n"
              "  SYMBOL 1:3-1:4 b\n");
  ast_free(&mem, sexpr);

  checkStatus(mkSym(&mem, &sp, tokenKind_ID, "a", 0, &a));
  checkStatus(mkSym(&mem, &sp, tokenKind_ID, "b", 2, &b));
  kids[0] = a; kids[1] = b;
  checkStatus(ast_newFormNode(&mem, astKind_CALL, astForm_IEXPR, true, kids, 2,
                              range_none(), &iexpr));
  ok = ok && dumpIs(iexpr, &sp,
                    "CALL 1:1-1:4 iexpr commas\n"
                    "  SYMBOL 1:1-1:2 a\n"
                    "  SYMBOL 1:3-1:4 b\n");

  /* a tuple with commas, then without; a tuple has no form */
  kids[0] = iexpr;
  checkStatus(ast_newFormNode(&mem, astKind_TUPLE, astForm_NONE, true, kids, 1,
                              range_none(), &tuple));
  ok = ok && dumpIs(tuple, &sp,
                    "TUPLE 1:1-1:4 commas\n"
                    "  CALL 1:1-1:4 iexpr commas\n"
                    "    SYMBOL 1:1-1:2 a\n"
                    "    SYMBOL 1:3-1:4 b\n");
  ast_free(&mem, tuple);

  checkStatus(ast_newNode(&mem, astKind_TUPLE, NULL, 0, range_none(), &plain));
  ok = ok && dumpIs(plain, &sp, "TUPLE ?\n");
  ast_free(&mem, plain);

  checkStatus(buildSuffix(&mem, &sp, &suffix));
  ok = ok && dumpIs(suffix, &sp,
                    "CALL 1:1-1:4 suffix\n"
                    "  SYMBOL 1:2-1:3 .\n"
                    "  SYMBOL 1:1-1:2 x\n"
                    "  SYMBOL 1:3-1:4 a\n");
  ast_free(&mem, suffix);
  symbolSpace_free(&sp);
  return ok;
}

/* a failing allocator: nothing is taken, and nothing is left behind. */
bool test_formOutOfMemory(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  SymbolSpace sp;
  AST* a; AST* kids[1];
  AST* out = NULL;
  usize before;
  bool ok;
  spNew(&sp);
  checkStatus(mkSym(&mem, &sp, tokenKind_ID, "a", 0, &a));
  kids[0] = a;
  before = usedBy(&mem);

  fa.allocsUntilFail = 0;
  ok = ast_newFormNode(&mem, astKind_CALL, astForm_SEXPR, true, kids, 1,
                       range_none(), &out) == status_OUTOFMEMORY &&
       out == NULL && usedBy(&mem) == before;
  fa.allocsUntilFail = 1;
  ok = ok && ast_newFormNode(&mem, astKind_TUPLE, astForm_NONE, true, kids, 1,
                             range_none(), &out) == status_OUTOFMEMORY &&
       out == NULL && usedBy(&mem) == before;
  fa.allocsUntilFail = -1;
  ast_free(&mem, a);
  symbolSpace_free(&sp);
  return ok && usedBy(&mem) == 0;
}

/* freeing a tree with every composite kind leaves nothing behind. */
bool test_formNoLeak(void) {
  i_FailAllocHeap fa;
  IAllocator mem = i_failAlloc_new(&fa, g_pao_heap, common_HEAPSIZE, -1);
  SymbolSpace sp;
  AST* suffix;
  AST* kids[1];
  AST* tuple;
  AST* program;
  bool ok;
  spNew(&sp);
  checkStatus(buildSuffix(&mem, &sp, &suffix));
  kids[0] = suffix;
  checkStatus(ast_newFormNode(&mem, astKind_TUPLE, astForm_NONE, false, kids, 1,
                              range_none(), &tuple));
  kids[0] = tuple;
  checkStatus(ast_newNode(&mem, astKind_PROGRAM, kids, 1, range_none(), &program));
  ok = usedBy(&mem) > 0;
  ast_free(&mem, program);
  symbolSpace_free(&sp);
  return ok && usedBy(&mem) == 0;
}
/* END: tests */

static Tester tests[] = {
  {"test_tupleKind", test_tupleKind},
  {"test_formNames", test_formNames},
  {"test_newNodeDefaults", test_newNodeDefaults},
  {"test_newFormNode", test_newFormNode},
  {"test_operatorSymbols", test_operatorSymbols},
  {"test_formDump", test_formDump},
  {"test_formOutOfMemory", test_formOutOfMemory},
  {"test_formNoLeak", test_formNoLeak},
};

int main(void) {
  run_tests("astForm", tests, LEN(tests));
  return 0;
}
