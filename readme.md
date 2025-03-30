# Padeir0's C23 Library (pao)

[Rationale](https://www.youtube.com/watch?v=Fm5Ust7vEhk).

## Structure

All libs must be a single `.h` file,
named `pao_<namespace>.h`,
with the MIT LICENSE up top.
Header guards must be
`#define PAO_<NAMESPACE>_H`.

All public function names must be of the form
`pao_<namespace>_<name>`, while private functions
must be of the form `_pao_<namespace>_<name>`.

All public macros and constants must be of the form `PAO_<NAMESPACE>_<NAME>`,
while private macros and constants must be of the form `_PAO_<NAMESPACE>_<NAME>`.

All public types must be typedef'd, with names
of the form `pao_<Name>`, private types must be
of the form `_pao_<Name>`.

Types often define the namespaces of functions, for example
`pao_Natural` will define the namespace for the functions
`pao_natural_add`, `pao_natural_mult`, etc.

## Guidelines

Guidelines are for libraries, tests can ignore some of these.

 - All code must be run with `-Wall -Wextra -Werror -std=c23`
 - Macro usage is restricted to `#define` constants and header guards.
 - Never use global variables.
 - Functions that need dynamic allocation should receive allocators as arguments.
 - Never use `goto`, `continue` or `for` loops.
 - Never take the address of local variables.
 - Never modify procedure arguments, never take their address.
 - All private functions must be `static`.
 - Only private functions may be marked with `inline`.
 - Only depend on standard library.
 - Never use inline assembly.
 - Provide no compile-time library configuration options.
 - Provide no compatibility with C++.
 - Assume POSIX compliant operating systems.
 - Write sufficient tests for a feature before plunging into the next.
 - Bad usage and internal errors should crash the program.
 - Recoverable errors should return a `pao_status` code.

