# Plans

[x] Allocator Interface
[ ] String type as `{ptr, len, cap}` with mutable and immutable semantics (procedures that mutate strings must end with `_mut`).
[ ] Arbitrary precision numbers
  [ ] Natural numbers.
  [ ] Integers.
  [ ] Rationals.
  [ ] Decimals (fixed point).
[ ] Allocators with static heap size (returns `null` when full)
  [ ] Arena allocator.
  [ ] Pool allocator.
  [ ] Free-list allocator.
  [ ] Stack allocator.
[ ] Allocators with dynamic heap size (calls `malloc` internally when full, may still return `null`)
  [ ] Arena allocator.
  [ ] Stack allocator.
  [ ] Pool allocator.
  [ ] Free-list allocator.
  [ ] Debug allocator (generic free-list allocator that tracks where things were allocated)
[ ] Command line argument parsing
[ ] Associative arrays
  [ ] `str -> void*` linear hashmap (only insert, lookup and clear, can use a simple arena allocator)
  [ ] `str -> void*` hashmap (insert, lookup, clear, remove and update, must use freelist or similar)
[ ] Guira
  [ ] Lexing
  [ ] Number tower structure
  [ ] List data structure
  [ ] Parsing
  [ ] Eval
  [ ] guiracore
