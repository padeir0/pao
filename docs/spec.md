# Iure's Minimal Language

Iure's Minimal Language (IML) is a simple scripting language for my C library.
This document is a quick, informal and incomplete specification.

<details>

<summary>Contents</summary>

- [The List](#list)
- [Forms](#forms)
- [Syntax](#syntax)
- [Semantics](#semantics)

</details>

## The List <a name="list"></a>

This syntax simply defines the structure of a list,
here, we will use S-Expressions to show how this list is parsed.

Consider the S-Expression `(f (a b) c d)`,
the following IML expressions are equivalent:

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

The syntax draws inspiration from
[S-expressions](https://www-sop.inria.fr/indes/fp/Bigloo/doc/r5rs-10.html#Formal-syntax),
[T-expressions](https://srfi.schemers.org/srfi-110/srfi-110.html),
[I-expressions](https://srfi.schemers.org/srfi-49/srfi-49.html),
[O-expressions](http://breuleux.net/blog/oexprs.html),
[M-expressions](https://en.m.wikipedia.org/wiki/M-expression) and
[Wisp](https://srfi.schemers.org/srfi-119/srfi-119.html).

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

I_Expr = Terms {Line_Continue} [End | NL >Block].
Line_Continue = '\\' NL Terms.
Terms = Term {Term}.
End = '.' Term.
Term = [Name] (Atom | S_Expr).
S_Expr = '[' Terms [End] ']'.
Name = id ':'.

NL = '\n' {'\n'}.
Atom = id | num | str.

str = /"[\u0000-\uFFFF]*"/.

id = ident_begin {ident_continue}.
ident_begin = /[a-zA-Z_\$]/.
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

## Semantics <a name="semantics"></a>

## Special Forms and Modules

Special forms and modules are not first class objects,
you can't pass them to functions or assign them to variables.
Similarly, special forms cannot be shadowed by other
variables. Some examples of special forms are:

```
if cond function let
```

They do not behave like functions, parameters to special forms
are only evaluated according to the semantics of that specific special form.
They are essentially "intrinsic macros".

### Types

IML is dynamically typed, with only a few types visible to the user,
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

## TO DO LIST

 - [ ] Number
  - [ ] Natural
   - [x] `natural_snprint`
   - [x] `natural_equalDigit`
   - [x] `natural_equal`
   - [x] `natural_addDigit`
   - [x] `natural_subDigit`
   - [x] `natural_multDigit`
   - [ ] `natural_divDigit`
   - [ ] `natural_add`
   - [ ] `natural_distance`
   - [ ] `natural_mult`
   - [ ] `natural_div`
   - [ ] `natural_compare`
  - [ ] Integer
   - [ ] `integer_snprint`
   - [ ] `integer_equalDigit`
   - [ ] `integer_equal`
   - [ ] `integer_neg`
   - [ ] `integer_abs`
   - [ ] `integer_addDigit`
   - [ ] `integer_subDigit`
   - [ ] `integer_multDigit`
   - [ ] `integer_divDigit`
   - [ ] `integer_add`
   - [ ] `integer_sub`
   - [ ] `integer_mult`
   - [ ] `integer_div`
   - [ ] `integer_compare`
  - [ ] Rational
   - [ ] `rational_snprint`
   - [ ] `rational_equalDigit`
   - [ ] `rational_equal`
   - [ ] `rational_neg`
   - [ ] `rational_abs`
   - [ ] `_rational_gcd`
   - [ ] `rational_addDigit`
   - [ ] `rational_subDigit`
   - [ ] `rational_multDigit`
   - [ ] `rational_divDigit`
   - [ ] `rational_add`
   - [ ] `rational_sub`
   - [ ] `rational_mult`
   - [ ] `rational_div`
   - [ ] `rational_compare`
 - [ ] String
  - [ ] `string_concat`
  - [ ] `string_copy`
