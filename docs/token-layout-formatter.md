# Token-preserving production formatter

`Formatter::format`, the CLI, LSP formatting, and the C formatter entry point use the same parser-driven layout implementation. `dump_doc_tree` exposes the document that this implementation renders. The experimental formatter and private implementation-copying harness have been removed. The public Rust API is unchanged. Against main (`ec045e4e`), production changes total **1,459 added/deleted lines**, including the generated parser and excluding test modules, test infrastructure, fixtures and documentation.

## How it works

1. A scoped observer is installed for one synchronous parser call. Lemon reports shifted token spans and reductions through the parse context, so generated dialects use the same mechanism. Ordinary parsing has no observer and builds no layout documents.
2. Shifted tokens become document placeholders. Reductions combine the corresponding stack fragments in source order. Lists and equal-precedence operator chains stay open until their owner establishes grouping and indentation; no CST is retained.
3. At statement completion, existing parser token flags distinguish keywords from identifiers, function names, and type names. Keyword placeholders receive the configured casing. Statement terminators follow `FormatConfig`; internal trigger terminators remain intact.
4. Comments stay between their original neighboring tokens, before discretionary layout breaks. A line comment defers its mandatory newline to the receiving boundary, so closing delimiters get their enclosure's indentation. Blank lines around comments become one paragraph break.
5. The existing Wadler/Lindig renderer chooses line breaks using the configured width and indentation. SQL-specific whitespace policy is in `token_layout_rules.rs`; document composition and parser integration are in `token_layout.rs`.

Authored syntax is preserved: explicit ASC, implicit aliases, operator spellings, identifier quoting, parentheses, and comma-style LIMIT are not normalized away. This intentionally changes previous formatting snapshots. Type declarations, virtual-table arguments and wildcard grammar captures preserve their opaque source spans, since those strings can be interpreted by another language or extension.

Normal statement layout now follows grammar policy rather than `.synq` AST-formatting bytecode. The existing bytecode remains relevant to the macro helper and bytecode debug command. Custom dialects must be regenerated with the current parser template. A dialect that produces no layout events fails explicitly rather than returning incomplete SQL. Macro calls retain the existing structured-argument and verbatim-reindent helpers, including their existing argument-normalization behavior. The token-preservation corpus covers SQLite statements, not normalization inside these macro helpers. The bytecode subtree interpreter remains for those structured arguments and the bytecode debug command; ordinary statements no longer traverse the AST for layout or drain comments through that interpreter.

## Validation

The tests exercise the production API, rather than an alternate implementation:

- 24 reviewed layout fixtures check exact whitespace, width, token preservation and second-pass stability.
- Additional Rust tests cover comments at every token boundary of 16 seeds, sequences up to 1,024 elements, configuration, internal versus outer semicolons, error locations, reuse after errors, and concurrent formatter instances.
- Every SQLite and Perfetto formatter snapshot now requires an identical second formatting pass. Snapshot updates preserve all original inputs, options and test bodies.
- All eleven former parentheses-related opt-outs are re-enabled. The semantic suite compares parsed ASTs and available SQLite EXPLAIN bytecode. It treats keyword NULL casing as lexical metadata and fails if formatting valid input returns an error.
- The external historical corpus contains 38,348 distinct valid SQL/width pairs. Its oracle checks all tokens and comments, allowing configured keyword casing and terminator normalization, plus successful reparsing and idempotency. This is not a visual layout oracle.
- The snapshot updater uses Python AST locations to replace only the intended literal. Regression tests cover indented multiline strings, Unicode, escaping, neighboring tests and failure without modifying the file.

## Costs and limits

The normal parser still builds its AST. Layout retains its document arena for the whole input and currently lexes trivia gaps with the existing tokenizer. No peak-memory improvement is claimed. The parser observer adds two pointers per parse context, and generated grammar-name tables remain available in release builds.

Width is a soft target for indivisible tokens and opaque source spans. Parser recovery takes priority over observer errors so formatting diagnostics retain their original source locations.

### Measurements

Release Criterion measurements compare this production implementation with main at `ec045e4e`, using the unchanged fixtures and reused formatter instances. Two runs use opposite binary order, with 30 samples, 0.3 s warmup and 2 s measurement. No builds or test suites ran during measurement.

| Fixture | Main | Production | Time ratio |
|---|---:|---:|---:|
| large | 1.993 ms | 2.817 ms | 1.41× |
| large_commented | 2.284 ms | 2.864 ms | 1.25× |
| medium | 17.183 µs | 26.418 µs | 1.54× |
| small | 2.287 µs | 3.772 µs | 1.65× |

This is a 25–65% performance regression, not a speed improvement. The formatting policy and retained syntax differ from main.

| Sequence | 256 elements | 512 elements | 1,024 elements | Doubling ratios |
|---|---:|---:|---:|---:|
| projection | 134.4 µs | 269.0 µs | 540.3 µs | 2.00×, 2.01× |
| boolean | 192.3 µs | 387.8 µs | 772.4 µs | 2.02×, 1.99× |
| arithmetic | 85.7 µs | 169.3 µs | 332.0 µs | 1.98×, 1.96× |

These sizes show approximately linear scaling; they do not establish a worst-case bound for every grammar shape.

## Reproduce

```bash
CC=clang CXX=clang++ tools/pre-push
cargo test -p syntaqlite --features lsp,dynload --test token_layout
python3 -m unittest python.dev.diff_tests.runner_test
tools/run-integration-tests --suite sql-idempotency
cargo bench -p benches --bench main -- '^formatter/'
cargo bench -p benches --bench main -- '^token_layout_scaling/'
```

The `token_layout` example is a JSONL verification harness around the production `Formatter`. It contains no alternate formatting implementation. The regular formatter benchmarks also call the production API.
