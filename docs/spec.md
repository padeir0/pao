# Marabaixo

Marabaixo will be, at first, a dynamically typed functional programming language,
it may be that later I add a type system on top of it.

<details>

<summary>Contents</summary>

</details>

## Example

```
let sq
  function [x] [mul x x]

let point [struct x y z]

let distance
  function [a, b]
    sqrt
      sum [sq a.x - b.x]
          [sq a.y - b.y]
          [sq a.z - b.z]

let origin [point 0 0 0]
let magnitude
  function [a]
    distance a origin
    
let some-point
  point x = 3.14
        y = 22/7
        z = 157/50

print-ln "magnitude: "
         [magnitude some-point]
```

The whole language revolves around the call syntax.
Like the function `sq` above, it's possible to call it with
the argument `2` like `[sq 2]` or in a few other ways.
Almost everything looks like a function, including special constructs
like `if` and `let`. This is intentional and is meant to make the
syntax simpler and more homogenous. The syntax is indentation
sensitive and the overall structure is derived from S-Expressions.

When calling functions, or anything that resemble functions,
you can use the named-parameter syntax with `=`,
just like in the declaration of `some-point` above.

## The call syntax

Consider the `point` struct used earlier: 

```
let point
  [struct x y z]
```

Then the value of all the following expressions have the same value:

```
[point [sq 2] 3 4]

point [sq 2] 3 4

point [sq 2] 3
  4

point [sq 2]
  3
  4

point
  [sq 2]
  3
  4

point
  sq 2
  3
  4

point
  sq
    2
  3
  4

point [sq 2] \
  3 4
```

## Syntax

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
Terms = Term {[','] Term} [','].
Term = Factor {Suffix}.
Suffix = SuffixOp Factor.
SuffixOp = ':' | '.' | '='.

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

## Function arguments

If the last argument of a function is named `args`
then this argument is treated as variadic.

When declaring a function, it's possible to declare
the last couple arguments as optionals. Here's how:

```
let ball-volume
  function [radius, dimension = 2]
    case
      [equal? dimension 2]
        mul pi [sq radius]
      [equal? dimension 3]
        mul 3/4 pi [cube radius]

print "unit-ball in 2D"
  ball-volume 1;
print "unit-ball in 3D"
  ball-volume 1, dimension = 3;
```

## List of built-in names

Counted 78 names (`[\w\-\?]+\s`).

```
values:
  nil true false

special forms:
  let help function
  if  case begin
  import   export

logical:
  not and or

comparison:
  equal? not-equal?
  less?  greater?
  less-or-equals? greater-or-equals?

predicates:
    string? number?   list? atom?
    symbol? function? form? nil?
    exact?  inexact?  proper? improper?

math:
    add sub mul div
    rem even? odd?
    abs round ceil
    floor pow

conversions:
    to-string to-symbol
    to-number to-list

numerical:
    to-exact  to-inexact max-precision
    numerator denominator

list:
  pair   head tail   last list length
  append map  filter fold for  reverse
  range  sort unique

string:
  join  concat format slice
  split str-len

other:
  print abort
```
