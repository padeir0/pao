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
   - [ ] Free-list allocator.
 - [ ] UTF8 Encoder/Decoder
   - [ ] Encoder
   - [x] Decoder
 - [ ] Associative arrays
   - [ ] `str -> void*` linear hashmap (only insert, lookup and clear, can use a simple arena allocator) 
   - [ ] `str -> void*` hashmap (insert, lookup, clear, remove and update, must use freelist or similar)
 - [ ] Command line argument parsing

## Language
 - [ ] Debug allocator (generic free-list allocator that tracks where things were allocated).
       (keeps track of block ranges, creates new fixed-sized-blocks when memory is full,
       uses a single type of static-heap allocator internally).
 - [ ] interpreter
   - [ ] List Data Structure
   - [ ] Lexer
   - [ ] Parser
   - [ ] Evaluation

## Draughts engine
 - [ ] Game data structure
 - [ ] Board printing
 - [ ] FEN Parser (outputs a board)
 - [ ] PDN Parser (outputs a game)
 - [ ] Move generator
 - [ ] Perft testing
 - [ ] Material-only evaluation
 - [ ] Minimax search
 - [ ] Alpha-beta search
 - [ ] Piece-square tables
 - [ ] Piece-square table tuning
