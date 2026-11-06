/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

/* ovo: the Ovo interpreter.
 *
 * usage:
 *   ovo lex <file>    prints one token per line, as
 *                       line:col KIND [start,end) "text"
 *                     (line and col start at 1). On a lexical error, it
 *                     prints `file:line:col: reason` to stderr instead and
 *                     exits with 1.
 *   ovo parse <file>  prints the tree of the program, one node per line (see
 *                     `ast_snprint`, which writes it). On an error in the
 *                     source (lexical, syntactic, or a number or a string
 *                     that cannot be converted), it prints
 *                       file:line:col: reason[, expected what]
 *                     to stderr instead and exits with 1. The same source
 *                     always gives the same text.
 *
 * exit codes: 0 ok, 1 error in the source, 2 bad usage or unreadable file.
 *
 * build, from the root of the repository:
 *   gcc -Wall -Wextra -Werror -pedantic -Wconversion -Wstrict-prototypes \
 *       -std=c99 -O2 cmd/ovo.c -o ovo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../lib/lexer.h"
#include "../lib/parser.h"
#include "../lib/alloc/stdAlloc.h"

#define ovo_OK 0
#define ovo_SOURCEERROR 1
#define ovo_USAGEERROR 2

static
const char* ovo_reason(Status st) {
  switch (st) {
    case status_INVALIDUTF8:        return "invalid UTF-8";
    case status_BADCHAR:            return "unexpected character";
    case status_BADESCAPE:          return "unknown escape sequence";
    case status_UNTERMINATEDSTRING: return "unterminated string";
    case status_BADNUMBER:          return "malformed number";
    case status_DIVISIONBYZERO:     return "division by zero in a fraction";
    case status_UNSUPPORTEDNUMBER:  return "number not supported yet";
    case status_UNEXPECTEDTOKEN:    return "unexpected token";
    case status_BADINDENT:          return "unexpected indentation";
    case status_BADDEDENT:          return "indentation does not match an open block";
    case status_MAXDEPTH:           return "nested too deeply";
    default:                        return "lexical error";
  }
}

/* reads the whole file into a new buffer. The buffer has at least one byte,
 * even if the file is empty, so it is never NULL. */
static
bool ovo_readFile(const char* path, u8** out, usize* len) {
  FILE* f = fopen(path, "rb");
  long size;
  u8* buffer;
  usize read;
  if (f == NULL) {
    return false;
  }
  if (fseek(f, 0, SEEK_END) != 0 || (size = ftell(f)) < 0 ||
      fseek(f, 0, SEEK_SET) != 0) {
    fclose(f);
    return false;
  }
  buffer = (u8*)malloc((usize)size + 1);
  if (buffer == NULL) {
    fclose(f);
    return false;
  }
  read = fread(buffer, 1, (usize)size, f);
  fclose(f);
  if (read != (usize)size) {
    free(buffer);
    return false;
  }
  *out = buffer;
  *len = read;
  return true;
}

static
void ovo_printToken(const u8* src, Token t) {
  printf("%u:%u %s [%u,%u) \"", t.line + 1, t.col + 1,
         tokenKind_name(t.kind), t.start, t.end);
  if (t.kind == tokenKind_NL) {
    printf("\\n");
  } else {
    fwrite(src + t.start, 1, (usize)(t.end - t.start), stdout);
  }
  printf("\"\n");
}

static
int ovo_lex(const char* path) {
  u8* src = NULL;
  usize len = 0;
  Lexer l;
  Token t;
  Status st;

  if (!ovo_readFile(path, &src, &len)) {
    fprintf(stderr, "ovo: cannot read '%s'\n", path);
    return ovo_USAGEERROR;
  }
  st = lexer_new(src, len, &l);
  if (st != status_OK) {
    fprintf(stderr, "ovo: '%s' is too big\n", path);
    free(src);
    return ovo_USAGEERROR;
  }

  st = lexer_next(&l, &t);
  while (st == status_OK) {
    ovo_printToken(src, t);
    if (t.kind == tokenKind_EOF) {
      free(src);
      return ovo_OK;
    }
    st = lexer_next(&l, &t);
  }
  fprintf(stderr, "%s:%u:%u: %s\n", path, t.line + 1, t.col + 1,
          ovo_reason(st));
  free(src);
  return ovo_SOURCEERROR;
}

/* writes the dump of the tree to stdout. `ast_snprint` writes everything or
 * nothing, so the buffer grows until the dump fits. Returns false if there is
 * no memory for it. */
static
bool ovo_dump(const AST* tree, const SymbolSpace* sp) {
  usize size = 4096;
  while (size <= SIZE_MAX / 2) {
    char* buffer = (char*)malloc(size);
    usize n;
    if (buffer == NULL) {
      return false;
    }
    n = ast_snprint(tree, sp, buffer, size);
    if (n > 0) {
      fwrite(buffer, 1, n, stdout);
      free(buffer);
      return true;
    }
    free(buffer);
    size *= 2;
  }
  return false;
}

static
int ovo_parse(const char* path) {
  u8* src = NULL;
  usize len = 0;
  IAllocator mem = stdAlloc_new();
  SymbolSpace sp;
  AST* tree = NULL;
  ParseError err;
  Status st;
  int code = ovo_OK;

  if (!ovo_readFile(path, &src, &len)) {
    fprintf(stderr, "ovo: cannot read '%s'\n", path);
    return ovo_USAGEERROR;
  }
  st = symbolSpace_new(&mem, 64, &sp);
  if (st != status_OK) {
    fprintf(stderr, "ovo: out of memory\n");
    free(src);
    return ovo_USAGEERROR;
  }

  st = parser_parse(&mem, &sp, src, len, parser_DEFAULTMAXDEPTH, &tree, &err);
  if (st == status_OK) {
    if (!ovo_dump(tree, &sp)) {
      fprintf(stderr, "ovo: out of memory\n");
      code = ovo_USAGEERROR;
    }
    ast_free(&mem, tree);
  } else if (st == status_BADSIZE) {
    fprintf(stderr, "ovo: '%s' is too big\n", path);
    code = ovo_USAGEERROR;
  } else if (st == status_OUTOFMEMORY) {
    fprintf(stderr, "ovo: out of memory\n");
    code = ovo_USAGEERROR;
  } else {
    fprintf(stderr, "%s:%llu:%llu: %s", path,
            (unsigned long long)err.range.begin.line + 1ULL,
            (unsigned long long)err.range.begin.col + 1ULL,
            ovo_reason(st));
    if (err.expected != NULL) {
      fprintf(stderr, ", expected %s", err.expected);
    }
    fprintf(stderr, "\n");
    code = ovo_SOURCEERROR;
  }
  symbolSpace_free(&sp);
  free(src);
  return code;
}

int main(int argc, char** argv) {
  if (argc == 3 && strcmp(argv[1], "lex") == 0) {
    return ovo_lex(argv[2]);
  }
  if (argc == 3 && strcmp(argv[1], "parse") == 0) {
    return ovo_parse(argv[2]);
  }
  fprintf(stderr, "usage: ovo lex <file>\n       ovo parse <file>\n");
  return ovo_USAGEERROR;
}
