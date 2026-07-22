---
name: implement-feature
description: Structured process for implementing features, fixes, and TODOs — phased workflow with user review gates at Requirements, Design, Tasks, and Implementation.
---

# Implement Feature — Phased Workflow

Use this skill when the user asks you to implement a feature, fix a bug,
resolve TODOs, or make any non-trivial change to the Pão codebase.

**Core principle**: never skip a phase. Each phase produces a concrete artifact
that the user reviews before you proceed. This prevents wasted work and ensures
alignment.

---

## Overview

```
Phase 1: Requirements   → What exactly are we doing and why?
            ↓ user review
Phase 2: Design         → How will we do it? What are the invariants?
            ↓ user review
Phase 3: Tasks          → Ordered checklist of concrete work items
            ↓ user review
Phase 4: Implementation → Write code, one task at a time
            ↓ self-verify
Phase 5: Verification   → Prove it works: build, test, review
            ↓ user review
Done.
```

---

## Phase 1 — Requirements

**Goal**: Produce a clear, bounded specification of *what* will change and *why*.

### Process

1. **Read the `known-issues` and `codebase-map` skills** to understand existing
   context. Never start from scratch — the codebase already has documented
   TODOs, bugs, and architecture.
2. **Scope the work**. For broad requests like "fix all testing TODOs",
   enumerate every individual item. For feature requests, identify what modules
   are affected.
3. **Write the requirements artifact** (`implementation_plan.md`, section
   "Requirements") containing:

| Field | Content |
|---|---|
| **Goal** | One sentence: what does success look like? |
| **Items** | Numbered list of every concrete change. Each item gets a one-liner. |
| **Out of scope** | What we are explicitly *not* doing. |
| **Affected modules** | Which `lib/` headers and `tests/` files are touched. |
| **Dependencies** | Does any item depend on another being done first? |
| **Risks** | Anything that could go wrong (breaking API, UB, portability). |

4. **Stop and request review**. Do not proceed until the user approves.

### Example (for "fix all testing-related TODOs")

```markdown
## Requirements

**Goal**: Resolve every TODO, UNTESTED marker, and known bug that relates to
testing infrastructure or missing test coverage.

### Items
1. Fix `bzero` → `memset` in `tests/common.h` (known-issues #8)
2. Fix typo `sucesses` → `successes` in `tests/common.h` (known-issues #15)
3. Add OOM failure-path tests for `natural.h` using `i_FailAllocHeap`
   (known-issues #11)
4. Test `buffer_toHex` (in-code TODO at `buffer.h:135`)
5. Test `rational_snprint` (in-code TODO at `rational.h:71`)
6. Test `rational_equal` (in-code TODO at `rational.h:95`)
7. Test `order_invert` (module with no tests)
8. Test `util_absI32` / `util_absI64` (module with no tests — but first fix
   the UB in known-issues #2)

### Out of scope
- Implementing missing rational arithmetic (add, sub, mul, div, etc.)
- Adding sanitizer flags or CI (known-issues #10, #17)
- Build system changes (known-issues #9)

### Affected modules
- `tests/common.h`, `tests/buffer_test.c`, `tests/numbers/rational_test.c`
- New files: `tests/order_test.c`, `tests/util_test.c`
- `lib/util.h` (bug fix required before testing)

### Dependencies
- Item 8 depends on fixing the UB in `util_absI32`/`util_absI64` first.
- Item 4 depends on verifying the `buffer_toHex` write offset (known-issues #4).

### Risks
- Fixing `util_absI32` changes return type from `i32` to `u32`, which may
  break callers.
```

---

## Phase 2 — Design

**Goal**: For each approved requirement item, define *how* it will be
implemented, what invariants must hold, and what correctness properties the
tests must verify.

### Process

1. **For each item**, write:

| Field | Content |
|---|---|
| **Approach** | How will this be implemented? Pseudocode or description. |
| **Invariants** | What must always be true before/after this code runs? |
| **Correctness criteria** | How do we know the implementation is correct? |
| **Test strategy** | What tests will be written? What do they verify? |
| **Edge cases** | Boundary conditions, overflow, empty inputs, NULL, OOM. |

2. **Follow Pão conventions** (refer to `AGENTS.md` and `new-module` skill):
   - `while` loops only
   - `static inline` on everything
   - `Status` return + `status_CHECK` propagation
   - Debug checks gated by `config_DEBUG`
   - Footnote comment system (`NOTE`, `SAFE`, `UNSAFE`)
   - Test structure: `Tester` array + `run_tests()`

3. **Identify any shared infrastructure** needed. For example, if multiple
   items need a helper function, design it once here.

4. **Stop and request review**.

### Example (for item 8: test `util_abs`)

```markdown
### Item 8 — Fix and test `util_absI32` / `util_absI64`

**Approach**:
- Change return type of `util_absI32` from `i32` to `u32`.
- Change return type of `util_absI64` from `i64` to `u64`.
- Cast to unsigned before negation: `return -(u32)a;`
- Grep for callers and update them to use `u32`/`u64`.

**Invariants**:
- `util_absI32(x) == util_absI32(-x)` for all x except `INT32_MIN`
  (where `-x` is UB, but `util_absI32(INT32_MIN)` must still work).
- Result is always ≥ 0 (trivially true since return type is unsigned).
- `util_absI32(0) == 0`.

**Correctness criteria**:
- No undefined behavior for any `i32` input, including `INT32_MIN`.
- The cast `-(u32)a` is well-defined because unsigned overflow wraps.

**Test strategy** (new file `tests/util_test.c`):
- `test_absI32_positive`: positive inputs return unchanged.
- `test_absI32_negative`: negative inputs return positive.
- `test_absI32_zero`: returns 0.
- `test_absI32_min`: `INT32_MIN` returns `(u32)2147483648`.
- Mirror tests for `absI64`.

**Edge cases**:
- `INT32_MIN` (the whole reason for the fix).
- `0`, `1`, `-1`.
```

---

## Phase 3 — Tasks

**Goal**: Convert the approved design into an ordered checklist of atomic work
items. Each task should be completable in one focused step.

### Process

1. **Order tasks by dependency**. Infrastructure and bug fixes before new
   features. Shared helpers before the code that uses them.
2. **Each task must specify**:
   - What file(s) to create or modify
   - What to do (in concrete terms, not vague)
   - What test(s) to write or update
   - How to verify (compile, run tests)
3. **Group tasks by component** when it makes sense.
4. **Write the task list** in `task.md`.
5. **Stop and request review**.

### Task format

```markdown
- [ ] **Task N**: [short title]
  - Files: `lib/foo.h`, `tests/foo_test.c`
  - Do: [concrete description]
  - Test: [what test to write]
  - Verify: `gcc -Wall -Wextra -Werror -pedantic -Wconversion -std=c99 -O2`
```

### Ordering principles

1. Bug fixes that unblock other work (e.g., fix UB before writing tests for it)
2. Infrastructure changes (e.g., `tests/common.h` fixes)
3. New test files for existing untested code
4. New features or implementations
5. Documentation updates

---

## Phase 4 — Implementation

**Goal**: Execute the task list, one task at a time.

### Process

1. **Pick the next unchecked task** from `task.md`.
2. **Mark it `[/]`** (in progress).
3. **Implement it**:
   - Follow all Pão conventions (see `AGENTS.md`).
   - Write the test *with* the implementation, not after.
   - Use the footnote comment system for non-obvious reasoning.
4. **Compile and run the specific test** to confirm it passes:
   ```bash
   gcc -Wall -Wextra -Werror -pedantic -Wconversion \
       -Wstrict-prototypes -std=c99 -O2 \
       tests/<path>/<file>_test.c -o /tmp/test && /tmp/test
   ```
5. **Mark the task `[x]`** in `task.md`.
6. **Move to the next task**. Do not batch — complete one before starting
   another.

### Rules during implementation

- **Never modify function signatures** without checking callers first (grep).
- **Never remove debug checks** — only add or improve them.
- **If you discover a new issue** while implementing, add it to `task.md` as a
  new task rather than fixing it ad-hoc. Note the discovery in your response.
- **If a task turns out to be wrong or impossible**, stop and report back to the
  user instead of improvising.

---

## Phase 5 — Verification

**Goal**: Prove that all changes are correct and nothing is broken.

### Process

1. **Run the full test suite**:
   ```bash
   cd tests && bash test
   ```
2. **Run the portable test matrix** (if changes affect library code):
   ```bash
   cd tests && bash testp
   ```
3. **Check for regressions**: every previously passing test must still pass.
4. **Review your own changes**:
   - Are all new functions `static inline`?
   - Are all new functions documented with docstrings?
   - Do all tests only test public API?
   - Are there any `for` loops, `goto`, or `continue`?
   - Are `const` qualifiers on all read-only pointer params?
   - Are out-parameters last?
   - Does every function with no args use `(void)`?
5. **Write the walkthrough** (`walkthrough.md`) summarizing:
   - What was changed
   - What was tested
   - Results of verification
6. **Stop and present to the user for final review**.

---

## Quick Reference — When to Use Each Phase

| Situation | Start at |
|---|---|
| User says "fix X" (single, clear bug) | Phase 2 (requirements are obvious) |
| User says "fix all TODOs related to Y" | Phase 1 (need to enumerate scope) |
| User says "add module Z" | Phase 1 (use `new-module` skill too) |
| User says "implement feature W" | Phase 1 |
| User approved requirements, asking to proceed | Phase 2 |
| User approved design, asking to proceed | Phase 3 |
| User approved tasks, asking to proceed | Phase 4 |

---

## Interplay with Other Skills

- **`known-issues`**: Always read in Phase 1 to find existing items.
- **`codebase-map`**: Always read in Phase 1 to understand module dependencies.
- **`new-module`**: Use its checklists in Phase 4 when creating new files.
- **`ovo-language`**: Reference when implementing Ovo-related features.
