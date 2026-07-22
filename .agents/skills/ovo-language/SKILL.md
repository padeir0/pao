---
name: ovo-language
description: Complete reference for the Ovo programming language specification — its custom extended Wirth Syntax Notation, call syntax semantics, built-in names, data structures, and implementation roadmap.
---

# Ovo Language Reference

Ovo is a dynamically typed, functional programming language with indentation-sensitive syntax derived from S-expressions. The name comes from "pão com ovo" (bread with egg).

Full spec: `docs/language/language-spec.md`. Roadmap: `docs/plans.md`.

---

## Grammar Notation — Extended Wirth Syntax Notation

> **Critical**: The Ovo grammar uses a non-standard formalism. An agent that only knows BNF/EBNF will misinterpret the `:` and `>` operators as literal characters. Read this section carefully before implementing or modifying the lexer/parser.

The notation is [Wirth Syntax Notation](https://dl.acm.org/doi/10.1145/359863.359883) extended with constructs from two sources:
- [Indentation-Sensitive Parsing for Parsec](https://osa1.net/papers/indentation-sensitive-parsec.pdf) — by Ömer Sinan Ağacan
- [PCRE](https://www.pcre.org/original/doc/html/pcresyntax.html) — for inline regex

### Standard WSN Elements

| Syntax | Meaning |
|---|---|
| `=` | Definition |
| `.` | Termination |
| `'...'` | Terminal (literal character or string) |
| `\|` | Alternation |
| `( )` | Grouping |
| `{ }` | Repetition (zero or more) |
| `[ ]` | Optional (zero or one) |

### Indentation Extensions

These three operators are the key innovation. They control how productions relate to each other via **indentation level** (column position of the first token consumed in a production):

| Operator | Name | Semantics |
|---|---|---|
| `:` | Justification | The child production must start at the **same** indentation level as the parent production |
| `>` | Indentation | The child production must start at an indentation level **strictly greater than** the parent production |
| *(implicit)* | Indentation level | Defined as the column position of the first token that is consumed (or produced) in that production |

### Regex Extension

Regular expressions appear between `//` delimiters within the grammar:

```
str = /"[\u0000-\uFFFF]*"/.
digit = /[0-9]/.
```

### Special Productions

- `Whitespace = '\r' | ' ' | Comment.` — tokens that serve only as separators and are otherwise ignored
- `NL = '\n' {'\n'}.` — newlines are significant (indentation sensitivity)

---

## The Complete Formal Grammar

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

### Key Grammar Notes

- **`Block = {:I_Expr NL} [';'].`** — The `:` operator here means each `I_Expr` in the block must start at the **same** indentation level. This is how the language knows block structure.
- **`I_Expr = Terms {Line_Continue} [NL >Block].`** — The `>` operator means the optional child block must be indented **further** than the expression that introduces it.
- **`Line_Continue = '\\' NL Terms.`** — Backslash allows continuing an expression on the next line.
- **Suffix operators** (`:`, `.`, `=`) are binary operators used for named params (`x = 3`), field access (`a.x`), and method-like calls (`a:method`).
- **Numbers** support: integers, fractions (`22/7`), floats (`3.14`), scientific notation (`1e10`), hex (`0xFF`), binary (`0b1010`), and digit separators (`1_000_000`).
- **Identifiers** can contain `?`, `-`, `!` (e.g., `equal?`, `not-equal?`, `print-ln`).

---

## Call Syntax — 8 Equivalent Forms

Given `let point [struct x y z]`, all of these produce the same value:

```
# Form 1: Fully bracketed
[point [sq 2] 3 4]

# Form 2: Bare call (first position is the function)
point [sq 2] 3 4

# Form 3: Partial indent
point [sq 2] 3
  4

# Form 4: Args on separate lines
point [sq 2]
  3
  4

# Form 5: Fully indented
point
  [sq 2]
  3
  4

# Form 6: Nested bare calls via indent
point
  sq 2
  3
  4

# Form 7: Deep nesting via indent
point
  sq
    2
  3
  4

# Form 8: Line continuation with backslash
point [sq 2] \
  3 4
```

The core rule: an indented block after an expression provides additional arguments to that expression.

### Named Parameters

Functions support named parameters using `=`:

```
ball-volume 1, dimension = 3
```

Optional parameters are declared with defaults:
```
function [radius, dimension = 2]
```

---

## Built-in Names — 78 Total

### Values
`nil`, `true`, `false`

### Special Forms
`let`, `help`, `function`, `if`, `case`, `begin`, `import`, `export`

### Logical
`not`, `and`, `or`

### Comparison
`equal?`, `not-equal?`, `less?`, `greater?`, `less-or-equals?`, `greater-or-equals?`

### Predicates
`string?`, `number?`, `list?`, `atom?`, `symbol?`, `function?`, `form?`, `nil?`, `exact?`, `inexact?`, `proper?`, `improper?`

### Math
`add`, `sub`, `mul`, `div`, `rem`, `even?`, `odd?`, `abs`, `round`, `ceil`, `floor`, `pow`

### Conversions
`to-string`, `to-symbol`, `to-number`, `to-list`

### Numerical
`to-exact`, `to-inexact`, `max-precision`, `numerator`, `denominator`

### List
`pair`, `head`, `tail`, `last`, `list`, `length`, `append`, `map`, `filter`, `fold`, `for`, `reverse`, `range`, `sort`, `unique`

### String
`join`, `concat`, `format`, `slice`, `split`, `str-len`

### Other
`print`, `abort`

---

## Data Structures — Current State

Defined in `lib/list.h`:

```c
typedef enum {
  atomKind_NULL,
  atomKind_BOOL,
  atomKind_NUMBER,
  atomKind_STRING,
  atomKind_LIST,
  atomKind_FUNCTION,
  atomKind_SYMBOL,
  atomKind_FORM,
} AtomKind;

typedef union i_AtomValue {
  Nil Nil;        // ✓ implemented (typedef byte Nil)
  bool Bool;      // ✓ implemented
  List* List;     // ✓ implemented
  // String String;     // ✗ not yet
  // Number Number;     // ✗ not yet
  // Symbol Symbol;     // ✗ not yet
  // Form Form;         // ✗ not yet
  // Function Function; // ✗ not yet
} AtomValue;

typedef struct { AtomKind kind; AtomValue value; } Atom;
struct i_List { Atom value; struct i_List* next; };
```

The lexer is designed to output `Atom`s from `list.h` directly.

---

## Implementation Roadmap

From `docs/plans.md`, the planned order is:

1. **`number.h`** — Unify Natural/Integer/Rational into a single `Number` object (the number tower)
2. **`string.h`** — Immutable string type
3. **`ovodict.h`** — Dictionary: `String → Atom`
4. **`symbol.h`** — `SymbolSpace` with string interning so `Symbol` is just an integer
5. **Create `Form` and `Function` types** — complete the `AtomValue` union
6. **`ovoalloc.h`** — Specialized allocator for `List` and `Atom` subtypes, reusing the primitive allocators
7. **Lexer** — tokenize source into atoms
8. **Parser** — build list structures from token stream (indentation-aware)
9. **Evaluation** — tree-walking interpreter
10. **Built-ins** — implement all 78 names

### Key Design Constraint

The library must remain **pure C with no side effects** beyond memory management — this is required for future Emscripten compilation to WebAssembly (for the Dama checkers UI in the browser).
