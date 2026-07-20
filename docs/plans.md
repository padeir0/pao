# Plans

## Lib
 - [x] Allocator Interface
 - [x] read/write Buffer type as `{ptr, start, len, cap}`
 - [ ] Arbitrary precision numbers
   - [x] Natural numbers.
   - [x] Integers.
   - [ ] Rationals.
   - [ ] Decimals (fixed point).
 - [ ] Allocators with static heap size.
   - [x] Arena allocator.
   - [ ] Stack allocator.
   - [x] Pool allocator.
   - [x] Free-list allocator.
 - [ ] UTF8 Encoder/Decoder
   - [ ] Encoder
   - [x] Decoder
 - [ ] Command line argument parsing

## Language

O Lexer do ovo vai cuspir átomos do `list.h` direto,
o parser vai criar a lista. Precisamos primeiro terminar o `list.h`:

 - [ ] `number.h` (implementa a number tower como um único objeto)
 - [ ] `string.h` (implementa a string imutável)
 - [ ] `ovodict.h` (implementa um dicionário `string -> Atom`)
 - [ ] `symbol.h` (implementa o `SymbolSpace` com internalização de strings pro `Symbol` ser só um inteiro)
 - [ ] criar o tipo `Form`
 - [ ] criar o tipo `Function`

Precisamos também:

 - [ ] Usar a config `config_DEBUG` pra adicionar a informação de onde cada objeto foi alocado nos alocadores.

Daí faremos:

 - [ ] `ovoalloc.h` (alocador especializado pra estrutura `List` e os subtipos de `Atom`, reusa os outros alocadores primitivos)
 - [ ] Lexer
 - [ ] Parser
 - [ ] Evaluation
 - [ ] Built-ins (fuckton of them)
