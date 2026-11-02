/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_SYMBOL_H
#define PAO_SYMBOL_H

#include <string.h>
#include "basicTypes.h"
#include "status.h"
#include "config.h"
#include "str.h"
#include "iallocator.h"

#if config_DEBUG
  #include "debug.h"
#endif

/* A symbol is a string interned in a `SymbolSpace`: the same string always
 * gives the same symbol, so symbols are compared as integers. Symbols are
 * numbered from 0, in the order they were interned. */
typedef i64 Symbol;

typedef struct i_sylist {
  Symbol id;
  struct i_sylist* next;
} SymbolList;

/* The symbols have an allocator of their own: the symbol space only grows
 * while the program is being read, and it never shrinks. All memory
 * (names, array of names, buckets and nodes) comes from `alloc`, which
 * needs to have `free`, or the old arrays leak when the space grows.
 * That is harmless for an arena. */
typedef struct {
  SymbolList** buckets;
  usize bucketLength;

  Str* symbols;
  /* how many symbols fit in `symbols` */
  usize symbolsLength;
  /* how many symbols exist, and the id of the next one */
  i64 nextSymbol;

  IAllocator alloc;
} SymbolSpace;

/* true if `count` elements of `size` bytes do not overflow a `usize`. */
static inline
bool i_symbolSpace_fits(usize count, usize size) {
  return count <= SIZE_MAX / size;
}

static inline
usize i_symbolSpace_index(const Str* s, usize bucketLength) {
  return (usize)str_hash(s) % bucketLength;
}

/* Allocates `capacity` names and as many buckets, both empty. On failure
 * it leaves nothing allocated. */
static inline
Status i_symbolSpace_alloc(const IAllocator* mem, usize capacity,
                           Str** symbols, SymbolList*** buckets) {
  usize i = 0;
  if (!i_symbolSpace_fits(capacity, sizeof(Str)) ||
      !i_symbolSpace_fits(capacity, sizeof(SymbolList*))) {
    return status_OUTOFMEMORY;
  }
  *symbols = (Str*)mem->alloc(mem->heap, capacity * sizeof(Str), __func__);
  if (*symbols == NULL) {
    return status_OUTOFMEMORY;
  }
  *buckets = (SymbolList**)mem->alloc(mem->heap, capacity * sizeof(SymbolList*), __func__);
  if (*buckets == NULL) {
    (void)mem->free(mem->heap, *symbols);
    return status_OUTOFMEMORY;
  }
  while (i < capacity) {
    (*buckets)[i] = NULL;
    i++;
  }
  return status_OK;
}

/* Doubles the capacity. The names and the nodes are not copied, only
 * rehashed into the new buckets. If it fails, nothing changed. */
static inline
Status i_symbolSpace_grow(SymbolSpace* sp) {
  Str* symbols;
  SymbolList** buckets;
  SymbolList* node;
  SymbolList* next;
  usize newLength;
  usize i = 0;
  usize index;
  Status st;

  if (sp->symbolsLength > SIZE_MAX / 2) {
    return status_OUTOFMEMORY;
  }
  newLength = sp->symbolsLength * 2;
  st = i_symbolSpace_alloc(&sp->alloc, newLength, &symbols, &buckets);
  if (st != status_OK) {
    return st;
  }

  memcpy(symbols, sp->symbols, (usize)sp->nextSymbol * sizeof(Str));
  while (i < sp->bucketLength) {
    node = sp->buckets[i];
    while (node != NULL) {
      next = node->next;
      index = i_symbolSpace_index(&symbols[node->id], newLength);
      node->next = buckets[index];
      buckets[index] = node;
      node = next;
    }
    i++;
  }

  // NOTE(1)
  (void)sp->alloc.free(sp->alloc.heap, sp->symbols);
  (void)sp->alloc.free(sp->alloc.heap, sp->buckets);
  sp->symbols = symbols;
  sp->symbolsLength = newLength;
  sp->buckets = buckets;
  sp->bucketLength = newLength;
  return status_OK;

  /* NOTE(1): the status of `free` is ignored on purpose. An allocator that
     cannot free (an arena) returns an error, and leaking the old arrays
     there is what is expected.
  */
}

/* Creates an empty symbol space, with room for `capacity` symbols (which
 * must not be 0). It grows as needed. `mem` is copied, and everything the
 * space allocates comes from it. Returns status_OUTOFMEMORY if it fails. */
static inline
Status symbolSpace_new(const IAllocator* mem, usize capacity, SymbolSpace* out) {
  #if config_DEBUG
    if (mem == NULL || out == NULL) {
      debug_FATAL("Argument is null.");
    }
    if (capacity == 0) {
      debug_FATAL("Capacity of a symbol space cannot be 0.");
    }
  #endif
  Str* symbols;
  SymbolList** buckets;
  Status st = i_symbolSpace_alloc(mem, capacity, &symbols, &buckets);
  if (st != status_OK) {
    return st;
  }
  out->symbols = symbols;
  out->symbolsLength = capacity;
  out->buckets = buckets;
  out->bucketLength = capacity;
  out->nextSymbol = 0;
  out->alloc = *mem;
  return status_OK;
}

/* Returns the symbol of the string `s` in `out`, which is the same symbol
 * every time. The first time, the string is copied. If this fails, it
 * returns status_OUTOFMEMORY and the space is as it was before. */
static inline
Status symbolSpace_intern(SymbolSpace* sp, const Str* s, Symbol* out) {
  #if config_DEBUG
    if (sp == NULL || s == NULL || out == NULL) {
      debug_FATAL("Argument is null.");
    }
  #endif
  SymbolList* node = sp->buckets[i_symbolSpace_index(s, sp->bucketLength)];
  while (node != NULL) {
    if (str_equals(&sp->symbols[node->id], s)) {
      *out = node->id;
      return status_OK;
    }
    node = node->next;
  }

  Status st;
  Str copy;
  usize index;
  if ((usize)sp->nextSymbol == sp->symbolsLength) {
    st = i_symbolSpace_grow(sp);
    if (st != status_OK) {
      return st;
    }
  }
  st = str_copy(&sp->alloc, s, &copy);
  if (st != status_OK) {
    return st;
  }
  node = (SymbolList*)sp->alloc.alloc(sp->alloc.heap, sizeof(SymbolList), __func__);
  if (node == NULL) {
    (void)str_free(&sp->alloc, copy);
    return status_OUTOFMEMORY;
  }

  index = i_symbolSpace_index(s, sp->bucketLength);
  node->id = sp->nextSymbol;
  node->next = sp->buckets[index];
  sp->buckets[index] = node;
  sp->symbols[sp->nextSymbol] = copy;
  sp->nextSymbol++;
  *out = node->id;
  return status_OK; // NOTE(1)

  /* NOTE(1): the space only changes after everything that can fail has
     happened. Growing first is no problem, since a bigger space is
     the same space.
  */
}

/* The string of a symbol, which is owned by the space and lives as long as it
 * does. The symbol must have been made by `symbolSpace_intern`. */
static inline
Str symbolSpace_findSymbol(const SymbolSpace* sp, Symbol id) {
  #if config_DEBUG
    if (sp == NULL) {
      debug_FATAL("Symbol space is null.");
    }
    if (id < 0 || id >= sp->nextSymbol) {
      debug_FATALFMT("Symbol %lld not within bounds", (long long)id);
    }
  #endif
  return sp->symbols[id];
}

/* Frees everything in the space, and leaves it empty and unusable. */
static inline
void symbolSpace_free(SymbolSpace* sp) {
  #if config_DEBUG
    if (sp == NULL) {
      debug_FATAL("Symbol space is null.");
    }
  #endif
  SymbolList* node;
  SymbolList* next;
  usize i = 0;
  while (i < sp->bucketLength) {
    node = sp->buckets[i];
    while (node != NULL) {
      next = node->next;
      (void)str_free(&sp->alloc, sp->symbols[node->id]);
      (void)sp->alloc.free(sp->alloc.heap, node);
      node = next;
    }
    i++;
  }
  (void)sp->alloc.free(sp->alloc.heap, sp->symbols); // NOTE(1)
  (void)sp->alloc.free(sp->alloc.heap, sp->buckets);
  sp->symbols = NULL;
  sp->symbolsLength = 0;
  sp->buckets = NULL;
  sp->bucketLength = 0;
  sp->nextSymbol = 0;

  /* NOTE(1): the statuses of `free` are ignored: an arena cannot free, and
     its memory is all released at once by whoever owns it.
  */
}

#endif
