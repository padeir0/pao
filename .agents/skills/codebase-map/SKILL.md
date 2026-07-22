---
name: codebase-map
description: Architecture reference map of the Pão repository — module inventory, dependency graph, core abstractions, and future plans.
---

# Pão Codebase Map

## Repository Structure

```
pao/
├── lib/                        # Header-only library (all .h files)
│   ├── basicTypes.h            # Integer typedefs: u8–u64, i8–i64, usize, uptr, byte, WORD
│   ├── config.h                # config_DEBUG toggle (currently 1)
│   ├── status.h                # Status enum + status_CHECK macro
│   ├── debug.h                 # debug_PRINT, debug_FATAL macros (stderr + abort)
│   ├── order.h                 # Order enum (LESS/EQUAL/GREATER) + order_invert
│   ├── util.h                  # minU32, maxU32, absI32, absI64, distanceU8Ptr
│   ├── colors.h                # ANSI escape code constants
│   ├── iallocator.h            # IAllocator vtable interface (alloc/free/freeAll/info)
│   ├── buffer.h                # Ring-buffer (ptr/start/len/cap) for read/write I/O
│   ├── utf8.h                  # UTF-8 decoder (RFC 3629 conformant)
│   ├── list.h                  # Atom/AtomKind/AtomValue/List — Lisp-like data (WIP for Ovo)
│   ├── blank.h                 # Template file for new modules
│   ├── alloc/                  # Allocator implementations
│   │   ├── linearAlloc.h       # Arena/linear allocator (no individual free)
│   │   ├── pool.h              # Fixed-size pool allocator
│   │   ├── flAlloc.h           # Free-list allocator (with coalescing)
│   │   ├── stdAlloc.h          # stdlib malloc/free wrapper
│   │   └── nullAlloc.h         # Always-NULL allocator (for static/embedded use)
│   └── numbers/                # Arbitrary-precision arithmetic
│       ├── natural.h           # Natural numbers (base-10⁹, u32 digits, ~1000 lines)
│       ├── integer.h           # Integers (Natural + sign)
│       └── rational.h          # Rationals (numerator/denominator Naturals) — partial
├── tests/                      # Test suite
│   ├── common.h                # Test harness: Tester struct, run_tests(), FailAllocHeap
│   ├── test                    # Bash script: runs *_test.c and *_crash.c (gcc, c99, -O2)
│   ├── testp                   # Portable test script: gcc/clang × c99/c17 × -O0/-O2
│   ├── .clangd                 # IDE diagnostic config for tests
│   ├── buffer_test.c           # Buffer tests
│   ├── utf8_test.c             # UTF-8 decoder tests (based on Markus Kuhn stress test)
│   ├── allocators/
│   │   ├── linearAlloc_test.c
│   │   ├── flAlloc/
│   │   │   ├── flAlloc_test.c
│   │   │   ├── pao_flAlloc_doubleFree_crash.c
│   │   │   └── pao_flAlloc_outOfBounds_crash.c
│   │   └── pool/
│   │       ├── pool_test.c
│   │       ├── pao_pool_badAlignment_crash.c
│   │       ├── pao_pool_doubleFree_crash.c
│   │       └── pao_pool_outOfBounds_crash.c
│   └── numbers/
│       ├── integer_test.c
│       ├── rational_test.c
│       └── natural/
│           ├── natural_test.c  # ~2,300 lines — the largest test file
│           ├── pao_natural_aliasing_crash.c
│           ├── pao_natural_invalidDigit_crash.c
│           └── pao_natural_nullPointer_crash.c
├── docs/
│   ├── styleguide.md           # C coding conventions (authoritative)
│   ├── plans.md                # Roadmap and TODO checklist
│   ├── language/
│   │   └── language-spec.md    # Ovo programming language specification
│   └── dama/
│       ├── dama.md             # Checkers engine design notes (Brazilian draughts)
│       └── exemplo.pdn         # Example PDN game file
├── readme.md
├── LICENSE                     # MIT
└── .gitignore
```

## Module Dependency Graph

```
basicTypes.h ◄── status.h
                    ▲
basicTypes.h ◄── util.h
                    ▲
basicTypes.h ◄── order.h
                    ▲
              ┌─ debug.h (stdio)
              │
basicTypes.h ─┤
 status.h ────┤── iallocator.h
              │
              ├── buffer.h (stdio, string, limits)
              │
 status.h ────┼── utf8.h (config.h, debug.h when DEBUG)
 config.h ────┘
              
iallocator.h ─┬── linearAlloc.h (stdio)
               ├── pool.h (config.h, debug.h, util.h, string)
               ├── flAlloc.h (stdio, config.h, debug.h, util.h)
               ├── stdAlloc.h (stdio, stdlib)
               └── nullAlloc.h (stdio)

basicTypes.h ──┬
status.h ──────┤
iallocator.h ──┤── natural.h (string, config.h, debug.h, util.h, order.h)
util.h ────────┤          ▲
order.h ───────┤          │
config.h ──────┘          │
                    integer.h (natural.h)
                          ▲
                          │
                    rational.h (natural.h)

basicTypes.h ──── list.h (Atom, AtomKind, AtomValue, List)
```

## Core Abstractions

### IAllocator — Memory Management Interface

Defined in `iallocator.h`. A vtable of 4 function pointers:
- `IAlloc alloc(void* heap, usize size, const char* func)` — allocate, `func` for debug provenance
- `IFree free(void* heap, void* obj)` — free single object
- `IFreeAll freeAll(void* heap)` — free entire heap
- `IInfo info(void* heap)` — returns `AllocatorInfo { used, total }`

Five implementations exist, each providing a `*_createInterface()` function that returns an `IAllocator`:
1. **linearAlloc** — arena, bulk-free only, no alignment (TODO)
2. **pool** — fixed-size chunks, O(n) double-free detection in debug
3. **flAlloc** — variable-size with coalescing (prepend/append/join), word-aligned
4. **stdAlloc** — `malloc`/`free` wrapper, no heap tracking
5. **nullAlloc** — always returns NULL, for embedded/static-only usage

### Status — Error Propagation

Enum in `status.h`: `status_OK`, `status_DIVISIONBYZERO`, `status_OUTOFMEMORY`, `status_NATURALNUMBEROVERFLOW`, `status_BUFFERTOOSMALL`, `status_INVALIDUTF8`, `status_EOF`, `status_OUTOFBOUNDS`, `status_BADALIGNMENT`, `status_BADSIZE`, `status_NULLBUFFER`, `status_FAILEDFREE`.

### Buffer — Ring Buffer

Struct `{ byte* ptr, usize start, usize len, usize cap }`. Read from `start`, write at `start+len`. Supports: `writeByte`, `readByte`, `writeLiteral`, `writeString`, `toHex`, `equals`, `printStr`, `bzero`, `reset`.

### Natural / Integer / Rational — Number Tower

- **Natural**: base-10⁹ digits stored as `u32` array (LSD first), with `i64` intermediates for safe multiplication. Supports: add, addDigit, mult, multDigit, multBase, distance, distanceDigit, div, divDigit, compare, copy, set, snprint. Division uses binary search for each quotient digit.
- **Integer**: `Natural abs` + `i8 sign`. Delegates arithmetic to Natural with sign tracking.
- **Rational**: `Natural numerator` + `Natural denominator` + `i8 sign`. Partially implemented (equality via cross-multiplication, printing). Missing: add, sub, mul, div, simplify, normalize, compare.

### Test Infrastructure

`tests/common.h` provides:
- `Tester` struct: `{ char* name, bool (*func)(void) }`
- `run_tests(name, tests, length)`: runs all testers, prints pass/fail summary
- `checkStatus(Status)`: aborts on non-OK
- `i_FailAllocHeap`: wraps a `FLAlloc` with a countdown that starts returning NULL after N successful allocations — used for OOM simulation testing
- `print_buff[DEFAULT_SIZE]`: shared buffer for test output

## Future Plans

### Ovo Language (see `docs/language/language-spec.md` and `docs/plans.md`)
A dynamically-typed functional programming language with indentation-sensitive syntax derived from S-expressions. Planned implementation order: number tower → immutable strings → dictionary → symbol interning → lexer → parser → evaluator → built-ins.

### Dama / Checkers Engine (see `docs/dama/dama.md`)
A Brazilian draughts engine targeting: move generation (verified against OEIS A133046 perft), alpha-beta search, a web UI via Emscripten, and Lidraughts bot integration.

### Library Roadmap (from `docs/plans.md`)
- Stack allocator
- Rationals (full arithmetic)
- Decimals (fixed point)
- UTF-8 encoder
- Command-line argument parsing
