# Plans

- [x] Allocator Interface
- [ ] String type as `{ptr, len, cap}` with mutable and immutable semantics (procedures that mutate strings must end with `_mut`).
- [ ] Arbitrary precision numbers
  - [ ] Natural numbers.
  - [ ] Integers.
  - [ ] Rationals.
  - [ ] Decimals (fixed point).
- [ ] Allocators with static heap size.
  - [ ] Arena allocator.
  - [ ] Stack allocator.
  - [ ] Queue allocator.
  - [ ] Pool allocator.
  - [ ] Free-list allocator.
  - [ ] Debug allocator (generic free-list allocator that tracks where things were allocated).
- [ ] Generic multi-block allocator
(keeps track of block ranges, creates new fixed-sized-blocks when memory is full, uses a single type of static-heap allocator internally).
- [ ] Command line argument parsing
- [ ] Associative arrays
  - [ ] `str -> void*` linear hashmap (only insert, lookup and clear, can use a simple arena allocator)
  - [ ] `str -> void*` hashmap (insert, lookup, clear, remove and update, must use freelist or similar)
- [ ] Guira
- [ ] AMD64 compiler backend
- [ ] ARM64 compiler backend
