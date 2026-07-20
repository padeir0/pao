/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_LIST_H
#define PAO_LIST_H

#include "basicTypes.h"

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
} AtomKind;


typedef struct i_List List;

typedef union i_AtomValue {
  Nil Nil;
  bool Bool;
  List* List;
  // String String;
  // Number Number;
  // Symbol Symbol;
  // Form Form;
  // Function Function;
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
