/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_SYMBOL_H
#define PAO_SYMBOL_H

#include "basicTypes.h"
#include "stdio.h"
#include "str.h"
#include "iallocator.h"

typedef i64 Symbol;

typedef struct i_sylist {
  Symbol id;
  struct i_sylist* next;
} SymbolList;

typedef struct {
  SymbolList** buckets;
  usize bucketLength;

  Str* symbols;
  usize symbolsLength;
  i64 nextSymbol;

  IAllocator alloc;
} SymbolSpace;

// TODO: symbolspace_new (to create a properly initialized symbol space)

static inline
Symbol symbolspace_createSymbol(SymbolSpace* sp, Str* s) {
  u32 index = str_hash(s) % sp->bucketLength;
  SymbolList** new;
  if (sp->buckets[index] == NULL) {
    new = &sp->buckets[index];
  } else {
    SymbolList* sl = sp->buckets[index];
    while (sl->next != NULL) {
      sl = sl->next;
    }
    new = &sl->next;
  }

  Symbol id = (Symbol)sp->nextSymbol;
  sp->symbols[id] = str_copy(sp->alloc, s);

  SymbolList* sl = (SymbolList*) sp->alloc.alloc(sp->alloc.heap, sizeof(SymbolList), __FUNCTION__);
  sl->id = id;
  sl->next = NULL;
  *new = sl;

  sp->nextSymbol++;
  return id;
}

static inline
Str symbolspace_findSymbol(SymbolSpace* sp, Symbol id) {
  #if DEBUG
    if ((usize)id > sp->symbolsLength || id < 0) {
      debug_FATALFMT("Symbol %d not whithin bounds", id);
    }
  #endif
  return sp->symbols[id];
}

#endif
