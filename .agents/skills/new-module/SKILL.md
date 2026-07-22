---
name: new-module
description: Step-by-step template and checklist for adding a new library module or allocator to Pão, following all established conventions.
---

# Adding a New Module to Pão

Use this when creating a new `.h` file in `lib/` or a new test file in `tests/`.

---

## Library Header Template

Every new module starts from `lib/blank.h`. Here is the full template:

```c
/*
MIT License
Copyright 2025 Artur Iure Vianna Fernandes
See the LICENSE file for more information.
*/

#ifndef PAO_MYMODULE_H
#define PAO_MYMODULE_H

#include "basicTypes.h"
#include "status.h"
#include "config.h"

#if config_DEBUG
  #include "debug.h"
#endif

/* Brief description of this module.
*/
typedef struct {
  /* fields in camelCase */
} MyModule;

/* Brief docstring: what the function does, what it returns.
*/
static inline
Status myModule_doSomething(IAllocator* mem, const MyModule* input, MyModule* out) {
  #if config_DEBUG
    if (mem == NULL || input == NULL || out == NULL) {
      debug_FATALFMT("Some pointer parameter is null. mem = %p, input = %p, out = %p.",
                     (void*)mem, (void*)input, (void*)out);
    }
  #endif

  Status st;
  /* implementation using while loops only */
  /* use // NOTE(1), // SAFE(1) markers inline */

  return status_OK;
  /* NOTE(1): explanation here at the bottom.
     SAFE(1): proof of safety here.
  */
}

#endif
```

## Checklist — New Library Module

1. **File**: Create `lib/<moduleName>.h` (or `lib/<subdir>/<moduleName>.h`)
2. **License block**: MIT header at top, exactly as in template
3. **Header guard**: `PAO_<MODULENAMEINCAPS>_H`
4. **Includes**: Only standard library + other pão headers. Include `debug.h` conditionally under `config_DEBUG`
5. **All functions**: Must be `static inline`
6. **Naming**:
   - Public functions: `moduleName_camelCase`
   - Internal functions: `i_moduleName_camelCase`
   - Public types: `ModuleName` (UpperCamelCase, typedef'd)
   - Internal types: `i_ModuleName`
   - Public macros/constants: `moduleName_CAPS`
   - Internal macros/constants: `i_moduleName_CAPS`
7. **Function signatures**:
   - `void` for empty parameter lists
   - `const` on read-only pointer params
   - Out-parameters last
   - Allocator as first param when allocation needed
8. **Error handling**:
   - Return `Status` for recoverable errors
   - Use `debug_FATAL`/`debug_FATALFMT` for bad usage (gated by `config_DEBUG`)
   - Propagate with `Status st; st = fn(...); status_CHECK;`
9. **Comments**:
   - Docstrings above functions: short, descriptive
   - Footnotes before closing `}` for complex reasoning
   - Use `NOTE(n)`, `SAFE(n)`, `UNSAFE(n)`, `DRAGON`, `TODO`, `UNTESTED`
10. **Control flow**: `while` loops only. No `for`, `goto`, `continue`
11. **No globals**: Functions that need memory take `IAllocator*`

## Checklist — New Allocator Implementation

When adding a new allocator (like the planned stack allocator):

1. Create `lib/alloc/<allocName>.h`
2. Define the allocator's state struct (e.g., `StackAlloc`)
3. Implement the concrete API:
   - `<allocName>_create(...)` or `<allocName>_new(...)` — initialize from buffer
   - `<allocName>_alloc(...)` — allocate
   - `<allocName>_free(...)` — free (or crash if not supported)
   - `<allocName>_freeAll(...)` — bulk free
   - `<allocName>_available(...)`, `<allocName>_used(...)`, `<allocName>_total(...)`, `<allocName>_empty(...)`
4. Implement the `IAllocator` adapter functions:
   - `i_<allocName>_alloc(void* heap, usize size, const char* func)`
   - `i_<allocName>_free(void* heap, void* obj)`
   - `i_<allocName>_freeAll(void* heap)`
   - `i_<allocName>_info(void* heap)`
5. Provide `<allocName>_createInterface(<AllocType>* alloc)` returning `IAllocator`
6. Add debug checks: NULL, out-of-bounds, double-free detection (gated by `config_DEBUG`)

## Checklist — New Number Type

When extending the number tower (like completing Rationals or adding Decimals):

1. Define the struct wrapping `Natural` (and possibly `Integer`)
2. Implement: `new`, `create`, `free`, `set`, `setVec`, `copy`, `isZero`, `equal`, `compare`, `snprint`
3. Implement arithmetic: `add`, `sub`, `mult`, `div` — delegating to the underlying Natural operations with sign/denominator tracking
4. All arithmetic functions take `IAllocator*` and return `Status`
5. The `out` parameter must not alias `A` or `B` for multiplication/division (enforce in debug mode)
6. Addition/subtraction may allow aliasing between inputs and output (document this)

## Checklist — New Test File

1. **File**: `tests/<path>/<moduleName>_test.c`
2. **Includes**: `tests/common.h` (provides `Tester`, `run_tests`, `checkStatus`, allocator helpers)
3. **Structure**:
   ```c
   #include "../common.h"       // or appropriate relative path
   #include "../../lib/myModule.h"

   bool test_something(void) {
     /* setup, call, verify, return true/false */
     return true;
   }

   int main(void) {
     Tester tests[] = {
       {"test_something", test_something},
     };
     int len = (int)(sizeof(tests) / sizeof(tests[0]));
     run_tests("myModule", tests, len);
     return 0;
   }
   ```
4. **Crash tests**: For debug assertions, create `pao_<module>_<scenario>_crash.c` that triggers the assertion. The `test` script verifies these exit with `SIGABRT` (status 134)
5. **Verify memory**: For modules using allocators, use `info().used` to check for leaks (e.g., `isAllFree()` patterns)
6. **OOM testing**: Use `i_FailAllocHeap` from `common.h` to simulate allocation failures
7. **Only test public API**: Never call `i_` internal functions from tests
8. **Before committing**: run `tests/testp` to verify across gcc/clang × c99/c17 × -O0/-O2
