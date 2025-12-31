# Plans

- [x] Allocator Interface
- [x] read/write Buffer type as `{ptr, start, len, cap}`
- [ ] Arbitrary precision numbers
  - [ ] Natural numbers.
  - [ ] Integers.
  - [ ] Rationals.
  - [ ] Decimals (fixed point).
- [ ] UTF8 Encoder/Decoder
- [ ] Command line argument parsing
- [ ] Allocators with static heap size.
  - [x] Arena allocator.
  - [ ] Stack allocator.
  - [ ] Pool allocator.
  - [ ] Free-list allocator.
  - [ ] Debug allocator (generic free-list allocator that tracks where things were allocated).
(keeps track of block ranges, creates new fixed-sized-blocks when memory is full, uses a single type of static-heap allocator internally).
- [ ] Associative arrays
  - [ ] `str -> void*` linear hashmap (only insert, lookup and clear, can use a simple arena allocator) 
  - [ ] `str -> void*` hashmap (insert, lookup, clear, remove and update, must use freelist or similar)
- [ ] Pão interpreter
