# Token-preserving layout prototype

This is an isolated architecture experiment on branch `prototype-token-layout`, based on formatter PR #364 (`fe511add`). It is not wired into the production Formatter and has not been pushed. The original working checkout and PR stack are untouched.

## Mechanism

1. A generated Lemon shift callback identifies the original token by byte range. The prototype emits that exact text, including original spelling and optional syntax.
2. A reduction callback receives the generated grammar rule and RHS count. It combines the corresponding stack fragments directly into a document. Children stay in source order. Completed grammar nodes are not retained as a CST.
3. Comments occupy the gaps between those tokens. A gap travels with a fragment until its surrounding reduction can place it before the layout separator. Line comments force a break before subsequent SQL. The existing document renderer chooses flat/broken layout.

There is no AST traversal, `source(...)` annotation, output-keyword matching, node-range ownership search, or global comment-draining cursor in this path. The ordinary parser still constructs its AST; this experiment simply does not use it for formatting.

The prototype has explicit layout choices: punctuation spacing, break opportunities at list separators and SELECT clause boundaries, expression breaks, and nesting for expression/list productions. These use grammar-symbol identities. They do not contain SQL reproducer-specific fixes. They are deliberately incomplete style rules, not a finished formatter specification.

## Behavioral scope

All authored tokens, including `==`, `ASC`, optional `AS`, `NOT NULL`, and comma-style LIMIT, are retained in their original order and spelling. Keyword casing is preserved. The prototype does not normalize syntax or emit absent semicolons.

Comment policy is deliberately simple: comments stay between their original neighboring tokens, are placed backwards before a discretionary layout break, and line comments always terminate before SQL resumes. Original blank lines and own-line versus trailing-comment distinctions are not preserved. This avoids classification changing after a first formatting pass, but is a product behavior change that still needs review.

At width 40:

```sql
SELECT first_column, second_column,
  third_column
FROM some_table
WHERE first_column > 1
  AND second_column < 20
ORDER BY third_column ASC
LIMIT 20, 10;
```

`SELECT a == /* explanation */ b;` is preserved exactly. The previous failing NOT NULL and comma-LIMIT cases no longer require source-role mappings. The generated-column line-comment cases preserve the entire SQL token sequence and parse on a second pass.

## Coverage

- Focused historical inputs: 3,336 checks pass (1,668 corpus entries at widths 30 and 80).
- Broad historical inputs: 45,940 accepted checks pass. Another 42 checks are 21 invalid `LEFT BOGUS JOIN` inputs at both widths; the independent production parser rejects all 21 as well. Required SQLite compile flags are honored by the harness.
- The corpora overlap and contain duplicate inputs. Together, the passing checks represent **19,174 distinct SQL inputs / 38,348 distinct SQL-and-width pairs**, not 49,276 independent SQL programs.
- Every accepted check compares the complete lexer token sequence, including comments and exact spelling, with the input. It also reparses/reformats the output and requires an identical second pass.
- `cargo check` and strict Clippy pass with `lsp,dynload`; 328 library unit tests pass. The full pre-push run was not completed after its autofix stage reported the manually corrected lint findings.
- Two committed Rust tests exercise syntax retention and injected block/line comments at every token boundary of 16 seeds, including empty statements and the earlier crash categories.

These checks establish token preservation and stability for those inputs. They do not establish desirable layout in every context, full dialect/macro support, or agreement with existing formatting snapshots.

## Performance

Criterion release runs use the unchanged four benchmark inputs, reused formatter instances, 30 samples, 0.3 s warmup, and 2 s measurement. Two runs use opposite binary orders. Values below average the run means. Main is the `ec045e4e` snapshot; current PR is `fe511add`. No compilation or test suites ran during measurement.

| Fixture | Main | Current PR | Prototype | Prototype time vs main |
|---|---:|---:|---:|---:|
| Small SELECT | 2.416 µs | 2.372 µs | 2.782 µs | +15.1% |
| Multi-join SELECT | 17.115 µs | 17.454 µs | 21.387 µs | +25.0% |
| 500 statements | 1.999 ms | 2.014 ms | 2.001 ms | +0.1% |
| 500 commented statements | 2.242 ms | 2.424 ms | 2.103 ms | -6.2% |

These are not equivalent-feature comparisons: the prototype preserves syntax and casing and has a smaller layout policy. The commented fixture is about 13% faster than the current PR, but the small/medium fixtures are slower. This is promising evidence, not an overall performance win.

The first implementation was 22–67% slower than main. Reusing the trivia tokenizer and rendering buffers, and skipping whitespace-only gaps, removed avoidable allocation work. Raw initial and final measurements are retained in `/tmp/syntaqlite-token-layout-results`.

## Remaining engineering work

- Comments currently come from lexing gaps between shifted tokens, using the existing tokenizer. SQL ownership is not inferred from gap text. A proper lexer-event bridge could supply comments directly and remove this duplicate trivia lexing.
- Reduction callbacks use generated rule-name strings and a thread-local C-to-Rust bridge with a panic guard. Production code could use generated numeric layout dispatch. No stable API is proposed here.
- The grammar stack contains document fragments, not a full CST. However, the parser still allocates its AST and the prototype retains the document arena for the whole input, unlike the production formatter's per-statement rendering. **Peak memory has not been benchmarked; no memory improvement is claimed.**
- Long recursive lists/expressions, indentation, multiline literals/comments, blank lines, and statement spacing need deliberate layout work. There is no claim of full layout or worst-case renderer-performance coverage.
- Macro expansion and alternate dialects are outside the prototype's verified scope. Non-authored/non-monotone shifted spans produce an explicit error.
- Semicolon handling includes a provisional verbatim tail because the parser runtime can consume a terminator without a normal shift. A complete token-event interface should cover that directly.

## Reproduce

From `/tmp/syntaqlite-token-layout-prototype`:

```bash
cargo test -p syntaqlite --test token_layout_prototype
cargo build -p syntaqlite --example token_layout --features serde-json
python3 /tmp/syntaqlite-token-layout-results/check.py
python3 /tmp/syntaqlite-token-layout-results/broad_check.py
```

The `token_layout` example reads one JSON object per line (`sql`, optional `width` and `cflags`) and returns output, token-equality and shift/reduction counters. The external results directory holds input corpus snapshots, complete results, benchmark binaries, and scripts. The modified `tests/benches/benches/main.rs` selects the prototype for formatter benchmarks; it must not be merged as a production benchmark replacement.
