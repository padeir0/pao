/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_LIST_H
#define PAO_LIST_H

#include "basicTypes.h"
#include "str.h"
#include "symbol.h"
#include "numbers/number.h"

typedef byte Nil;

typedef enum {
  atomKind_NULL,
  atomKind_BOOL,
  atomKind_NUMBER,
  atomKind_STRING,
  atomKind_LIST,
  atomKind_FUNCTION,
  atomKind_SYMBOL,
  atomKind_FORM,
  // atomKind_FILE,
} AtomKind;

typedef struct i_List List;

typedef union i_AtomValue {
  Nil Nil;
  bool Bool;
  List* List;
  Str   Str;
  Number Number;
  Symbol Symbol;
  // Form Form; // (if, let, etc...)
  // Function Function; // funções puras, closures
  // File file; // algum dia penso nisso
} AtomValue;

typedef struct {
  AtomKind kind;
  AtomValue value;
} Atom;

struct i_List {
  Atom value;
  struct i_List* next;
};

#endif
