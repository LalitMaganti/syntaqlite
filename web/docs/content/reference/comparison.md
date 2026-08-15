+++
title = "Comparison to other tools"
weight = 9
+++
# How does syntaqlite compare?

There are many SQL tools out there. This page tests them head-to-head on
**SQLite-specific SQL**, the kind of syntax that trips up generic SQL parsers,
and shows raw, reproducible results.

> Generated on `arm64-darwin` with syntaqlite `0.1.0` on 2026-03-14.
> To reproduce: `tools/run-comparison --setup && tools/run-comparison --all`.
> Per-statement breakdowns and reproduction details are in the
> [detailed results](@/reference/comparison-details.md).

---

# Parsing

**What we test:** The corpus contains 40 statements covering advanced SQLite
syntax, including `UPSERT`, `RETURNING`, `STRICT` tables, window frames with
`EXCLUDE`, numeric underscores,
`IS NOT DISTINCT FROM`, recursive CTEs, and more. Each statement is first
validated against `sqlite3` itself (the ground truth), then run through every
parser. A tool scores "correct" only if it agrees with sqlite3.

## Accuracy

| Tool            | Correct                           | Rejects Valid | Accepts Invalid |
| --------------- | --------------------------------- | ------------: | --------------: |
| syntaqlite      | 40/40 (100%) ████████████████████ |             - |               - |
| lemon-rs        | 40/40 (100%) ████████████████████ |             - |               - |
| sql-parser-cst  | 39/40 (97%) ███████████████████   |             1 |               - |
| sqlglot[c]      | 35/40 (87%) █████████████████     |             5 |               - |
| sqlfluff        | 29/40 (72%) ██████████████        |            11 |               - |
| sqlparser-rs    | 26/40 (65%) █████████████         |            14 |               - |
| node-sql-parser | 15/40 (37%) ███████               |            25 |               - |

## Speed

We benchmark two file sizes: a 40-statement file (startup-dominated) and that
file repeated 30× (throughput-dominated).

### bench.sql (1×)

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `syntaqlite` | 1.7 ± 0.3 | 1.6 | 6.7 | 1.12 ± 0.46 |
| `lemon-rs` | 1.5 ± 0.6 | 1.3 | 20.3 | 1.00 |
| `sql-parser-cst` | 78.9 ± 7.4 | 74.5 | 115.2 | 51.81 ± 20.03 |
| `sqlglot[c]` | 86.2 ± 3.2 | 82.1 | 93.4 | 56.61 ± 21.33 |
| `sqlparser-rs` | 1.8 ± 0.2 | 1.6 | 3.8 | 1.18 ± 0.46 |
| `node-sql-parser` | 73.3 ± 1.1 | 71.3 | 76.0 | 48.14 ± 18.07 |
| `sqlfluff` | 445.7 ± 5.8 | 437.6 | 457.5 | 292.56 ± 109.79 |

### bench_30x.sql (30×)

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `syntaqlite` | 2.5 ± 0.2 | 2.4 | 5.3 | 1.00 |
| `lemon-rs` | 4.1 ± 0.1 | 4.0 | 5.4 | 1.62 ± 0.12 |
| `sql-parser-cst` | 141.8 ± 2.4 | 139.5 | 151.4 | 55.84 ± 4.05 |
| `sqlglot[c]` | 182.2 ± 3.9 | 179.4 | 195.2 | 71.76 ± 5.28 |
| `sqlparser-rs` | 11.0 ± 0.6 | 10.3 | 15.1 | 4.32 ± 0.39 |
| `node-sql-parser` | 149.7 ± 1.7 | 147.3 | 155.2 | 58.94 ± 4.20 |
| `sqlfluff` | 6382.9 ± 40.8 | 6333.2 | 6426.1 | 2513.49 ± 177.72 |

---

# Formatting

**What we test:** Round-trip semantic preservation. Each of the same 40
statements is formatted, then we run `EXPLAIN` on both the original and
formatted SQL and compare the bytecode sqlite3 produces. Identical bytecode
means sqlite3 will execute the exact same operations, so the formatter preserved
semantics, not just validity. Tools that crash or refuse to format score
"refused". Tools whose output produces different bytecode score "corrupt".

## Accuracy

| Tool          | Correct                           | Corrupt | Refused |
| ------------- | --------------------------------- | ------: | ------: |
| syntaqlite    | 40/40 (100%) ████████████████████ |       - |       - |
| prettier-cst  | 39/40 (97%) ███████████████████   |       - |       1 |
| sql-formatter | 39/40 (97%) ███████████████████   |       - |       1 |
| sleek         | 37/40 (92%) ██████████████████    |       3 |       - |
| sqruff        | 33/40 (82%) ████████████████      |       2 |       5 |
| sqlglot[c]    | 31/40 (77%) ███████████████       |       4 |       5 |

## Speed

### bench.sql (1×)

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `syntaqlite` | 1.8 ± 0.1 | 1.7 | 2.2 | 1.00 |
| `prettier-cst` | 404.8 ± 5.0 | 397.0 | 412.1 | 228.45 ± 7.74 |
| `sql-formatter` | 75.3 ± 1.3 | 73.1 | 79.5 | 42.50 ± 1.53 |
| `sqlglot[c]` | 87.4 ± 1.2 | 85.7 | 92.6 | 49.32 ± 1.70 |
| `sleek` | 8.4 ± 0.3 | 7.9 | 9.9 | 4.76 ± 0.22 |
| `sqruff` | 39.7 ± 0.6 | 38.5 | 42.0 | 22.41 ± 0.80 |

### bench_30x.sql (30×)

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `syntaqlite` | 4.9 ± 0.1 | 4.8 | 5.7 | 1.00 |
| `prettier-cst` | 558.1 ± 4.2 | 553.9 | 564.7 | 113.23 ± 2.82 |
| `sql-formatter` | 199.4 ± 2.8 | 195.7 | 204.6 | 40.45 ± 1.11 |
| `sqlglot[c]` | 264.5 ± 2.1 | 261.3 | 269.7 | 53.66 ± 1.34 |
| `sleek` | 27.2 ± 0.6 | 26.4 | 31.7 | 5.52 ± 0.18 |
| `sqruff` | 3347.0 ± 62.4 | 3262.9 | 3432.5 | 679.02 ± 20.51 |

---

# Analysis

**What we test:** We check whether each tool can identify unknown tables, bad
column references, incorrect function arity, and CTE column mismatches without
running the query. The corpus contains 24 cases against a known schema, with 15
intentional errors and 9 valid queries. Each tool's verdict is checked against
sqlite3.

## Accuracy

Schema: `users`, `orders`, `products`, `order_items`. Ground truth: sqlite3.

| Tool              | Approach          | Correct                    | Missed | FP |
| ----------------- | ----------------- | -------------------------- | -----: | -: |
| sqlite3           | runtime execution | 24/24 ████████████████████ |      - |  - |
| syntaqlite        | static semantic   | 23/24 ███████████████████  |      1 |  - |
| sql-lint          | structural checks | 12/24 ██████████           |     11 |  1 |
| sqlite-runner-lsp | runtime via LSP   | 9/24 ███████               |     15 |  - |

## Diagnostic quality

The test query contains two errors: its CTE declares three columns while its
`SELECT` produces two, and `ROUND` is misspelled as `ROUDN`:

| Tool              | Approach          | Errors Found | Finds All | Did-you-mean |
| ----------------- | ----------------- | :----------: | :-------: | :----------: |
| syntaqlite        | static semantic   |     2/2      |    Yes    |     Yes      |
| sqlite3           | runtime execution |     1/2      |    No     |      No      |
| sqlite-runner-lsp | runtime via LSP   |     0/2      |    No     |      No      |
| sql-lint          | structural checks |     0/2      |    No     |      No      |

**syntaqlite** finds both errors in one pass, with source locations and a
did-you-mean suggestion:

```
error: table 'monthly_stats' has 2 values for 3 columns
  --> /var/folders/rx/t6_rqmqx0f15l7kgp7yjhcbc0000gn/T/tmpico33e5u.sql:30:3
   |
30 |   monthly_stats(month, revenue, order_count) AS (
   |   ^~~~~~~~~~~~~
warning: unknown function 'ROUDN'
  --> /var/folders/rx/t6_rqmqx0f15l7kgp7yjhcbc0000gn/T/tmpico33e5u.sql:42:3
   |
42 |   ROUDN(ms.revenue / ms.order_count, 2) AS avg_order
   |   ^~~~~
   = help: did you mean 'round'?
```

**sqlite3** stops at the first error (runtime execution):

```
Error: in prepare, table monthly_stats has 2 values for 3 columns
```

## Speed

### bench.sql (1×)

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `syntaqlite` | 2.0 ± 0.1 | 1.9 | 3.3 | 1.00 |
| `sqlite3` | 4.7 ± 0.2 | 4.4 | 6.1 | 2.34 ± 0.17 |
| `sqlite-runner-lsp` | 10069.1 ± 7.5 | 10054.8 | 10075.0 | 4972.95 ± 270.92 |
| `sql-lint` | 356.5 ± 5.9 | 348.7 | 364.1 | 176.07 ± 10.03 |

### bench_30x.sql (30×)

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `syntaqlite` | 6.1 ± 0.2 | 5.8 | 8.2 | 1.00 |
| `sqlite3` | 19.8 ± 8.1 | 9.7 | 68.5 | 3.24 ± 1.34 |
| `sqlite-runner-lsp` | 10072.1 ± 4.2 | 10066.9 | 10078.6 | 1650.58 ± 62.91 |
| `sql-lint` | 378.1 ± 3.0 | 374.3 | 382.1 | 61.97 ± 2.41 |

---

# LSP (Language Server)

**What we test:** We start each language server, open a test document, and probe
for completions, hover, diagnostics, and formatting. Results are from actual LSP
protocol responses, not self-reported feature lists.

**Key difference:** `sqls` requires a live database connection for its features.
syntaqlite and sql-language-server work offline.

## Features

| Feature                |    syntaqlite   |     sqls ¹    | sql-language-server |
| ---------------------- | :-------------: | :-----------: | :-----------------: |
| Completion             | Yes (129 items) | Yes (6 items) |   Yes (11 items)    |
| Hover                  |       No        |      Yes      |         No          |
| Go to definition       |       Yes       |      Yes      |         No          |
| Find references        |       Yes       |      No       |         No          |
| Diagnostics: syntax    |       Yes       |      No       |         Yes         |
| Diagnostics: semantic  |       Yes       |      No       |   No (style only)   |
| Formatting             |       Yes       |      Yes      |         No          |
| Rename                 |       Yes       |      Yes      |         Yes         |
| Signature help         |       Yes       |      Yes      |         No          |
| Requires DB connection |       No        |      Yes      |         No          |

¹ sqls requires a live database connection. Completion and hover results come
from the connected database schema, not static analysis. Without a database,
these features return no results.

### Completion depth

| Tool                | Items                    |
| ------------------- | ------------------------ |
| syntaqlite          | 129 ████████████████████ |
| sql-language-server | 11 █                     |
| sqls ¹              | 6 █                      |

## Startup + response speed

Time from server start → document open → diagnostics received → exit:

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `syntaqlite` | 32.3 ± 0.9 | 30.3 | 34.7 | 1.00 |
| `sqls` | 10065.8 ± 2.8 | 10063.0 | 10069.3 | 311.28 ± 8.73 |
| `sql-language-server` | 470.5 ± 5.1 | 464.1 | 478.6 | 14.55 ± 0.44 |
