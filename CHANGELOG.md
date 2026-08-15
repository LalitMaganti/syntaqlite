# Changelog

## 0.7.1

**Bug fixes:**
- Fixed the full C amalgamation failing to compile under `-DNDEBUG`. The inline parser-trace dispatch macro called `Synq<Dialect>ParseTrace`, which Lemon only defines in debug builds, so release builds hit an undeclared-identifier error. The macro now degrades to a no-op when `NDEBUG` is defined ([#305](https://github.com/LalitMaganti/syntaqlite/pull/305)).

## 0.7.0

**Breaking: WASM / JavaScript API rework:**
- The wasm ABI now uses session handles: `wasm_session_new` returns a handle and every stateful export takes it, replacing the previous thread-local global state. `Engine` in syntaqlite-js gains `dispose()` and can share a runtime instance with other Engines via `EngineConfig.runtime` ([#299](https://github.com/LalitMaganti/syntaqlite/pull/299)).
- LSP JSON-RPC is now the sole editor-feature surface in syntaqlite-wasm via `wasm_lsp_message`. The `wasm_diagnostics`, `wasm_semantic_tokens`, and `wasm_completions` exports are removed; schema context is set through the `syntaqlite/setSessionContext` extension request. `attachLspPort` gives worker embedders a one-line bridge ([#301](https://github.com/LalitMaganti/syntaqlite/pull/301)).
- One-shot ops are served over the CLI RPC protocol via a single `wasm_rpc` export, replacing `wasm_fmt` and `wasm_ast_json`. The JS API exposes typed `parse`, `format`, `tokenize`, and `analyze` methods; the old `run*` method names are gone and `keywordCase` now takes `"upper"` / `"lower"` ([#303](https://github.com/LalitMaganti/syntaqlite/pull/303)).

**Validation:**
- ALTER TABLE is now modeled in the semantic catalog: added, dropped, and renamed columns resolve correctly afterward, renamed tables are tracked, and altering a missing table is flagged ([#297](https://github.com/LalitMaganti/syntaqlite/pull/297)).
- Column references with a table qualifier that is not in scope are now flagged as "unknown table" with a fuzzy suggestion ([#292](https://github.com/LalitMaganti/syntaqlite/pull/292)).
- Fixed upsert (`ON CONFLICT DO UPDATE`) validation, which previously produced spurious failures ([#286](https://github.com/LalitMaganti/syntaqlite/pull/286)).
- CTEs are now registered as self-referential regardless of the `RECURSIVE` keyword, matching SQLite ([#293](https://github.com/LalitMaganti/syntaqlite/pull/293)).
- Fixed `ORDER BY` scoping inside `UNION` / `UNION ALL` compound selects ([#290](https://github.com/LalitMaganti/syntaqlite/pull/290)).
- FROM sources are captured before statement traversal, fixing resolution of tables that appear late in the AST ([#288](https://github.com/LalitMaganti/syntaqlite/pull/288)).
- Quoted identifiers are handled uniformly with correct escape processing, and single-quoted string aliases are now accepted ([#296](https://github.com/LalitMaganti/syntaqlite/pull/296), [#287](https://github.com/LalitMaganti/syntaqlite/pull/287)).

**Parser:**
- Fixed node extents for `EXPLAIN` / `EXPLAIN QUERY PLAN` statements and other edge cases ([#280](https://github.com/LalitMaganti/syntaqlite/pull/280), [#283](https://github.com/LalitMaganti/syntaqlite/pull/283)).

## 0.6.0

**Shell file support:**
- syntaqlite now understands SQLite shell scripts that mix `.dot` commands with SQL. The embedded SQL is extracted and the surrounding shell syntax is ignored, so `ast`, `fmt`, and `validate` all operate on just the SQL regions ([#275](https://github.com/LalitMaganti/syntaqlite/pull/275), [#276](https://github.com/LalitMaganti/syntaqlite/pull/276)).
- The LSP analyzes embedded SQL the same way, surfacing diagnostics on the SQL inside shell files directly in your editor ([#276](https://github.com/LalitMaganti/syntaqlite/pull/276)).

**Python / Pyodide:**
- syntaqlite can now run in the browser and other Pyodide environments via a Pyodide wheel ([#272](https://github.com/LalitMaganti/syntaqlite/pull/272)).
- The Python bindings now run the parser in-process instead of shelling out to a subprocess ([#271](https://github.com/LalitMaganti/syntaqlite/pull/271)).

**Performance:**
- Amalgamated builds are faster: dialect virtual functions are now inlined automatically ([#266](https://github.com/LalitMaganti/syntaqlite/pull/266)).

## 0.5.10

**Zed extension:**
- Renamed the extension and its language server id to `syntaqlite-lsp` and bundled a license, retooling it to meet the Zed extension registry's requirements ([#267](https://github.com/LalitMaganti/syntaqlite/pull/267)).

## 0.5.9

**Parser:**
- Reverted the eager macro argument expansion introduced in 0.5.8, which broke the rewrite tree shape expected by downstream consumers. The diamond-recursion case it fixed is now detected structurally by walking the expansion layer's parent chain through `body_call_offset == ARG_INTERNAL` links, with no pre-expansion needed ([#262](https://github.com/LalitMaganti/syntaqlite/pull/262)).

## 0.5.8

**Parser:**
- Macro arguments are now eagerly expanded at the call site before the body is instantiated. This fixes the "diamond" macro problem where an inner macro appeared to recurse through a parent even though no real cycle existed, and also speeds up macros that reference the same argument multiple times (the argument is expanded once instead of N times) ([#260](https://github.com/LalitMaganti/syntaqlite/pull/260)).

## 0.5.7

**Build:**
- Linux CLI binaries (`syntaqlite-linux-x64.tar.gz`, `syntaqlite-linux-arm64.tar.gz`) are now built inside `manylinux_2_28` containers, pinning them to glibc 2.28 (RHEL 8 / Ubuntu 20.04+ / Debian 10+) instead of inheriting `ubuntu-latest`'s glibc 2.39. Fixes `GLIBC_2.39 not found` when running the prebuilt CLI on older Linux hosts ([#258](https://github.com/LalitMaganti/syntaqlite/pull/258)).
- Added a Nix flake for building and running the CLI: `nix build`, `nix run`, and consumption via `overlays.default` are all supported ([#253](https://github.com/LalitMaganti/syntaqlite/pull/253)).

## 0.5.6

**Rust API:**
- Added `AnyParsedStatement::node_token_range(node_id)` returning the inclusive token range for any AST node. Ranges include expansion-layer tokens, so macro-produced nodes report meaningful ranges instead of collapsing to empty ([#248](https://github.com/LalitMaganti/syntaqlite/pull/248)).
- Added `AnyParsedStatement::node_leading_comments` / `node_trailing_comments` as composition sugar over `node_token_range` + `token_{leading,trailing}_comments` for the common boundary-comment case ([#248](https://github.com/LalitMaganti/syntaqlite/pull/248)).
- Token API is now layer-aware: offsets/lengths are typed as `LayerOffset` / `LayerLen`, `text()` resolves per-layer, and `stmt_range()` drills expansion tokens up to the authored call site in source coordinates. `layer_id()` is exposed on tokens ([#247](https://github.com/LalitMaganti/syntaqlite/pull/247)).
- `Comment::layer_id` distinguishes authored-source comments (layer 0) from comments inside macro expansion bodies (layer > 0); `Comment::text()` resolves against the owning layer's buffer ([#248](https://github.com/LalitMaganti/syntaqlite/pull/248)).
- Removed `AnyParsedStatement::token_spans()`: use `tokens().map(|t| t.stmt_range())` instead ([#247](https://github.com/LalitMaganti/syntaqlite/pull/247)).

**C API:**
- Added `syntaqlite_node_token_range(p, node_id, &first, &last)` for O(1) inclusive token ranges per AST node, and `syntaqlite_node_{leading,trailing}_comments` for boundary comments at the node level ([#248](https://github.com/LalitMaganti/syntaqlite/pull/248)).
- `SyntaqliteComment.layer_id` now distinguishes authored-source comments from comments inside macro expansion bodies ([#248](https://github.com/LalitMaganti/syntaqlite/pull/248)).

**Formatter:**
- New block-comment classification rule: a block comment is trailing on the preceding token only if it is same-line with that token AND nothing else follows on the rest of the line; otherwise it is leading on the next token. Previously, any block comment same-line with the preceding token was treated as trailing, conflating "annotates the prev token" with "sits between two tokens". Line comments are unchanged ([#248](https://github.com/LalitMaganti/syntaqlite/pull/248)).
- Fixed `/* c */ KEYWORD` losing its separator between the comment and the following keyword after the leading-comment drain cleared `pending` ([#248](https://github.com/LalitMaganti/syntaqlite/pull/248)).

## 0.5.5

**C API:**
- Public-API offsets and lengths are now typed with named aliases: `SyntaqliteDocOffset` / `SyntaqliteStmtOffset` / `SyntaqliteLayerOffset` make the coordinate system explicit, and `SyntaqliteLength` replaces the parallel `*Len` aliases. `SyntaqliteDiagnostic` offsets are now documented as document-absolute even when read via `statement_diagnostics` ([#244](https://github.com/LalitMaganti/syntaqlite/pull/244)).

**Build:**
- Amalgamated headers now also suppress `-Wzero-as-null-pointer-constant` under C++ to keep Lemon-generated parser code compiling cleanly when the amalgamation is consumed from C++ (notably on Windows toolchains) ([#245](https://github.com/LalitMaganti/syntaqlite/pull/245)).

## 0.5.4

**JavaScript package:**
- Fixed the `syntaqlite` npm package crashing with `window is not defined` under Node and Bun. The WASM runtime is now built as an Emscripten `MODULARIZE` factory and consumed via dynamic `import()` in Node/Bun (and an injected `<script>` in the browser), so `new Engine().load()` works on all three runtimes. The default SQLite preset also switched to an absolute `import.meta.url` URL so `loadDynamicLibrary` resolves in Node ([#242](https://github.com/LalitMaganti/syntaqlite/pull/242)).
- Added a Node smoke test (`npm run test:node`) that exercises the full `load → loadDefault → runFmt → runDiagnostics` path, wired into CI and the npm publish workflow ([#242](https://github.com/LalitMaganti/syntaqlite/pull/242)).

## 0.5.3

**Formatter:**
- Fixed leading comments followed by a blank line collapsing into the next element (e.g. a header comment block above `INCLUDE PERFETTO MODULE` was losing its trailing blank line) ([#238](https://github.com/LalitMaganti/syntaqlite/pull/238)).
- Fixed a formatter crash on statements where comments appeared inside a verbatim-emitted span such as a `CREATE PERFETTO MACRO` body ([#238](https://github.com/LalitMaganti/syntaqlite/pull/238)).
- Fixed a formatter panic on empty quoted tokens like `""` when preceded by a comment: `span_text` was collapsing any zero-length span to offset 0, so the comment-drain walk-back underflowed when computing the position of the opening quote ([#240](https://github.com/LalitMaganti/syntaqlite/pull/240)).
- Fallback (unresolved) macro calls now always format through the structured path, so multi-line calls get a single canonical layout and interior comments are preserved at every argument position ([#238](https://github.com/LalitMaganti/syntaqlite/pull/238)).
- Fallback macro-arg re-parses no longer silently strip parens around single-element `ColumnNameList`-shaped args (e.g. `_interval_intersect!(..., (utid))` preserves the paren so the one-element list expands correctly) ([#240](https://github.com/LalitMaganti/syntaqlite/pull/240)).

**Parser:**
- Fixed `x IN tbl` / `x IN tvf(args)` dropping the table or table-valued-function reference from the AST ([#238](https://github.com/LalitMaganti/syntaqlite/pull/238)).
- `TableRef` now distinguishes a bare table (`tbl`) from a zero-arg table-valued function call (`tvf()`) via a new `has_parens` field, so both forms round-trip through the formatter ([#238](https://github.com/LalitMaganti/syntaqlite/pull/238)).
- Added a `ParenExpr { expr: Expr }` AST node so that source parens around an expression survive as a real tree node. Previously the parser dropped them and the formatter re-derived parens from operator precedence, which worked for most expressions but couldn't recover author-written parens in macro-arg positions or around redundant scalar wraps like `(x)` ([#240](https://github.com/LalitMaganti/syntaqlite/pull/240)).

**Build:**
- Amalgamated headers now suppress `-Wimplicit-void-ptr-cast` and `-Wimplicit-int-enum-cast` under `__clang__`. These are clang-19 additions that fire under `-Weverything -Werror` when the header is pulled into a C++ translation unit; older clangs without these flags are covered by `-Wno-unknown-warning-option` as before ([#240](https://github.com/LalitMaganti/syntaqlite/pull/240)).

## 0.5.2

**Formatter:**
- Fixed a crash when a leading comment appeared inside parentheses that the parser preserves but no fmt opcode consumes (e.g. the redundant `(expr)` wrapping) ([#236](https://github.com/LalitMaganti/syntaqlite/pull/236)).
- Fixed a spurious blank line being inserted between a leading comment and the following element (e.g. a `CASE WHEN`) ([#236](https://github.com/LalitMaganti/syntaqlite/pull/236)).

**Analysis and validator:**
- Fixed `lineage` reporting CTE-body lineage instead of the outer SELECT for `WITH` statements ([#235](https://github.com/LalitMaganti/syntaqlite/pull/235)).

## 0.5.1

> **Note:** 0.5.0 was tagged but never released. Its release workflow failed to upload Python wheels and CLI binaries (stale `validate` subcommand reference in the smoke test, and the `linux_*` wheel platform tag is not accepted by PyPI). 0.5.1 bundles every change originally intended for 0.5.0 plus the pipeline fixes.

**Breaking:**
- Renamed the `semantic` module to `analysis` across the Rust, C, Python, and CLI surfaces. Rust `syntaqlite::semantic` → `syntaqlite::analysis`, C symbols drop their `syntaqlite_semantic_` prefix for `syntaqlite_analysis_`, and Python `syntaqlite.semantic` → `syntaqlite.analysis` ([#225](https://github.com/LalitMaganti/syntaqlite/pull/225)).
- Renamed `TableAccess` to `PhysicalTableAccess` and `StatementModel::tables_accessed()` to `physical_tables_accessed()`. The corresponding C symbols (`SyntaqliteTableAccess`, `syntaqlite_validator_table_count`, `syntaqlite_validator_tables`, and the per-statement variants) gain a `physical_` infix, and the Python `Lineage` attribute renames `tables` to `physical_tables`.
- Python bindings no longer link against `libsyntaqlite` directly. They now shell out to the `syntaqlite` CLI binary over JSON-RPC, so `pip install syntaqlite` pulls the CLI in as part of the package ([#196](https://github.com/LalitMaganti/syntaqlite/pull/196)).

**CLI:**
- Added `syntaqlite lineage [tables|columns] [FILES]`. Emits NDJSON (`schema_version: 0`, pre-stable) or human-readable text for column and table lineage, and exits 1 when any statement fails to parse or validate ([#163](https://github.com/LalitMaganti/syntaqlite/pull/163)).
- Added `syntaqlite tokenize` for dumping the token stream, and regrouped dialect-management commands under a dedicated `syntaqlite dialect ...` subcommand group ([#178](https://github.com/LalitMaganti/syntaqlite/pull/178)).
- Added `syntaqlite analyze --output json` for structured diagnostics ([#177](https://github.com/LalitMaganti/syntaqlite/pull/177)).

**Analysis and validator:**
- Added `StatementModel::unexpanded_views()`, exposing canonical names of views whose bodies were not available for expansion. Surfaces through C FFI as `SyntaqliteUnexpandedView` with aggregate and per-statement accessors, and in Python via `Lineage.unexpanded_views`.
- Added C FFI setters for per-check toggles, `strict-schema`, and the fuzzy suggestion threshold on the validator ([#180](https://github.com/LalitMaganti/syntaqlite/pull/180)). Those knobs are now sticky across schema reloads ([#181](https://github.com/LalitMaganti/syntaqlite/pull/181)).
- Added C FFI for registering custom scalar and table-valued functions against a dialect ([#185](https://github.com/LalitMaganti/syntaqlite/pull/185)).
- Diagnostics now carry a stable code (e.g. `unknown-table`) exposed via the C FFI and the Python `Diagnostic.code` attribute ([#184](https://github.com/LalitMaganti/syntaqlite/pull/184)).
- SQLite's double-quoted string (DQS) fallback is now applied when a double-quoted column reference cannot be resolved, matching real SQLite behavior ([#224](https://github.com/LalitMaganti/syntaqlite/pull/224)).

**C API:**
- Added the `SYNTAQLITE_OMIT_RUNTIME` build flag for compiling cdylib dialect plugins that reuse the host process's runtime symbols instead of bundling their own copy ([#174](https://github.com/LalitMaganti/syntaqlite/pull/174)).

**Macros and formatter:**
- Fixed the formatter dropping text that appeared after a macro invocation ([#172](https://github.com/LalitMaganti/syntaqlite/pull/172)).
- Fixed macro expansion when the replacement text is shorter than the original call site ([#173](https://github.com/LalitMaganti/syntaqlite/pull/173)).
- Fallback (unresolved) macro-call arguments are now formatted with structure rather than as an opaque blob ([#221](https://github.com/LalitMaganti/syntaqlite/pull/221)).
- `MacroRewrite` now exposes the call-site arguments for each expansion, not just the expanded body ([#227](https://github.com/LalitMaganti/syntaqlite/pull/227)).

**MCP server:**
- Tightened `keyword_case` argument validation and fixed the MCP server reporting the wrong dialect symbol ([#229](https://github.com/LalitMaganti/syntaqlite/pull/229)).

**Release pipeline:**
- Fixed the 0.5.0 release failure: smoke test now calls `syntaqlite analyze` instead of the renamed `validate`, and Linux wheels are built inside the `manylinux_2_28` container and validated with `auditwheel` so they publish to PyPI with a correct platform tag ([#233](https://github.com/LalitMaganti/syntaqlite/pull/233)).

## 0.5.0

*Never released. See the 0.5.1 notes above; everything intended for 0.5.0 ships there.*

## 0.4.2

- Fixed the web playground failing on load with `TypeError: resolved is not a function`: runtime → dialect hook symbols were being stripped under emscripten `MAIN_MODULE=2` ([#157](https://github.com/LalitMaganti/syntaqlite/pull/157)).
- Added `syntaqlite_dialect/dialect_abi.h` with the new `SYNTAQLITE_DIALECT_API` macro documenting and enforcing the runtime↔dialect symbol boundary; relevant for anyone building custom dialects as side modules ([#157](https://github.com/LalitMaganti/syntaqlite/pull/157)).

## 0.4.1

- Amalgamated C sources no longer trigger `-Werror=pragmas` when `#include`d from C++ translation units; C-only warning pragmas (`-Wdeclaration-after-statement`, `-Wmissing-prototypes`, `-Wold-style-declaration`) are now guarded by `#ifndef __cplusplus` ([#153](https://github.com/LalitMaganti/syntaqlite/pull/153)).

## 0.4.0

**Breaking:**
- Macro lookup is now callback-driven so dialects can resolve macros lazily ([#121](https://github.com/LalitMaganti/syntaqlite/pull/121)). The incremental parser API drops `begin_macro`/`end_macro` ([#110](https://github.com/LalitMaganti/syntaqlite/pull/110)).
- Spans now cover SQL longer than 64KB; `SyntaqliteTextSpan.length` is `uint32_t` ([#109](https://github.com/LalitMaganti/syntaqlite/pull/109)).
- `.synq` grammar files use `//` comments instead of `#` ([#115](https://github.com/LalitMaganti/syntaqlite/pull/115)).

**Macros:**
- Expansions are now exposed as a flat rewrite list with `$param` arg segments, nested call offsets, and APIs to ask whether a span/node/statement came from a macro, providing enough information to map cleanly between expanded SQL and source for diagnostics, hovers, and refactors ([#124](https://github.com/LalitMaganti/syntaqlite/pull/124), [#142](https://github.com/LalitMaganti/syntaqlite/pull/142), [#145](https://github.com/LalitMaganti/syntaqlite/pull/145)).
- Unknown macro names now produce a hard parse error instead of silently expanding to nothing ([#139](https://github.com/LalitMaganti/syntaqlite/pull/139)). A permissive mode lets tools inspect bodies with unknown `$param` references ([#141](https://github.com/LalitMaganti/syntaqlite/pull/141)).
- Embedders can compile macros out entirely with `SYNTAQLITE_OMIT_MACROS` ([#135](https://github.com/LalitMaganti/syntaqlite/pull/135)).
- Fixed OOB read on non-NUL-terminated bodies ([#132](https://github.com/LalitMaganti/syntaqlite/pull/132)), layer-id overflow past 256 layers ([#134](https://github.com/LalitMaganti/syntaqlite/pull/134)), and several spurious-straddle and whitespace-handling bugs in deeply-nested expansions.

**Spans and AST:**
- Every parse tree node carries a precise source range, with traceback through nested macro expansions so diagnostics report the original source location ([#96](https://github.com/LalitMaganti/syntaqlite/pull/96), [#101](https://github.com/LalitMaganti/syntaqlite/pull/101), [#102](https://github.com/LalitMaganti/syntaqlite/pull/102), [#103](https://github.com/LalitMaganti/syntaqlite/pull/103)). The Rust span API now mirrors C 1:1 ([#98](https://github.com/LalitMaganti/syntaqlite/pull/98)).
- Fixed stale span info on AST nodes from multi-RHS passthrough reductions ([#122](https://github.com/LalitMaganti/syntaqlite/pull/122), [#123](https://github.com/LalitMaganti/syntaqlite/pull/123)).

**Grammar and build:**
- SQL using `window`, `over`, or `filter` as identifiers now parses correctly ([#113](https://github.com/LalitMaganti/syntaqlite/pull/113)).
- Dialects can list base SQL keywords in `extra_keywords` without duplicate-keyword errors ([#111](https://github.com/LalitMaganti/syntaqlite/pull/111)).
- LSP semantic analysis is feature-gated behind `lsp` ([#97](https://github.com/LalitMaganti/syntaqlite/pull/97)).

## 0.3.1

- Restored Python 3.14 (cp314) wheel publishing, which had been silently dropped since 0.2.13 ([#91](https://github.com/LalitMaganti/syntaqlite/issues/91)).
- Added macro registration, module resolver, and DDL lineage tracking to the semantic model.
- Added per-statement C and Python API for diagnostics, lineage, and defined relations.
- Fixed panic when diagnostics spanned macro expansions ([#84](https://github.com/LalitMaganti/syntaqlite/issues/84)).
- Improved macro expansion span tracking to report full traceback through nested expansions.

## 0.3.0

- **Breaking:** Renamed `Grammar` to `Dialect` across C and Rust APIs (`SyntaqliteGrammar` → `SyntaqliteDialect`, `Grammar` → `Dialect`).
- Added dialect support to the Python API: `syntaqlite.Dialect` and `syntaqlite.load_dialect()` allow parsing and formatting with custom dialects.

## 0.2.17

- Fixed LSP position encoding to use UTF-16 code units instead of byte offsets, fixing misplaced diagnostics, hovers, and semantic tokens on lines with non-ASCII characters.
- Fixed LSP formatting response using `u32::MAX` for the TextEdit range end, exceeding the spec's `uinteger` upper bound of 2^31-1 ([#66](https://github.com/LalitMaganti/syntaqlite/issues/66)).
- Formatter now indents parenthesized content and subquery table sources when they break across lines.
- Fixed `indentWidth` having no effect when changed ([#62](https://github.com/LalitMaganti/syntaqlite/issues/62)).

## 0.2.16

- Fixed quoted identifiers (`"set"`, `` `set` ``, `[set]`) producing false-positive `unknown-table` and `unknown-column` diagnostics when DDL and query used different quoting styles ([#50](https://github.com/LalitMaganti/syntaqlite/issues/50)).
- Formatter now normalizes backtick and bracket quoting to standard double quotes.

## 0.2.15

- Added Zed editor extension for syntax highlighting and diagnostics.
- Added Zed getting-started documentation.
- Fixed Windows ARM64 Python wheel cross-compilation.
- Fixed amalgamation missing `tokens.h` after embedded-sources refactor.
- Fixed release workflow failing to upload artifacts to GitHub Release.
- Fixed Homebrew tap update using wrong token for cross-repo push.

## 0.2.14

- Skipped (broken release pipeline).

## 0.2.13

- Skipped (broken release pipeline).

## 0.2.12

- Fixed `cargo install syntaqlite-cli` failing due to `syntaqlite-buildtools` referencing files outside its crate directory ([#29](https://github.com/LalitMaganti/syntaqlite/issues/29))

## 0.2.11

- Added Python 3.14 wheel support.
- Fixed PyPI publishing (wheels were not uploaded for 0.2.1–0.2.9).

## 0.2.10

- Added Python 3.14 wheel support.
- Fixed PyPI publishing (wheels were not uploaded for 0.2.1–0.2.9).

## 0.2.9

- No user-visible changes (release pipeline consolidation).

## 0.2.8

- Fixed PyPI wheel smoke tests failing on non-Linux platforms by running each build on its native runner.

## 0.2.7

- **Breaking:** Renamed Python modules to public names: `syntaqlite._enums` → `syntaqlite.enums`, `syntaqlite._nodes` → `syntaqlite.nodes`.
- **Breaking:** `syntaqlite._parse_raw()` is now `syntaqlite.parse_raw()`.

## 0.2.6

- `parse()` now returns typed Python objects (`SelectStmt`, `BinaryExpr`, etc.) with attribute access, IDE autocomplete, and `isinstance` support. Raw dict access is still available via `syntaqlite._parse_raw()`.
- Added `python/.gitignore` and removed tracked build artifacts (`.so`, `.egg-info`).

## 0.2.5

- Fixed Python extension CI for x86_64 macOS by replacing deprecated `macos-13` runner with `macos-26-intel`.

## 0.2.4

- Fixed Windows Python extension linker errors (`__imp_` unresolved symbols) by making static linking the default for `SYNTAQLITE_API`.
- Added `SYNTAQLITE_API` annotations to all C function definitions, fixing MSVC dllimport/dllexport mismatch warnings.

## 0.2.3

- Fixed Windows Python extension build using the correct static library name for MSVC.

## 0.2.2

- Fixed Python extension build failing on Windows due to MSVC not supporting `_Static_assert` in C mode.

## 0.2.1

- Fixed PyPI wheel builds failing across all Python versions (3.10–3.13).

## 0.2.0

### Python library API

The `pip install syntaqlite` package now includes a native C extension with a full library API; previously, it only bundled the CLI binary. Four functions are available: `parse()`, `format_sql()`, `validate()`, and `tokenize()`.

- `validate()` returns a `ValidationResult` with `.diagnostics` and `.lineage` attributes.
- Schema can be provided via `Table`, `View` objects or raw DDL with `schema_ddl=`.
- `format_sql()` supports `line_width`, `indent_width`, `keyword_case`, and `semicolons` kwargs.
- `parse()` returns typed AST node dicts; `tokenize()` returns token dicts.

### Column lineage

- New lineage analysis for SELECT statements: traces each result column back to its source table and column, resolving through CTEs, subqueries, and aliases.
- `SemanticModel` gains `lineage()`, `relations_accessed()`, and `tables_accessed()` methods, returning `Complete` or `Partial` results depending on view resolution.
- C API: 7 new lineage accessor functions (`syntaqlite_validator_column_lineage`, `syntaqlite_validator_relations`, `syntaqlite_validator_tables`, etc.).

### Schema registration

- C API: new `syntaqlite_validator_add_views()` for registering views separately from tables.
- C API: new `syntaqlite_validator_load_schema_ddl()` to register schema from DDL strings (CREATE TABLE/VIEW).
- C API: `SyntaqliteTableDef` renamed to `SyntaqliteRelationDef` (used for both tables and views).

### Bug fixes

- Fix stack overflow in lineage resolver for recursive CTEs on Linux.
- Fix formatter macro handling to respect per-dialect macro style settings.

## 0.1.0

Initial release of syntaqlite: a fast, accurate SQL toolkit for SQLite, built from SQLite's own grammar.

### Highlights

- **Formatter**: opinionated SQL formatter with configurable line width, keyword casing, and semicolons. Supports stdin, files, and glob patterns.
- **Parser**: full SQLite SQL parser producing a concrete syntax tree. Handles all SQLite syntax including CTEs, window functions, upsert clauses, and `RETURNING`.
- **Validator**: semantic analysis with diagnostics for unknown tables, columns, and functions. Supports embedded SQL extraction from Python and TypeScript.
- **Language Server (LSP)**: diagnostics, formatting, completions, go-to-definition, document highlights, and semantic tokens over stdio.
- **C API**: prebuilt shared libraries for macOS, Linux, and Windows, plus a source amalgamation for embedding.
- **WASM / JS**: browser-ready builds powering the interactive playground.
- **Dialect extensibility**: load custom grammars as shared libraries at runtime.

### Project configuration

`syntaqlite.toml` is the single, editor-agnostic source of truth for schemas and formatting across VS Code, Claude Code, Neovim, Helix, and the CLI.

```toml
[schemas]
"src/**/*.sql" = ["schema/main.sql"]
"tests/**/*.sql" = ["schema/main.sql", "schema/test_fixtures.sql"]
"migrations/*.sql" = []

[format]
line-width = 100
keyword-case = "lower"
```

### Install

- CLI binaries for macOS (arm64, x86_64), Linux (arm64, x86_64), and Windows (x86_64, arm64)
- `pip install syntaqlite`: bundled platform-specific binary, includes built-in MCP server (`syntaqlite mcp`)
- `brew install LalitMaganti/tap/syntaqlite`
- `cargo install syntaqlite-cli`
- `mise use github:LalitMaganti/syntaqlite`
- VS Code extension with bundled LSP (VS Code Marketplace + Open VSX)
- Claude Code plugin via Marketplace
- Rust crates: `syntaqlite`, `syntaqlite-cli`, `syntaqlite-syntax`, `syntaqlite-common`
- NPM package: `syntaqlite`
