# Style-guide

Write C without a style-guide and suddenly you'll see something like:
```c
int a=10000,b,c=2800,d,e,f[2801],g;main(){for(;b-c;)f[b++]=a/5;
for(;d=0,g=c*2;c-=14,printf("%.4d",e+d/a),e=d%a)for(b=c;d+=f[b]*a,
f[b]=d%--g,d/=g--,--b;d*=b);}
```

## Identifier Style

All libs inside `src` must be a single `.h` file,
named `pao_<namespaceName>.h`, ie,
the namespace must be in camelCase.
All files must start license information up top.
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
 - All global variables must be prefixed with `g_pao_` and must only exist inside tests.

Types often define the namespaces of functions, for example
`pao_Natural` will define the namespace for the functions
`pao_natural_add`, `pao_natural_addDigit`, etc.

Exceptions to this naming rule are the types inside `pao_basicTypes.h`.

## Comment Style

Because a lot of comments in the middle of the code makes the code harder to read,
i've adopted a policy to keep comments as footnotes. In functions, these footnotes
are just before the closing `}`, while in the middle of the function you'd only have
things like `// NOTE(1)`.

There are, so far, these kinds of notes:
 - `NOTE`: Is a brief explanation of rationale that is not tied to safety.
 - `SAFE`: Explains why some operations are indeed safe.
 - `UNSAFE`: Warns about unsafe situations that are assumed to never happen.
 - `UNTESTED`: Marks functions that have no automated test routine.
 - `DRAGON`: Marks a piece of code that is hard to understand and that I can't do nothing about it.
 - `TODO`: Things that i *need* to finish.

I've been trying to justify safety in all situations where something may
lead to undefined behaviour, memory corruption, integer overflow, integer underflow, etc.

## Testing

Tests must live inside the `test` folder. Any file that must be run as part
of an automated test routine must end with `*_test.c`.

Tests should be performed with at least the following flags in `gcc`:

```bash
gcc -Wall -Wextra -Werror -pedantic -Wconversion -Wstrict-prototypes -std=c99 -O2
```

But before any commits are made, the code must be tested in `clang`,
with different C standards and optimization levels.
It is sufficient to test for both C99 and C17 (See `tests/run_portable_tests`),
and both `-O0` and `-O2`. It is best if also tested in C23 and also with `-O3`.

Rationale for this is:
 - The code must be portable between `gcc` and `clang`.
 - The code must behave well _even if_ the compiler abuses undefined behaviours for optimization.
 - The code must *not* use any deprecated features of new C standards.

Tests should not test internal `i_pao` functions, only public ones.
It is fine for functions that write to `STDOUT` to be untested,
these should be marked as `UNTESTED`.

New things should be tested before the next one, otherwise
a cascade of untested things will collapse this codebase.
Anyway, if something is to be committed without tests,
a `UNTESTED` comment must be added to it.

## Operating System and Architecture

I boldly assume that the code will run inside a POSIX compliant operating system.
I also assume the architecture is either amd64 or arm64. Rationale is that
if i try to be too portable, then the code will be too messy. It is already
hard to be careful with bugs in these two targets, it would be exponentially
worse if i had to deal with Windows or with AVR.

## Function declaration

Functions with no arguments should receive an explicit `void` argument instead.
Out-parameters must be the last parameters of a function.
Normal parameters that are passed by pointer and not modified within the function
must be marked `const`.

## Miscelaneous

Other small things:
 - Macro usage is restricted to constants and header guards.
 - Never use global variables inside library code (inside tests, this is fine).
 - Functions that need dynamic allocation should receive allocators as arguments.
 - Never use `goto`, `continue` or `for` loops.
 - Avoid taking the address of local variables.
 - Never modify procedure arguments, never take their address.
 - All private functions must be `static`.
 - Only private functions may be marked with `inline`.
 - Only depend on standard library.
 - Provide no compile-time library configuration options.
 - Provide no compatibility with C++.
 - Bad usage and internal errors should crash the program.
 - Recoverable errors should return a `pao_status` code (or should document the return values).
