---
name: debug-formatter
description: Debug a SQL formatter bug. Use when the user reports incorrect formatting, such as wrong whitespace, misplaced comments, or blank lines.
user_invocable: true
---

# debug-formatter

Debug a SQL formatter bug using red-green testing and pipeline tracing.

## Instructions

### 1. Reproduce (RED)

Add a failing test to `syntaqlite/tests/fmt_comments.rs` (comment bugs) or the
relevant `tests/fmt_diff_tests/*.py` file. Always `eprintln!` the actual output:

```rust
#[test]
fn descriptive_name() {
    let input = "...";
    let out = fmt(input);
    eprintln!("=== actual ===\n{out}=== end ===");
    assert_eq!(out, "...\n");
}
```

Run it and confirm RED:
```sh
cargo test -p syntaqlite --features fmt,sqlite --test fmt_comments TEST_NAME -- --nocapture
```

### 2. Trace the pipeline

Work through these layers to find the root cause:

| Layer | File | What to check |
|-------|------|---------------|
| Grammar | `syntaqlite-buildtools/parser-nodes/*.synq` | `fmt { }` block; `clause("KW", field)` expands to `IF_SET → LINE → KEYWORD → NEST(LINE, CHILD)` |
| Bytecode compiler | `syntaqlite-buildtools/src/dialect_codegen/fmt_compiler.rs` | How `Fmt::Clause`, `Fmt::Keyword` compile to opcodes |
| Interpreter | `syntaqlite/src/fmt/interpret.rs` | Main loop in `interpret_node`; state = `running` (committed doc) + `pending` (deferred line breaks) |
| Comment drain | `syntaqlite/src/fmt/comment.rs` | `drain_before(offset)` respects `has_non_comment_text` guard; `drain_keyword_interior()` skips it for multi-word keywords |
| flush_drain | `interpret.rs` | Integrates `DrainResult { trailing, leading }` into running/pending; when `leading != NIL`, pending is dropped |
| Doc renderer | `syntaqlite/src/fmt/doc.rs` | Wadler-style; `hardline` always breaks, `line` breaks if group broken |

### 3. Common bug patterns

- **Blank line between comments**: Two consecutive drains both produce `hardline + comment + hardline`; back-to-back hardlines = blank line. Fix via `continuation` flag on `drain_keyword_interior`.
- **Comment on wrong side of keyword**: `flush_drain` for interior comments runs before keyword text is emitted. Check flush vs keyword append ordering in the `Keyword` match arm.
- **Comment lost**: `has_non_comment_text` guard stops `drain_before` when a keyword sits between the comment and target offset. Multi-word interiors need `skip_text_check: true`.
- **Pending dropped**: `flush_drain` with `leading != NIL` zeroes pending. If pending held a LINE from the clause template, that break is lost.

### 4. Fix and verify (GREEN)

```sh
cargo test -p syntaqlite --features fmt,sqlite --test fmt_comments TEST_NAME -- --nocapture
cargo test -p syntaqlite --features fmt,sqlite
cargo build -p syntaqlite-cli && tools/run-integration-tests
```

All 7 integration suites must pass (including ~650 idempotency checks).
