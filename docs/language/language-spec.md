# Ovo

Ovo will be, at first, a dynamically typed functional programming language,
it may be that later I add a type system on top of it.
The name is because pão com ovo is very good.

<details>

<summary>Contents</summary>

</details>

## Example

```
let sq
  function (x) (mul x x)

let point (struct x y z)

let distance
  function (a, b)
    sqrt
      sum (sq (sub a.x b.x))
          (sq (sub a.y b.y))
          (sq (sub a.z b.z))

let origin (point 0 0 0)
let magnitude
  function (a)
    distance a origin
    
let some-point
  point x = 3.14
        y = 22/7
        z = 157/50

print-ln "magnitude: "
         (magnitude some-point)
```

The whole language revolves around the call syntax.
Like the function `sq` above, it's possible to call it with
the argument `2` like `(sq 2)` or in a few other ways.
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
  (struct x y z)
```

Then the value of all the following expressions have the same value:

```
(point (sq 2) 3 4)

point (sq 2) 3 4

point (sq 2) 3
  4

point (sq 2)
  3
  4

point
  (sq 2)
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

point (sq 2) \
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
Comment = '#' {not_newline_char}.

Program = Block.
Block = {:I_Expr NL} [:Closing_Expr NL].

I_Expr = Terms {Line_Continue} [NL >Block].
Closing_Expr = Terms {Line_Continue} ';'.
Line_Continue = '\\' NL Terms.
Terms = Term {[','] Term} [','].
Term = Factor_3 ['=' Factor_3].
Factor_3 = Factor_2 [':' Factor_2].
Factor_2 = Factor {'.' Factor}.

Factor = Atom | S_Expr | Tuple_Expr.

S_Expr = '(' Terms ')'.
Tuple_Expr = '[' Terms ']'.
Atom = id | num | str.

NL = '\n' {'\n'}.

str = '"' {str_char | escape} '"'.
str_char = /[^"\\\n\t\x00-\x1f\x7f]/ | '\r'.
escape = '\\' ('n' | 't' | '\'' | '"' | '\\').

id = ident_begin {ident_continue}.
ident_begin = /[a-zA-Z_]/.
ident_special = /[\?\-\!]/
ident_continue = ident_begin | ident_special.

num = hex | bin | dec.
dec = [neg] integer [frac | float] [exp] [suffix].
integer = digit {digit_}.
frac = '/' integer.
float = '.' [integer].
exp = 'e' [neg] integer.
suffix = 'f'.

hex = '0x' hexdigits.
hexdigits = /[0-9A-Fa-f_]*[0-9A-Fa-f][0-9A-Fa-f_]*/.
bin = '0b' bindigits.
bindigits = /[01_]*[01][01_]*/.

neg = '-'.
digit = /[0-9]/.
digit_ = digit | '_'.
```

### Syntax notes

 - The suffixes are the operators `.`, `:` and `=`, which bind in this
   order: `.` first, `=` last. `=` is for named parameters (and for
   `let a = 1`), `:` is for type annotations, and `.` is for accessing
   what is inside another namespace (a module, a struct, etc). Only `.` may
   be repeated, as in `a.b.c`: `a = b = c` and `a : b : c` are errors, and
   `let a.b = c.d` is `let` called with `a.b = c.d`, whose operands are
   `a.b` and `c.d`.
 - A `S_Expr`, between parentheses, is always a call: `(f x)`, and even
   `(f)`. A `Tuple_Expr`, between square brackets, is a tuple: `[a, b, c]`.
   The parser never unwraps a tuple, so `[a]` is a tuple of one element, and
   it is up to the evaluator to take it as a grouping. This is how a suffix
   is the operand of another one: `let x = [a.y]`, `let [p.a] = [p.b]`.
 - `Terms` has at least one term, so `()` and `[]` are errors. A second `,`
   in a row is an error too.
 - A `;` is the last token of the last line of a block, and it closes that
   block. It is optional, and it is there so that a block cannot grow by
   accident when the code is edited: nothing can come after it but the end
   of the input or a line that is less indented.
 - The first token of the program sets the column of its top level. The
   expressions of a block are in the same column, and the lines of a
   `Line_Continue` must be more indented than the line they continue.
   A line that is less indented than a block ends it, and has to be in the
   column of a block that is still open.
 - A newline is not allowed inside `(...)` or `[...]`: an expression is
   written either with them, in a single line, or with the indentation.

### Lexical notes

The grammar above is what the lexer (`lib/lexer.h`) implements, with
these details that the notation does not show:

 - The lexer emits one `NL` for each `'\n'`, and no `NL` is made up at the
   end of the input. The parser treats a sequence of `NL` as the single `NL`
   of the grammar. A comment does not consume the `'\n'` that ends it.
 - The whole source must be valid UTF-8. Outside strings and comments only
   ASCII is allowed. Tabs and the other control characters
   (U+0000 to U+001F, and U+007F) are invalid everywhere, except `'\n'`
   and `'\r'`.
 - `neg` is only part of decimal numbers, so `-0x1` is not a number.
   A `-` that does not start a number is invalid.
 - `float` may have no digits after the dot, so `3.` is a number, and `3.x`
   is not. A number after a `'.'` is still a number: `a.0.1` is `a`, `.`
   and `0.1`.
 - The `f` is lexed, but whoever converts the number decides what it means.
 - A `num`, `id` or `str` must be followed by the end of the input or by one
   of: whitespace, `'\n'`, `'#'`, `']'`, `')'`, `','`, `';'`, `':'`, `'='`,
   `'\\'`, and also `'.'`, except after a `num`. So `a-1`, `x1`, `1-2`, `3x`,
   `"a""b"`, `f(x)` and `f[x]` are not valid, and `a.b`, `(f x)` and `1,2`
   are. An opening `(` or `[` is not a delimiter on purpose: `f(x)` is not
   a call of `f` as it would be in other languages (a call is `(f x)`), and
   what follows a term has to be separated from it, as in `f (x)`.

## Function arguments

If the last argument of a function is named `args`
then this argument is treated as variadic.

When declaring a function, it's possible to declare
the last couple arguments as optionals. Here's how:

```
let ball-volume
  function (radius, dimension = 2)
    case
      (equal? dimension 2)
        mul pi (sq radius)
      (equal? dimension 3)
        mul 3/4 pi (cube radius)

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
