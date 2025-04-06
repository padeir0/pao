# Padeir0's C99 Library (pao)

[Rationale](https://www.youtube.com/watch?v=Fm5Ust7vEhk).

## Identifier Style

All libs must be a single `.h` file,
named `pao_<namespaceName>.h`, ie,
the namespace must be in camelCase.
All files must start with the MIT LICENSE up top.
Header guards must be `#define PAO_<namespaceName>_H`.

Underscores `_` are reserved, so that snake_case is not allowed.
Instead we use camelCase for compound names,
and use underscores `_` to separate namespace names and internal
names. Identifier rules are the following:
 - Public names start with `pao_`, while internal (private) names
 start with `i_pao_`.
 - All public function names must be of the form
`pao_<namespaceName>_<nameCamelCase>`, while private functions
must be of the form `i_pao_<namespaceName>_<nameCamelCase>`.
 - All public macros, constants and enums must be of the form `PAO_<namespaceName>_<nameCamelCase>`,
while private macros, constants and enums must be of the form `I_PAO_<namespaceName>_<nameCamelCase>`.
 - All public types must be typedef'd, with names
of the form `pao_<NameUpperCamelCase>`, private types must be
of the form `i_pao_<NameUpperCamelCase>`.
 - All local variables, arguments, struct fields and union fields must be camelCase,
 no need to prefix them with `i_pao_` or `pao_`.

Types often define the namespaces of functions, for example
`pao_Natural` will define the namespace for the functions
`pao_natural_add`, `pao_natural_addDigit`, etc.

Exceptions to this naming rule are the types inside `pao_basicTypes.h`.

## Guidelines

Guidelines are for libraries, tests can ignore some of these.

 - All code must run with `-Wall -Wextra -Werror -pedantic -Wconversion`
 for each C version from C99 to C23 in _both_ `gcc` and `clang`
 (see `tests/run_portable_tests`).
 - Macro usage is restricted to `#define` constants and header guards.
 - Never use global variables.
 - Functions that need dynamic allocation should receive allocators as arguments.
 - Never use `goto`, `continue` or `for` loops.
 - Never take the address of local variables.
 - Never modify procedure arguments, never take their address.
 - All private functions must be `static`.
 - Only private functions may be marked with `inline`.
 - Only depend on standard library.
 - Provide no compile-time library configuration options.
 - Provide no compatibility with C++.
 - Assume POSIX compliant operating systems.
 - Assume architecture is either amd64 or arm64.
 - Write sufficient tests for a feature before plunging into the next.
 - Bad usage and internal errors should crash the program.
 - Recoverable errors should return a `pao_status` code.
