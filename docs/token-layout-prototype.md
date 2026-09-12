# Token-preserving layout prototype

This is an isolated architecture experiment on branch `prototype-grammar-token-layout`, based directly on main (`ec045e4e`). It replaces the direction explored in PRs #363 and #364 without including their parser attachment API or formatter changes. It is not wired into the production Formatter. The prototype is compiled only by private test, example, and benchmark harnesses; it adds no Rust library API.

## Mechanism

1. A generated Lemon shift callback identifies the original token by byte range. The prototype emits that exact text, including original spelling and optional syntax.
2. A reduction callback receives the generated grammar rule and RHS count. It combines the corresponding stack fragments directly into a document. Children stay in source order. Completed grammar nodes are not retained as a CST.
3. Comments occupy the gaps between those tokens. A gap travels with a fragment until its surrounding reduction can place it before the layout separator. Line comments force a break before subsequent SQL. The existing document renderer chooses flat/broken layout.

There is no AST traversal, `source(...)` annotation, output-keyword matching, node-range ownership search, or global comment-draining cursor in this path. The ordinary parser still constructs its AST; this experiment simply does not use it for formatting.

Layout policy is isolated in `syntaqlite/src/fmt/token_layout_rules.rs`. It uses grammar symbols to select shared document operations: sequences, hanging headers/bodies, independent suffix groups, and enclosures. The document interpreter is unchanged.

Recursive lists and same-precedence operator chains accumulate as open sequences. A clause, item, or enclosure establishes their group and indentation once. Grammar wrappers add no groups. This prevents each recursive reduction from adding indentation and repeated fit checks. Different-precedence chains retain their structural grouping.

The policy covers SELECT clauses and compounds, joins, aliases, function arguments and window clauses, CASE and BETWEEN, assignments and RETURNING, column definitions and constraints, declaration headers, and trigger bodies. These are explicit SQL style decisions, not generic punctuation heuristics or reproducer-specific text matches. This remains a prototype, not a complete formatter specification.

## Behavioral scope

All authored tokens, including `==`, `ASC`, optional `AS`, `NOT NULL`, and comma-style LIMIT, are retained in their original order and spelling. Keyword casing is preserved. The prototype does not normalize syntax or emit absent semicolons.

Comment policy is deliberately simple: comments stay between their original neighboring tokens, are placed backwards before a discretionary layout break, and line comments always terminate before SQL resumes. Blank lines around comments are retained as one paragraph break; whitespace-only gaps between SQL tokens are normalized. Leading comments occupy a header line. Other own-line versus trailing-comment distinctions are not retained. A line comment carries a pending mandatory break until the receiving boundary places it: a closing parenthesis therefore returns to its enclosure indentation. Separate statements receive a blank line. These are explicit style choices that still need product review.

At width 40:

```sql
SELECT
  first_column,
  second_column,
  third_column
FROM some_table
WHERE
  first_column > 1
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
- 328 library unit tests and six prototype tests pass. The prototype tests include injected comments at every token boundary of 16 seeds, 24 reviewed exact-layout fixtures, comment-boundary indentation, and recursive lists/chains of 64, 256 and 1,024 elements. The reviewed fixtures require exact output, width compliance, token preservation and second-pass stability.
- The standalone replacement branch passes `CC=clang CXX=clang++ tools/pre-push`: formatting, strict Clippy, dead-code checks, C checks, unchanged public API snapshots, workspace tests, AST/formatter/amalgamation/Perfetto/grammar integration suites, and the web build. Prototype types are compiled privately in their harnesses.
- The committed reviewed fixtures are in `syntaqlite/tests/fixtures/token_layout_cases.rs`. The broader historical corpus and before/after gallery are local development artifacts, not part of this checkout.

These checks establish token preservation and stability for those inputs. They do not establish desirable layout in every context, full dialect/macro support, or agreement with existing formatting snapshots.

## Performance

Criterion release runs use the unchanged four benchmark inputs, reused formatter instances, 30 samples, 0.3 s warmup, and 2 s measurement. Two runs use opposite binary orders. These historical measurements were collected before rebasing the prototype directly onto main; they are not timings of the replacement PR branch. Values below average the run means. Main is the `ec045e4e` snapshot; before is the initial prototype `bf8fa706`. No compilation or test suites ran during measurement.

| Fixture | Main | Before layout fixes | After layout fixes | After vs main | After vs before |
|---|---:|---:|---:|---:|---:|
| Small SELECT | 2.359 µs | 2.810 µs | 3.232 µs | +37.0% | +15.0% |
| Multi-join SELECT | 17.489 µs | 21.656 µs | 24.400 µs | +39.5% | +12.7% |
| 500 statements | 2.042 ms | 2.032 ms | 2.668 ms | +30.7% | +31.3% |
| 500 commented statements | 2.308 ms | 2.119 ms | 2.772 ms | +20.1% | +30.8% |

The fuller layout policy regresses performance by 13–31% against the initial prototype on these fixtures. It is 20–40% slower than the main snapshot. These are not equivalent-feature comparisons: the prototype preserves original syntax and casing, and now produces different documents and layouts. This is a layout improvement, not a performance win. Raw measurements and binaries are under `/tmp/syntaqlite-token-layout-results/layout`.

Long-sequence benchmarks use width 40 and the same Criterion sampling parameters (one run):

| Sequence | 256 elements | 512 elements | 1,024 elements | Doubling ratios |
|---|---:|---:|---:|---:|
| projection | 125.4 µs | 260.4 µs | 503.2 µs | 2.08×, 1.93× |
| boolean | 184.3 µs | 352.4 µs | 708.8 µs | 1.91×, 2.01× |
| arithmetic | 81.2 µs | 159.6 µs | 317.5 µs | 1.97×, 1.99× |

These measurements are consistent with approximately linear growth for these lists and chains; they are not a proof for every grammar shape or deeply nested input. Peak memory remains unmeasured.

## Remaining engineering work

- Comments currently come from lexing gaps between shifted tokens, using the existing tokenizer. SQL ownership is not inferred from gap text. A proper lexer-event bridge could supply comments directly and remove this duplicate trivia lexing.
- Reduction callbacks use generated rule-name strings and a thread-local C-to-Rust bridge with a panic guard. Production code could use generated numeric layout dispatch. No stable API is proposed here.
- The grammar stack contains document fragments, not a full CST. However, the parser still allocates its AST and the prototype retains the document arena for the whole input, unlike the production formatter's per-statement rendering. **Peak memory has not been benchmarked; no memory improvement is claimed.**
- The reviewed fixtures exercise layout, but token-preservation sweeps are not a visual oracle. Unreviewed grammar combinations may still have undesirable whitespace. Width is a soft target: indivisible tokens and authored multiline literals/comments can exceed it. There is no claim of exhaustive layout or worst-case renderer-performance coverage.
- Macro expansion and alternate dialects are outside the prototype's verified scope. Non-authored/non-monotone shifted spans produce an explicit error.
- The parser runtime can consume a terminator without a normal shift. The final gap is tokenized and emitted through the same trivia/boundary mechanism; a complete token-event interface should cover these terminators directly.

## Reproduce

From the repository root:

```bash
cargo test -p syntaqlite --test token_layout_prototype
cargo build -p syntaqlite --example token_layout --features serde-json
cargo bench -p benches --bench main -- '^formatter/'
cargo bench -p benches --bench main -- '^token_layout_scaling/'
```

The `token_layout` example reads one JSON object per line (`sql`, optional `width` and `cflags`) and returns output, token-equality and shift/reduction counters. The external results directory holds input corpus snapshots, complete results, benchmark binaries, and scripts. The modified `tests/benches/benches/main.rs` selects the prototype for formatter benchmarks; it must not be merged as a production benchmark replacement.
