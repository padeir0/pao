# Pão

Pão will be a functional programming language meant to be an interface
to C code, nothing more. My intention is to slowly introduce more static
checks into the language, even if this extra information gets discarded
at runtime.

<details>

<summary>Contents</summary>

- [The List](#list)
- [Forms](#forms)
- [Syntax](#syntax)
- [Semantics](#semantics)

</details>

## The List <a name="list"></a>

Consider the S-Expression `(f (a b) c d)`,
the following Pão expressions are equivalent:

```
[f [a b] c d]

f [a b] c d

f [a b] c
  d

f [a b]
  c
  d

f
  [a b]
  c
  d

f
  a b
  c
  d

f
  a
    b
  c
  d

f [a b] \
  c d
```

## Syntax <a name="syntax"></a>

Notation here is [Wirth Syntax Notation](https://dl.acm.org/doi/10.1145/359863.359883)
with extensions from the article
[Indentation-Sensitive Parsing for Parsec](https://osa1.net/papers/indentation-sensitive-parsec.pdf)
and [PCRE](https://www.pcre.org/original/doc/html/pcresyntax.html).

These extensions are, briefly:
 - the _justification operator_ `:` that forces the production to be in the same indentation as the parent production;
 - the _indentation operator_ `>` that forces the production to be in an indentation _strictly greater than_ the parent production;
 - the indentation level of a production, which is defined to be the column position of the first token that is consumed (or produced) in that production;
 - the production `Whitespace` that indicates tokens that serve only as separators and are otherwise ignored;
 - the regular expressions, which are inside `//`.

```ebnf
Whitespace = '\r' | ' ' | Comment.
Comment = '#' {not_newline_char} '\n'.

Program = Block.
Block = {:I_Expr NL} [';'].

I_Expr = Terms {Line_Continue} [NL >Block].
Line_Continue = '\\' NL Terms.
Terms = Term {Term}.
Term = {Prefix} Factor {Suffix}.
Prefix = PrefixOp Factor.
PrefixOp = '@'.
Suffix = SuffixOp Factor.
SuffixOp = ':' | '.'.

Factor = Atom | S_Expr.

S_Expr = '[' Terms ']'.
Atom = id | num | str.

NL = '\n' {'\n'}.

str = /"[\u0000-\uFFFF]*"/.

id = ident_begin {ident_continue}.
ident_begin = /[a-zA-Z_]/.
ident_special = /[\?\-\!]/
ident_continue = ident_begin | ident_special | digit.

num = hex | bin | dec.
dec = [neg] integer [frac | float] [exp].
integer = digit {digit_}.
frac = '/' integer.
float = '.' integer.
exp = 'e' [neg] integer.

hex = '0x' hexdigits.
hexdigits = /[0-9A-Fa-f_]+/.
bin = '0b' bindigits.
bindigits = /[01_]+/.

neg = '-'.
digit = /[0-9]/.
digit_ = digit | '_'.
```

## Name Resolution

Names are statically resolved top down, except in case of `letrec`s,
which are resolved out-of-order.
Both `let` and `letrec` define names in the enclosing scope.
Only functions and modules may create new scopes.

Modules are resolved before the main symbols are resolved.
Imports work with `export`, `import` and `from ... import`,
similar to python. `export`s are deferred to be evaluated
at the end of the program.

## Special Forms and Modules

Special forms are not first class objects,
you can't pass them to functions or assign them to variables.
Some examples of special forms are:

```
if cond function let
```

They do not behave like functions, parameters to special forms
are only evaluated according to the semantics of that specific special form.
They are essentially "intrinsic macros" or "intrinsic FEXPR".

### Types

IML is dinamically typed.
There are only a few types visible to the user,
these types are defined by the following predicates:

```
list? number? nil? bool? string? function?
```

There are subtypes of numbers, which are also visible:

```
integer? rational? decimal? exact? inexact?
```

But be aware that `(exact? x)` is the same as
`(or (integer? x) (rational? x))`, while `(inexact? x)`
is the same as `(not (exact? x))`.

Some well behaved lists also have predicates, but
do not correspond to runtime types:

```
proper-list? improper-list? proper-pair? improper-pair?
```

Conversion between types uses conversion functions:
```
number->string list->string
symbol->string bool->string

string->number string->list

exact->inexact inexact->exact

bool->number number->bool
```

### Pure vs Impure

Mutation is not allowed, there's no way to perform
assignment in the language, however,
there are still side-effects.
