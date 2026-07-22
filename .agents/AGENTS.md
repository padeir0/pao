# Pão — Agent Rules

This is a **header-only C99 utility library** under the MIT license.
All library code lives in `lib/`, all tests in `tests/`.
The author is Artur Iure Vianna Fernandes.

See also: [styleguide.md](../docs/styleguide.md), [plans.md](../docs/plans.md).

---

## Hard Rules — Never Violate

### Language & Standard

- **C99**. No GNU extensions in library code (except `__attribute__((unused))`).
- **Header-only**: every library module is a single `.h` file.
- **All library functions must be `static inline`**. No exceptions.
- Only depend on the C standard library.
- No C++ compatibility.

### Naming Conventions

Underscores `_` are **namespace separators only**. Snake_case is forbidden.

| Element | Public | Internal (private) |
|---|---|---|
| Functions | `namespace_camelCase` | `i_namespace_camelCase` |
| Macros / Constants / Enums | `namespace_CAPS` | `i_namespace_CAPS` |
| Types (typedef'd) | `NameUpperCamelCase` | `i_NameUpperCamelCase` |
| Global variables (tests only) | `g_namespace_name` | — |
| Header guards | `PAO_NAMEINCAPS_H` | — |

- Local variables, arguments, struct/union fields: plain `camelCase`, no prefix.
- Exception: types in `basicTypes.h` use `lowerCamelCase` (e.g. `u32`, `usize`).
- Type names often define function namespaces: type `Natural` → functions `natural_add`, `natural_set`, etc.

### Control Flow

- **Only `while` loops**. Never use `for`, `goto`, or `continue`.
- Never modify function arguments. Never take their address.
- Avoid taking the address of local variables.

### Function Signatures

- Functions with no arguments must use explicit `void`: `fn(void)`.
- Out-parameters must be the **last** parameters.
- Read-only pointer parameters must be marked `const`.

### Error Handling

- **Bad usage / internal errors**: crash via `debug_FATAL()` / `debug_FATALFMT()`.
- **Recoverable errors**: return a `Status` enum value.
- **Propagation idiom**: uses the `status_CHECK` macro, which requires the local variable to be named `st`:
  ```c
  Status st;
  st = some_function(args); status_CHECK;
  ```
- The `status_CHECK` macro expands to `if (st != status_OK) { return st; }`.

### Memory

- **No global variables in library code**. Globals are only allowed in `tests/` and `cmd/`, prefixed with `g_namespace_`.
- Functions needing dynamic allocation must receive an `IAllocator*` as an argument.
- Macros should be used sparingly.

### Debug Mode

- Controlled by `config_DEBUG` in `lib/config.h` (currently set to `1`).
- Debug checks are gated with `#if config_DEBUG` and use `debug_FATAL`/`debug_FATALFMT` to crash with file/function/line context.
- Debug checks include: NULL pointer checks, aliasing requirement enforcement, digit-range validation, bounds checking on allocators, double-free detection.

### Comment System — Footnotes

Large comments go as **footnotes** just before the function's closing `}`. In the function body, use only a short marker like `// NOTE(1)`. One-line comments that fit within 80 columns may stay inline.

Annotation types:
- `NOTE(n)`: Rationale explanation not tied to safety.
- `SAFE(n)`: Proves why an operation is safe (cast, overflow, bounds).
- `UNSAFE(n)`: Warns about an assumption that could break.
- `UNTESTED`: Marks functions lacking automated tests.
- `DRAGON`: Marks hard-to-understand code that cannot be simplified.
- `TODO`: Work that must be finished.

Docstrings go **above** the function: short, descriptive, to the point.

### File Structure

Every library `.h` file must:
1. Start with the MIT license block:
   ```c
   /*
   MIT License
   Copyright 2025 Artur Iure Vianna Fernandes
   See the LICENSE file for more information.
   */
   ```
2. Use header guard `#ifndef PAO_<NAMEINCAPS>_H` / `#define PAO_<NAMEINCAPS>_H`.
3. Include dependencies immediately after the guard.
4. End with `#endif`.

### Testing

- Test files live in `tests/`, named `*_test.c`.
- Crash tests (that verify debug assertions fire) are named `*_crash.c`.
- Only test **public** API — never test `i_` internal functions.
- Functions that write to stdout may be left untested but must be marked `UNTESTED`.
- **Compiler flags**: `-Wall -Wextra -Werror -pedantic -Wconversion -Wstrict-prototypes -std=c99 -O2`.
- **Before committing**: run `tests/testp` (gcc/clang × c99/c17 × -O0/-O2).
- Test framework: use the `Tester` struct array + `run_tests()` from `tests/common.h`.
- New things must be tested before moving on. If committed without tests, mark with `UNTESTED`.

### Platform Assumptions

- POSIX-compliant operating system.
- Architecture: amd64 or arm64.
