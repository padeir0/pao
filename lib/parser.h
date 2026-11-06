/*
MIT License
Copyright 2026 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_PARSER_H
#define PAO_PARSER_H

#include "basicTypes.h"
#include "status.h"
#include "config.h"
#include "str.h"
#include "iallocator.h"
#include "lexer.h"
#include "ast.h"
#include "symbol.h"
#include "literal.h"
#include "alloc/stackAlloc.h"

#if config_DEBUG
  #include "debug.h"
#endif

/* The Ovo parser. It turns the source text into an `AST` (see `ast.h`),
 * recursive descent, one function for each production of the grammar in
 * `docs/language/language-spec.md`, and the indentation is handled with the
 * columns of the tokens: there is no INDENT or DEDENT token.
 *
 * What the tree looks like (the dump of `ast_snprint` shows it):
 *   Program        a `PROGRAM`, whose children are the expressions of the
 *                  top level. An empty program (nothing, or only comments and
 *                  blank lines) is a `PROGRAM` without children, whose range
 *                  is the end of the input.
 *   id, num, str   a `SYMBOL`, a `NUMBER` and a `STRING`. `nil`, `true` and
 *                  `false` are `SYMBOL`s too: it is up to the evaluator to
 *                  know them.
 *   S_Expr         `(a b c)` is a `CALL` (form `SEXPR`) of the three terms,
 *                  and `(a)` is a `CALL` with one child. The range goes from
 *                  the `(` to the `)`.
 *   Tuple_Expr     `[a b c]` is a `TUPLE` of the terms. It is never
 *                  unwrapped, not even `[a]`. The range goes from the `[` to
 *                  the `]`.
 *   I_Expr         a line with more than one term, or with a block, is one
 *                  flat `CALL` (form `IEXPR`) whose children are the terms of
 *                  the line, of the lines that continue it with `\`, and the
 *                  expressions of the indented block, in this order. A line
 *                  with only one term and no block is that term.
 *   suffixes       `.`, `:` and `=` become a `CALL` (form `SUFFIX`) whose
 *                  first child is a `SYMBOL` of the operator, and the others
 *                  are the operands: `a.b.c` is `(. a b c)`, `a : T` is
 *                  `(: a T)`, `x = 1` is `(= x 1)`. They bind in this order:
 *                  `.` first, then `:`, and `=` last. Only `.` chains, so
 *                  `a = b = c` and `a : b : c` are errors, and
 *                  `let a.b = c.d` is `(let (= (. a b) (. c d)))`.
 *   commas         never are nodes, but a `CALL` or a `TUPLE` that was
 *                  written with a `,` among its terms has `hasCommas`. A
 *                  `,` after the only term of a line is lost, because a line
 *                  of one term is that term.
 *   `;`            closes the block that its line belongs to, and is not in
 *                  the tree.
 *
 * Indentation: the first token of the program sets the column of the top
 * level, whatever it is. The expressions of a block have exactly the same
 * column. A line that is more indented than the one before it starts a block
 * of that line, and it is a block of the column of its first line. A line
 * that is less indented ends the blocks that are more indented than it, and
 * its column must be the one of a block that is still open (`BADDEDENT`
 * otherwise). A line that is more indented but opens no block (because the
 * line before it ended in `;`) is `BADINDENT`, and so is the line after a `\`
 * if it is not more indented than the line it continues.
 *
 * Errors: every one of them stops the parse, and `ParseError` says what and
 * where: the `code` is the `Status` that was returned, `range` is the range
 * of the token that caused it (the one that was unexpected, or the invalid
 * token of the lexer), and `expected` is a text, or NULL, with what the
 * parser was looking for. A lexical error is the status of the lexer
 * (`status_INVALIDUTF8`, `status_BADCHAR`, `status_BADESCAPE`,
 * `status_UNTERMINATEDSTRING`, `status_BADNUMBER`), and a number or a string
 * that the lexer accepts but `literal.h` does not has the status of the
 * `literal_*` function (including `status_UNSUPPORTEDNUMBER`, which is what
 * `3.14` is for now). The statuses of the parser are `status_UNEXPECTEDTOKEN`,
 * `status_BADINDENT`, `status_BADDEDENT` and `status_MAXDEPTH`; and there are
 * `status_OUTOFMEMORY` and `status_BADSIZE` (a source that does not fit in 32
 * bits).
 */

/* The depth `ovo parse` uses. */
#define parser_DEFAULTMAXDEPTH 256

typedef struct {
  Status code;
  Range range;
  /* static text, never freed, or NULL */
  const char* expected;
} ParseError;

/* BEGIN: internal helpers */

/* the children of the node that is being built are in the work stack, one
 * chunk for each, side by side. `first` is the first, and `n` is how many.
 * The function that declares a group is the one that drops it on failure. */
typedef struct {
  AST** first;
  usize n;
} i_ParserGroup;

typedef struct {
  const IAllocator* mem;
  SymbolSpace* sp;
  const u8* src;
  usize srcLen;
  Lexer lexer;
  /* the token under the cursor, which is not consumed yet */
  Token tok;
  usize depth;
  usize maxDepth;
  StackAlloc* stack;
  ParseError err;
} i_Parser;

static inline Status i_parser_term(i_Parser* p, AST** out);
static inline Status i_parser_factor2(i_Parser* p, AST** out);
static inline Status i_parser_factor3(i_Parser* p, AST** out);
static inline Status i_parser_block(i_Parser* p, u32 col, i_ParserGroup* g);

/* the range of the token under the cursor. */
static inline
Range i_parser_tokenRange(const i_Parser* p) {
  Range r;
  range_fromToken(&p->tok, p->src, p->srcLen, &r);
  return r;
}

/* records the error, for the token under the cursor, and returns `code`. */
static inline
Status i_parser_fail(i_Parser* p, Status code, const char* expected) {
  p->err.code = code;
  p->err.range = i_parser_tokenRange(p);
  p->err.expected = expected;
  return code;
}

/* moves the cursor to the next token. An invalid token of the lexer is an
 * error: its status is the one of the parse, and the token is the one that is
 * reported. */
static inline
Status i_parser_advance(i_Parser* p) {
  Status st = lexer_next(&p->lexer, &p->tok);
  if (st != status_OK) {
    return i_parser_fail(p, st, NULL);
  }
  return status_OK;
}

/* skips the `NL`s, which are the same as a single one. */
static inline
Status i_parser_skipNewlines(i_Parser* p) {
  Status st = status_OK;
  while (st == status_OK && p->tok.kind == tokenKind_NL) {
    st = i_parser_advance(p);
  }
  return st;
}

/* true for the tokens that can begin a `Term`. */
static inline
bool i_parser_startsTerm(TokenKind k) {
  return k == tokenKind_ID || k == tokenKind_NUM || k == tokenKind_STR ||
         k == tokenKind_LBRACKET || k == tokenKind_LPAREN;
}

/* goes one level deeper (a bracket, a parenthesis or an indented block),
 * unless that is more than the parser may go. */
static inline
Status i_parser_enter(i_Parser* p) {
  if (p->depth >= p->maxDepth) {
    return i_parser_fail(p, status_MAXDEPTH, NULL);
  }
  p->depth += 1; // SAFE(1)
  return status_OK;

  /* SAFE(1): `depth` is less than `maxDepth`, which is a `usize`. */
}

/* puts `node` at the end of the group. The node is taken even if this
 * fails, and then it is freed. */
static inline
Status i_parser_push(i_Parser* p, i_ParserGroup* g, AST* node) {
  AST** slot = (AST**)stackAlloc_alloc(p->stack);
  if (slot == NULL) {
    ast_free(p->mem, node); // NOTE(1)
    return i_parser_fail(p, status_OUTOFMEMORY, NULL);
  }
  *slot = node;
  if (g->n == 0) {
    g->first = slot;
  }
  g->n += 1; // SAFE(2)
  return status_OK;

  /* NOTE(1): the stack has a chunk for each byte of the source, which is
   * more than the nodes that wait there at the same time, so this does not
   * happen. It is checked anyway, as every allocation is.
   * SAFE(2): the group never has more nodes than the stack has chunks. */
}

/* frees the nodes of the group, and takes them off the stack. */
static inline
void i_parser_drop(i_Parser* p, i_ParserGroup g) {
  usize i = g.n;
  while (i > 0) {
    i--;
    ast_free(p->mem, g.first[i]);
    stackAlloc_pop(p->stack);
  }
}

/* makes a composite node of the group, which is always consumed: on success
 * its nodes belong to the new node, and on failure they were freed. */
static inline
Status i_parser_build(i_Parser* p, ASTKind kind, ASTForm form, bool commas,
                      i_ParserGroup g, Range range, AST** out) {
  usize i = g.n;
  Status st = ast_newFormNode(p->mem, kind, form, commas,
                              g.n == 0 ? NULL : g.first, g.n, range, out);
  if (st != status_OK) {
    i_parser_drop(p, g);
    return i_parser_fail(p, st, NULL);
  }
  while (i > 0) {
    stackAlloc_pop(p->stack);
    i--;
  }
  return status_OK;
}

/* the `SYMBOL` of the token under the cursor, which is an id or the operator
 * of a suffix. It does not move the cursor. */
static inline
Status i_parser_symbol(i_Parser* p, AST** out) {
  Lexeme lex;
  Symbol sym;
  AST* node;
  Status st = lexeme_new(p->mem, &p->tok, p->src, p->srcLen, &lex);
  if (st != status_OK) {
    return i_parser_fail(p, st, NULL);
  }
  st = symbolSpace_intern(p->sp, &lex.text, &sym);
  if (st != status_OK) {
    (void)lexeme_free(p->mem, lex);
    return i_parser_fail(p, st, NULL);
  }
  st = ast_newSymbol(p->mem, lex, sym, &node);
  if (st != status_OK) {
    (void)lexeme_free(p->mem, lex);
    return i_parser_fail(p, st, NULL);
  }
  *out = node;
  return status_OK;
}

/* the `NUMBER` of the `NUM` under the cursor, which does not move. */
static inline
Status i_parser_number(i_Parser* p, AST** out) {
  Lexeme lex;
  Number num;
  AST* node;
  Status st = lexeme_new(p->mem, &p->tok, p->src, p->srcLen, &lex);
  if (st != status_OK) {
    return i_parser_fail(p, st, NULL);
  }
  st = literal_toNumber(p->mem, &lex.text, &num);
  if (st != status_OK) {
    (void)lexeme_free(p->mem, lex);
    return i_parser_fail(p, st, NULL);
  }
  st = ast_newNumber(p->mem, lex, num, &node);
  if (st != status_OK) {
    literal_freeNumber(p->mem, num);
    (void)lexeme_free(p->mem, lex);
    return i_parser_fail(p, st, NULL);
  }
  *out = node;
  return status_OK;
}

/* the `STRING` of the `STR` under the cursor, which does not move. */
static inline
Status i_parser_string(i_Parser* p, AST** out) {
  Lexeme lex;
  Str value;
  AST* node;
  Status st = lexeme_new(p->mem, &p->tok, p->src, p->srcLen, &lex);
  if (st != status_OK) {
    return i_parser_fail(p, st, NULL);
  }
  st = literal_toString(p->mem, &lex.text, &value);
  if (st != status_OK) {
    (void)lexeme_free(p->mem, lex);
    return i_parser_fail(p, st, NULL);
  }
  st = ast_newString(p->mem, lex, value, &node);
  if (st != status_OK) {
    (void)str_free(p->mem, value);
    (void)lexeme_free(p->mem, lex);
    return i_parser_fail(p, st, NULL);
  }
  *out = node;
  return status_OK;
}

/* Atom = id | num | str; the cursor moves past it. */
static inline
Status i_parser_atom(i_Parser* p, AST** out) {
  AST* node = NULL;
  Status st;
  if (p->tok.kind == tokenKind_NUM) {
    st = i_parser_number(p, &node);
  } else if (p->tok.kind == tokenKind_STR) {
    st = i_parser_string(p, &node);
  } else {
    st = i_parser_symbol(p, &node);
  }
  if (st != status_OK) {
    return st;
  }
  st = i_parser_advance(p);
  if (st != status_OK) {
    ast_free(p->mem, node);
    return st;
  }
  *out = node;
  return status_OK;
}

/* Terms = Term {[','] Term} [',']. At least one term, and the cursor must be
 * on the first. The terms are put in the group, and `commas` is set if there
 * was a comma. On failure the group has the terms that were made. */
static inline
Status i_parser_terms(i_Parser* p, i_ParserGroup* g, bool* commas) {
  bool more = true;
  while (more) {
    AST* term = NULL;
    Status st = i_parser_term(p, &term);
    if (st != status_OK) {
      return st;
    }
    st = i_parser_push(p, g, term);
    if (st != status_OK) {
      return st;
    }
    if (p->tok.kind == tokenKind_COMMA) {
      *commas = true;
      st = i_parser_advance(p);
      if (st != status_OK) {
        return st;
      }
    }
    more = i_parser_startsTerm(p->tok.kind);
  }
  return status_OK;

  /* NOTE(1): after a comma, what is not a term ends the list, which makes the
   * trailing comma valid. A second comma is then not what the caller
   * expects, and it is the caller that reports it. */
}

/* S_Expr = '(' Terms ')' and Tuple_Expr = '[' Terms ']'. The cursor is on
 * the opening token. */
static inline
Status i_parser_delimited(i_Parser* p, ASTKind kind, TokenKind close,
                          AST** out) {
  i_ParserGroup g = { NULL, 0 };
  bool commas = false;
  Position begin;
  Position end;
  Status st = i_parser_enter(p);
  if (st != status_OK) {
    return st;
  }
  begin = i_parser_tokenRange(p).begin;
  st = i_parser_advance(p);
  if (st != status_OK) {
    return st;
  }
  if (!i_parser_startsTerm(p->tok.kind)) {
    return i_parser_fail(p, status_UNEXPECTEDTOKEN, "term");
  }
  st = i_parser_terms(p, &g, &commas);
  if (st != status_OK) {
    i_parser_drop(p, g);
    return st;
  }
  if (p->tok.kind != close) {
    i_parser_drop(p, g);
    return i_parser_fail(p, status_UNEXPECTEDTOKEN,
                         close == tokenKind_RBRACKET ? "']'" : "')'");
  }
  end = i_parser_tokenRange(p).end;
  st = i_parser_advance(p);
  if (st != status_OK) {
    i_parser_drop(p, g);
    return st;
  }
  p->depth -= 1; // SAFE(1)
  return i_parser_build(p, kind,
                        kind == astKind_CALL ? astForm_SEXPR : astForm_NONE,
                        commas, g, range_new(begin, end), out);

  /* SAFE(1): `i_parser_enter` has added one, and nothing took it away. */
}

/* Factor = Atom | S_Expr | Tuple_Expr. */
static inline
Status i_parser_factor(i_Parser* p, AST** out) {
  switch (p->tok.kind) {
    case tokenKind_ID:
    case tokenKind_NUM:
    case tokenKind_STR:
      return i_parser_atom(p, out);
    case tokenKind_LPAREN:
      return i_parser_delimited(p, astKind_CALL, tokenKind_RPAREN, out);
    case tokenKind_LBRACKET:
      return i_parser_delimited(p, astKind_TUPLE, tokenKind_RBRACKET, out);
    default:
      return i_parser_fail(p, status_UNEXPECTEDTOKEN, "term");
  }
}

/* what comes after the operator `op` of a suffix. */
static inline
Status i_parser_operand(i_Parser* p, TokenKind op, AST** out) {
  if (op == tokenKind_EQUALS) {
    return i_parser_factor3(p, out);
  }
  if (op == tokenKind_COLON) {
    return i_parser_factor2(p, out);
  }
  return i_parser_factor(p, out);
}

/* The suffix whose operator `op` is under the cursor, applied to `lhs`, which
 * is taken: `(op lhs operand)`, and for a `.` all the operands of the chain,
 * `(. lhs a b c)`. The head of the call is the symbol of the (first)
 * operator. */
static inline
Status i_parser_suffix(i_Parser* p, AST* lhs, TokenKind op, AST** out) {
  i_ParserGroup g = { NULL, 0 };
  AST* head = NULL;
  bool more = true;
  Status st = i_parser_symbol(p, &head);
  if (st != status_OK) {
    ast_free(p->mem, lhs);
    return st;
  }
  st = i_parser_push(p, &g, head);
  if (st != status_OK) {
    ast_free(p->mem, lhs);
    return st;
  }
  st = i_parser_push(p, &g, lhs);
  if (st != status_OK) {
    i_parser_drop(p, g);
    return st;
  }
  while (more) {
    AST* operand = NULL;
    st = i_parser_advance(p);
    if (st == status_OK) {
      st = i_parser_operand(p, op, &operand);
    }
    if (st == status_OK) {
      st = i_parser_push(p, &g, operand);
    }
    if (st != status_OK) {
      i_parser_drop(p, g);
      return st;
    }
    more = op == tokenKind_DOT && p->tok.kind == tokenKind_DOT;
  }
  return i_parser_build(p, astKind_CALL, astForm_SUFFIX, false, g,
                        range_none(), out);
}

/* Factor_2 = Factor {'.' Factor}. */
static inline
Status i_parser_factor2(i_Parser* p, AST** out) {
  AST* f = NULL;
  Status st = i_parser_factor(p, &f);
  if (st != status_OK) {
    return st;
  }
  if (p->tok.kind == tokenKind_DOT) {
    return i_parser_suffix(p, f, tokenKind_DOT, out);
  }
  *out = f;
  return status_OK;
}

/* Factor_3 = Factor_2 [':' Factor_2]. */
static inline
Status i_parser_factor3(i_Parser* p, AST** out) {
  AST* f = NULL;
  Status st = i_parser_factor2(p, &f);
  if (st != status_OK) {
    return st;
  }
  if (p->tok.kind == tokenKind_COLON) {
    return i_parser_suffix(p, f, tokenKind_COLON, out);
  }
  *out = f;
  return status_OK;
}

/* Term = Factor_3 ['=' Factor_3]. */
static inline
Status i_parser_term(i_Parser* p, AST** out) {
  AST* f = NULL;
  Status st = i_parser_factor3(p, &f);
  if (st != status_OK) {
    return st;
  }
  if (p->tok.kind == tokenKind_EQUALS) {
    return i_parser_suffix(p, f, tokenKind_EQUALS, out);
  }
  *out = f;
  return status_OK;
}

/* I_Expr = Terms {Line_Continue} [NL >Block], with the `[';']` that ends
 * the line. The cursor is on the first token of the line, which is in the
 * column `col`; when it returns, it is on the first token of the next line
 * (or on the end of the input). `closed` tells if the line ended in `;`. */
static inline
Status i_parser_iexpr(i_Parser* p, u32 col, bool* closed, AST** out) {
  i_ParserGroup g = { NULL, 0 };
  bool commas = false;
  Status st = i_parser_terms(p, &g, &commas);
  *closed = false;
  while (st == status_OK && p->tok.kind == tokenKind_BACKSLASH) {
    st = i_parser_advance(p);
    if (st == status_OK && p->tok.kind != tokenKind_NL) {
      st = i_parser_fail(p, status_UNEXPECTEDTOKEN, "end of line");
    }
    if (st == status_OK) {
      st = i_parser_skipNewlines(p);
    }
    if (st == status_OK && !i_parser_startsTerm(p->tok.kind)) {
      st = i_parser_fail(p, status_UNEXPECTEDTOKEN, "term");
    }
    if (st == status_OK && p->tok.col <= col) {
      st = i_parser_fail(p, status_BADINDENT,
                         "a column deeper than the line it continues");
    }
    if (st == status_OK) {
      st = i_parser_terms(p, &g, &commas);
    }
  }
  if (st == status_OK && p->tok.kind == tokenKind_SEMI) {
    *closed = true;
    st = i_parser_advance(p);
  }
  if (st == status_OK && p->tok.kind != tokenKind_NL &&
      p->tok.kind != tokenKind_EOF) {
    st = i_parser_fail(p, status_UNEXPECTEDTOKEN, "end of line");
  }
  if (st == status_OK) {
    st = i_parser_skipNewlines(p);
  }
  if (st == status_OK && p->tok.kind != tokenKind_EOF && p->tok.col > col) {
    if (*closed) {
      st = i_parser_fail(p, status_BADINDENT, NULL); // NOTE(1)
    } else {
      st = i_parser_enter(p);
      if (st == status_OK) {
        st = i_parser_block(p, p->tok.col, &g);
      }
      if (st == status_OK) {
        p->depth -= 1; // SAFE(2)
      }
    }
  }
  if (st != status_OK) {
    i_parser_drop(p, g);
    return st;
  }
  if (g.n == 1) {
    *out = g.first[0];
    stackAlloc_pop(p->stack);
    return status_OK;
  }
  return i_parser_build(p, astKind_CALL, astForm_IEXPR, commas, g,
                        range_none(), out);

  /* NOTE(1): a line that ends in `;` ends its block, so there is nothing
   * for the more indented line to be a part of.
   * SAFE(2): `i_parser_enter` has added one just before the block. */
}

/* Block = {:I_Expr NL} [';'], whose expressions are in the column `col`.
 * They are put in the group `g` of the expression that opened the block,
 * after what is already there. The cursor is on the first token of the
 * first one, and when it returns it is on the first token that is not part
 * of the block (a smaller column, or the end of the input). */
static inline
Status i_parser_block(i_Parser* p, u32 col, i_ParserGroup* g) {
  bool open = true;
  while (open) {
    AST* item = NULL;
    bool closed = false;
    Status st = i_parser_iexpr(p, col, &closed, &item);
    if (st != status_OK) {
      return st;
    }
    st = i_parser_push(p, g, item);
    if (st != status_OK) {
      return st;
    }
    if (closed) {
      if (p->tok.kind != tokenKind_EOF && p->tok.col >= col) {
        return i_parser_fail(p, status_UNEXPECTEDTOKEN, "end of the block");
      }
      open = false;
    } else if (p->tok.kind == tokenKind_EOF || p->tok.col < col) {
      open = false;
    } else if (p->tok.col > col) {
      return i_parser_fail(p, status_BADDEDENT, NULL);
    }
  }
  return status_OK;

  /* NOTE(1): a column greater than `col` here is only possible after an
   * expression that had a block, which has ended in a column that is
   * between the one of that block and the one of this. The expressions
   * without a block are followed by a column that is not greater: if it were,
   * it would have been a block. */
}

/* Program = Block. */
static inline
Status i_parser_program(i_Parser* p, AST** out) {
  i_ParserGroup g = { NULL, 0 };
  Status st = i_parser_advance(p);
  if (st == status_OK) {
    st = i_parser_skipNewlines(p);
  }
  if (st != status_OK) {
    return st;
  }
  if (p->tok.kind == tokenKind_EOF) {
    return i_parser_build(p, astKind_PROGRAM, astForm_NONE, false, g,
                          i_parser_tokenRange(p), out);
  }
  st = i_parser_block(p, p->tok.col, &g);
  if (st == status_OK && p->tok.kind != tokenKind_EOF) {
    st = i_parser_fail(p, status_BADDEDENT, NULL);
  }
  if (st != status_OK) {
    i_parser_drop(p, g);
    return st;
  }
  return i_parser_build(p, astKind_PROGRAM, astForm_NONE, false, g,
                        range_none(), out);
}

/* END: internal helpers */

/* Parses the `len` bytes at `src`, which must not be NULL (even when `len` is
 * 0), and gives the program in `out`: a `PROGRAM` that has to be freed with
 * `ast_free` and the same `mem`. Everything the tree owns (nodes, lexemes,
 * strings, numbers) comes from `mem`, so the source may be freed after the
 * parse. The identifiers are interned in `sp`, which is the caller's, and its
 * symbols stay there, even if the parse fails.
 *
 * `maxDepth` is how many brackets, parentheses and indented blocks may be
 * open one inside the other (the top level does not count): the parser
 * recurs as deep as that, so it is a `status_MAXDEPTH` and not a stack
 * overflow. `parser_DEFAULTMAXDEPTH` is a good one. The tree is as deep as
 * three times that, and the functions that recur on it (`ast_free`,
 * `ast_snprint`) go as far.
 *
 * It returns `status_OK`, and then `err` is left alone, or the status that
 * is in `err->code`, and then `out` is left alone and nothing that was made
 * stays allocated, but for the symbols. See the top of the file for the
 * errors.
 *
 * The parser keeps the nodes that it has made and not yet put in a parent in
 * a stack of its own, allocated from `mem` for the time of the parse, with
 * room for one for each byte of the source. */
static inline
Status parser_parse(const IAllocator* mem, SymbolSpace* sp, const u8* src,
                    usize len, usize maxDepth, AST** out, ParseError* err) {
  #if config_DEBUG
    if (mem == NULL || sp == NULL || src == NULL || out == NULL ||
        err == NULL) {
      debug_FATAL("Argument is null.");
    }
  #endif
  i_Parser p;
  AST* tree = NULL;
  u8* buffer;
  usize chunks;
  usize size;
  Status st;

  p.err.code = status_OK;
  p.err.range = range_none();
  p.err.expected = NULL;
  p.tok.kind = tokenKind_EOF;
  p.tok.start = 0;
  p.tok.end = 0;
  p.tok.line = 0;
  p.tok.col = 0;
  st = lexer_new(src, len, &p.lexer);
  if (st != status_OK) {
    err->code = st;
    err->range = range_none();
    err->expected = NULL;
    return st;
  }

  if (len >= (SIZE_MAX - sizeof(StackAlloc) - WORD) / sizeof(AST*)) {
    err->code = status_OUTOFMEMORY;
    err->range = range_none();
    err->expected = NULL;
    return status_OUTOFMEMORY;
  }
  chunks = len + 1; // SAFE(1)
  size = sizeof(StackAlloc) + WORD + chunks * sizeof(AST*);
  buffer = (u8*)mem->alloc(mem->heap, size, __func__);
  if (buffer == NULL) {
    err->code = status_OUTOFMEMORY;
    err->range = range_none();
    err->expected = NULL;
    return status_OUTOFMEMORY;
  }
  p.stack = stackAlloc_create(buffer, size, sizeof(AST*));
  if (p.stack == NULL) { // NOTE(2)
    (void)mem->free(mem->heap, buffer);
    err->code = status_OUTOFMEMORY;
    err->range = range_none();
    err->expected = NULL;
    return status_OUTOFMEMORY;
  }

  p.mem = mem;
  p.sp = sp;
  p.src = src;
  p.srcLen = len;
  p.depth = 0;
  p.maxDepth = maxDepth;

  st = i_parser_program(&p, &tree);
  #if config_DEBUG
    if (!stackAlloc_empty(p.stack)) {
      debug_FATAL("The work stack of the parser is not empty.");
    }
  #endif
  (void)mem->free(mem->heap, buffer);
  if (st != status_OK) {
    *err = p.err;
    return st;
  }
  *out = tree;
  return status_OK;

  /* SAFE(1): `len` is less than the biggest number of chunks that fit in a
   * `usize` along with the header, so neither `len + 1` nor the size wrap
   * around.
   * NOTE(2): the buffer has the header, the bytes lost to align it, and at
   * least one chunk, so it is not too small. */
}

#endif
