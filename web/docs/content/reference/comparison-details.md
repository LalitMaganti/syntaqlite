+++
title = "Comparison Details"
weight = 10
+++
# Comparison: detailed results

Per-statement breakdowns, reproduction instructions, and methodology for the
[competitive comparison](@/reference/comparison.md).

Generated on `x86_64-linux` with syntaqlite `0.14.0` on 2026-11-06.

> To reproduce these results: `tools/run-comparison --setup && tools/run-comparison --all`.
> See the script for requirements and tool versions.

---

# Parser details

## Ground truth

Every test statement validated against `sqlite3` (via `EXPLAIN`):

| Statement                                          | sqlite3 |
| -------------------------------------------------- | ------- |
| T01: Multi ON CONFLICT UPSERT + RETURNING          | OK      |
| T02: Recursive CTE + MATERIALIZED / NOT MATERIALIZ | OK      |
| T03: CREATE TABLE STRICT + WITHOUT ROWID + generat | OK      |
| T04: UPDATE FROM + INDEXED BY                      | OK      |
| T05: CREATE TRIGGER + RAISE + WHEN + FOR EACH ROW  | OK      |
| T06: FILTER clause + IIF + NULLS LAST              | OK      |
| T07: ATTACH DATABASE                               | OK      |
| T08: INSERT OR REPLACE                             | OK      |
| T09: CREATE VIRTUAL TABLE (FTS5)                   | OK      |
| T10: PRAGMA                                        | OK      |
| T11: EXPLAIN QUERY PLAN                            | OK      |
| T12: ALTER TABLE DROP COLUMN                       | OK      |
| T13: ALTER TABLE RENAME COLUMN                     | OK      |
| T14: REINDEX                                       | OK      |
| T15: Window frame RANGE BETWEEN                    | OK      |
| T16: CREATE INDEX with WHERE (partial index)       | OK      |
| T17: REPLACE statement                             | OK      |
| T18: Nested window functions + EXCLUDE             | OK      |
| T19: GLOB and LIKE with ESCAPE                     | OK      |
| T20: INSERT with multiple VALUES + ON CONFLICT DO  | OK      |
| T21: Complex subquery expressions                  | OK      |
| T22: ANALYZE                                       | OK      |
| T23: SAVEPOINT / RELEASE / ROLLBACK TO             | OK      |
| T24: DROP TABLE IF EXISTS                          | OK      |
| T25: CREATE TABLE AS SELECT                        | OK      |
| T26: DETACH DATABASE                               | OK      |
| T27: UPSERT with complex expressions in DO UPDATE  | OK      |
| T28: WITH (non-recursive) + DELETE ... RETURNING   | OK      |
| T29: UPDATE ... RETURNING                          | OK      |
| T30: RIGHT JOIN + IS DISTINCT FROM                 | OK      |
| T31: FULL OUTER JOIN                               | OK      |
| T32: JSON -> and ->> operators                     | OK      |
| T33: Numeric literals with underscores             | OK      |
| T34: Multiple WINDOW definitions + nth_value + nti | OK      |
| T35: HAVING without GROUP BY (3.39+)               | OK      |
| T36: IS NOT DISTINCT FROM in complex expression    | OK      |
| T37: Blob literals + CAST chains                   | OK      |
| T38: GENERATED ALWAYS AS (VIRTUAL vs STORED) + com | OK      |
| T39: Deeply nested CTE + compound SELECT (UNION /  | OK      |
| T40: Window GROUPS frame + EXCLUDE TIES            | OK      |

**40/40** statements validated by sqlite3.

## Per-statement results

Legend: **PASS** = correctly parses valid SQL, **FAIL** = rejects valid SQL, **FP** = accepts invalid SQL

| Test                                   | sqlite3 | syntaqlite | lemon-rs | sql-parser-cst | sqlglot[c] | sqlfluff | sqlparser-rs | node-sql-parser |
| -------------------------------------- | :-----: | :--------: | :------: | :------------: | :--------: | :------: | :----------: | :-------------: |
| T01: Multi ON CONFLICT UPSERT + RETURN |   OK    |    PASS    |   PASS   |      PASS      |    FAIL    |   FAIL   |     FAIL     |      FAIL       |
| T02: Recursive CTE + MATERIALIZED / NO |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   FAIL   |     FAIL     |      FAIL       |
| T03: CREATE TABLE STRICT + WITHOUT ROW |   OK    |    PASS    |   PASS   |      PASS      |    FAIL    |   PASS   |     FAIL     |      FAIL       |
| T04: UPDATE FROM + INDEXED BY          |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   FAIL   |     FAIL     |      FAIL       |
| T05: CREATE TRIGGER + RAISE + WHEN + F |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     FAIL     |      PASS       |
| T06: FILTER clause + IIF + NULLS LAST  |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     PASS     |      FAIL       |
| T07: ATTACH DATABASE                   |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   FAIL   |     PASS     |      PASS       |
| T08: INSERT OR REPLACE                 |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     PASS     |      PASS       |
| T09: CREATE VIRTUAL TABLE (FTS5)       |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   FAIL   |     FAIL     |      FAIL       |
| T10: PRAGMA                            |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     PASS     |      FAIL       |
| T11: EXPLAIN QUERY PLAN                |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   FAIL   |     PASS     |      FAIL       |
| T12: ALTER TABLE DROP COLUMN           |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     PASS     |      PASS       |
| T13: ALTER TABLE RENAME COLUMN         |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     PASS     |      PASS       |
| T14: REINDEX                           |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   FAIL   |     FAIL     |      FAIL       |
| T15: Window frame RANGE BETWEEN        |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     PASS     |      FAIL       |
| T16: CREATE INDEX with WHERE (partial  |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     PASS     |      PASS       |
| T17: REPLACE statement                 |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     PASS     |      PASS       |
| T18: Nested window functions + EXCLUDE |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     FAIL     |      FAIL       |
| T19: GLOB and LIKE with ESCAPE         |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     FAIL     |      FAIL       |
| T20: INSERT with multiple VALUES + ON  |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     PASS     |      FAIL       |
| T21: Complex subquery expressions      |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     PASS     |      PASS       |
| T22: ANALYZE                           |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   FAIL   |     FAIL     |      FAIL       |
| T23: SAVEPOINT / RELEASE / ROLLBACK TO |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   FAIL   |     PASS     |      FAIL       |
| T24: DROP TABLE IF EXISTS              |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     PASS     |      PASS       |
| T25: CREATE TABLE AS SELECT            |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     PASS     |      PASS       |
| T26: DETACH DATABASE                   |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   FAIL   |     FAIL     |      FAIL       |
| T27: UPSERT with complex expressions i |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     PASS     |      FAIL       |
| T28: WITH (non-recursive) + DELETE ... |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     FAIL     |      FAIL       |
| T29: UPDATE ... RETURNING              |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     PASS     |      PASS       |
| T30: RIGHT JOIN + IS DISTINCT FROM     |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     PASS     |      FAIL       |
| T31: FULL OUTER JOIN                   |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     PASS     |      FAIL       |
| T32: JSON -> and ->> operators         |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     PASS     |      PASS       |
| T33: Numeric literals with underscores |   OK    |    PASS    |   PASS   |      FAIL      |    FAIL    |   FAIL   |     FAIL     |      FAIL       |
| T34: Multiple WINDOW definitions + nth |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     PASS     |      FAIL       |
| T35: HAVING without GROUP BY (3.39+)   |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     PASS     |      PASS       |
| T36: IS NOT DISTINCT FROM in complex e |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     PASS     |      FAIL       |
| T37: Blob literals + CAST chains       |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     PASS     |      PASS       |
| T38: GENERATED ALWAYS AS (VIRTUAL vs S |   OK    |    PASS    |   PASS   |      PASS      |    FAIL    |   PASS   |     PASS     |      PASS       |
| T39: Deeply nested CTE + compound SELE |   OK    |    PASS    |   PASS   |      PASS      |    PASS    |   PASS   |     PASS     |      FAIL       |
| T40: Window GROUPS frame + EXCLUDE TIE |   OK    |    PASS    |   PASS   |      PASS      |    FAIL    |   PASS   |     FAIL     |      FAIL       |

### Scoreboard

| Tool            | Correct                           | Rejects Valid | Accepts Invalid |
| --------------- | --------------------------------- | ------------: | --------------: |
| syntaqlite      | 40/40 (100%) ████████████████████ |             - |               - |
| lemon-rs        | 40/40 (100%) ████████████████████ |             - |               - |
| sql-parser-cst  | 39/40 (97%) ███████████████████   |             1 |               - |
| sqlglot[c]      | 35/40 (87%) █████████████████     |             5 |               - |
| sqlfluff        | 29/40 (72%) ██████████████        |            11 |               - |
| sqlparser-rs    | 26/40 (65%) █████████████         |            14 |               - |
| node-sql-parser | 15/40 (37%) ███████               |            25 |               - |

## Speed details

- `bench.sql`: 117 lines, 3,545 bytes
- `bench_30x.sql`: 3510 lines, 106,350 bytes

### bench.sql (1×)

| Command | Mean Wall Time [ms] | Change | Factor |
|:---|---:|---:|:---|
| `syntaqlite` | 4.1 ± 2.0 |  |  |
| `lemon-rs` | 2.2 ± 1.2 | -47.1% | (1.9x faster) |
| `sql-parser-cst` | 104.2 ± 2.6 | +2459.0% | (25.6x slower) |
| `sqlglot[c]` | 84.0 ± 1.2 | +1964.2% | (20.6x slower) |
| `sqlparser-rs` | 3.0 ± 1.8 | -25.2% |  |
| `node-sql-parser` | 89.9 ± 2.4 | +2108.5% | (22.1x slower) |
| `sqlfluff` | 505.8 ± 13.1 | +12324.7% | (124.2x slower) |

### bench_30x.sql (30×)

| Command | Mean Wall Time [ms] | Change | Factor |
|:---|---:|---:|:---|
| `syntaqlite` | 4.1 ± 2.1 |  |  |
| `lemon-rs` | 5.9 ± 2.5 | +46.3% | (1.5x slower) |
| `sql-parser-cst` | 251.6 ± 19.2 | +6099.8% | (62.0x slower) |
| `sqlglot[c]` | 231.5 ± 9.1 | +5606.1% | (57.1x slower) |
| `sqlparser-rs` | 16.6 ± 0.3 | +309.3% | (4.1x slower) |
| `node-sql-parser` | 240.0 ± 6.7 | +5815.7% | (59.2x slower) |
| `sqlfluff` | 8069.8 ± 49.8 | +198785.9% | (1988.9x slower) |

---

# Formatter details

## Ground truth

| Statement                                          | sqlite3 |
| -------------------------------------------------- | ------- |
| T01: Multi ON CONFLICT UPSERT + RETURNING          | OK      |
| T02: Recursive CTE + MATERIALIZED / NOT MATERIALIZ | OK      |
| T03: CREATE TABLE STRICT + WITHOUT ROWID + generat | OK      |
| T04: UPDATE FROM + INDEXED BY                      | OK      |
| T05: CREATE TRIGGER + RAISE + WHEN + FOR EACH ROW  | OK      |
| T06: FILTER clause + IIF + NULLS LAST              | OK      |
| T07: ATTACH DATABASE                               | OK      |
| T08: INSERT OR REPLACE                             | OK      |
| T09: CREATE VIRTUAL TABLE (FTS5)                   | OK      |
| T10: PRAGMA                                        | OK      |
| T11: EXPLAIN QUERY PLAN                            | OK      |
| T12: ALTER TABLE DROP COLUMN                       | OK      |
| T13: ALTER TABLE RENAME COLUMN                     | OK      |
| T14: REINDEX                                       | OK      |
| T15: Window frame RANGE BETWEEN                    | OK      |
| T16: CREATE INDEX with WHERE (partial index)       | OK      |
| T17: REPLACE statement                             | OK      |
| T18: Nested window functions + EXCLUDE             | OK      |
| T19: GLOB and LIKE with ESCAPE                     | OK      |
| T20: INSERT with multiple VALUES + ON CONFLICT DO  | OK      |
| T21: Complex subquery expressions                  | OK      |
| T22: ANALYZE                                       | OK      |
| T23: SAVEPOINT / RELEASE / ROLLBACK TO             | OK      |
| T24: DROP TABLE IF EXISTS                          | OK      |
| T25: CREATE TABLE AS SELECT                        | OK      |
| T26: DETACH DATABASE                               | OK      |
| T27: UPSERT with complex expressions in DO UPDATE  | OK      |
| T28: WITH (non-recursive) + DELETE ... RETURNING   | OK      |
| T29: UPDATE ... RETURNING                          | OK      |
| T30: RIGHT JOIN + IS DISTINCT FROM                 | OK      |
| T31: FULL OUTER JOIN                               | OK      |
| T32: JSON -> and ->> operators                     | OK      |
| T33: Numeric literals with underscores             | OK      |
| T34: Multiple WINDOW definitions + nth_value + nti | OK      |
| T35: HAVING without GROUP BY (3.39+)               | OK      |
| T36: IS NOT DISTINCT FROM in complex expression    | OK      |
| T37: Blob literals + CAST chains                   | OK      |
| T38: GENERATED ALWAYS AS (VIRTUAL vs STORED) + com | OK      |
| T39: Deeply nested CTE + compound SELECT (UNION /  | OK      |
| T40: Window GROUPS frame + EXCLUDE TIES            | OK      |

**40/40** statements validated by sqlite3.

## Round-trip validation

For each formatter: does the formatted output produce identical `EXPLAIN`
bytecode to the original? This verifies semantic preservation, not just
syntactic validity. "CORRUPT" means the bytecode differs or `EXPLAIN` fails.

| Test                                   | syntaqlite | prettier-cst | sql-formatter | sqlglot[c] |  sleek  | sqruff |
| -------------------------------------- | :--------: | :----------: | :-----------: | :--------: | :-----: | :----: |
| T01: Multi ON CONFLICT UPSERT + RETURN |     OK     |      OK      |      OK       |    FAIL    |   OK    |  FAIL  |
| T02: Recursive CTE + MATERIALIZED / NO |     OK     |      OK      |      OK       |  CORRUPT   |   OK    |  FAIL  |
| T03: CREATE TABLE STRICT + WITHOUT ROW |     OK     |      OK      |      OK       |    FAIL    |   OK    |   OK   |
| T04: UPDATE FROM + INDEXED BY          |     OK     |      OK      |      OK       |     OK     |   OK    |  FAIL  |
| T05: CREATE TRIGGER + RAISE + WHEN + F |     OK     |      OK      |      OK       |  CORRUPT   |   OK    |   OK   |
| T06: FILTER clause + IIF + NULLS LAST  |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T07: ATTACH DATABASE                   |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T08: INSERT OR REPLACE                 |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T09: CREATE VIRTUAL TABLE (FTS5)       |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T10: PRAGMA                            |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T11: EXPLAIN QUERY PLAN                |     OK     |      OK      |      OK       |     OK     |   OK    |  FAIL  |
| T12: ALTER TABLE DROP COLUMN           |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T13: ALTER TABLE RENAME COLUMN         |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T14: REINDEX                           |     OK     |      OK      |      OK       |  CORRUPT   |   OK    |   OK   |
| T15: Window frame RANGE BETWEEN        |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T16: CREATE INDEX with WHERE (partial  |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T17: REPLACE statement                 |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T18: Nested window functions + EXCLUDE |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T19: GLOB and LIKE with ESCAPE         |     OK     |      OK      |      OK       |     OK     | CORRUPT |  FAIL  |
| T20: INSERT with multiple VALUES + ON  |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T21: Complex subquery expressions      |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T22: ANALYZE                           |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T23: SAVEPOINT / RELEASE / ROLLBACK TO |     OK     |      OK      |      OK       |  CORRUPT   |   OK    |  FAIL  |
| T24: DROP TABLE IF EXISTS              |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T25: CREATE TABLE AS SELECT            |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T26: DETACH DATABASE                   |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T27: UPSERT with complex expressions i |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T28: WITH (non-recursive) + DELETE ... |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T29: UPDATE ... RETURNING              |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T30: RIGHT JOIN + IS DISTINCT FROM     |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T31: FULL OUTER JOIN                   |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T32: JSON -> and ->> operators         |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T33: Numeric literals with underscores |     OK     |     FAIL     |     FAIL      |    FAIL    | CORRUPT |  FAIL  |
| T34: Multiple WINDOW definitions + nth |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T35: HAVING without GROUP BY (3.39+)   |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T36: IS NOT DISTINCT FROM in complex e |     OK     |      OK      |      OK       |     OK     |   OK    |   OK   |
| T37: Blob literals + CAST chains       |     OK     |      OK      |      OK       |     OK     | CORRUPT |   OK   |
| T38: GENERATED ALWAYS AS (VIRTUAL vs S |     OK     |      OK      |      OK       |    FAIL    |   OK    |   OK   |
| T39: Deeply nested CTE + compound SELE |     OK     |      OK      |      OK       |     OK     |   OK    |  FAIL  |
| T40: Window GROUPS frame + EXCLUDE TIE |     OK     |      OK      |      OK       |    FAIL    |   OK    |   OK   |

### Scoreboard

| Tool          | Correct                           | Corrupt | Refused |
| ------------- | --------------------------------- | ------: | ------: |
| syntaqlite    | 40/40 (100%) ████████████████████ |       - |       - |
| prettier-cst  | 39/40 (97%) ███████████████████   |       - |       1 |
| sql-formatter | 39/40 (97%) ███████████████████   |       - |       1 |
| sleek         | 37/40 (92%) ██████████████████    |       3 |       - |
| sqruff        | 32/40 (80%) ████████████████      |       - |       8 |
| sqlglot[c]    | 31/40 (77%) ███████████████       |       4 |       5 |

### Corruption details

| Tool       | Test                                                 | Error                                                        |
| ---------- | ---------------------------------------------------- | ------------------------------------------------------------ |
| sqlglot[c] | T02: Recursive CTE + MATERIALIZED / NOT MATERIALIZED | EXPLAIN failed on formatted SQL                              |
| sqlglot[c] | T05: CREATE TRIGGER + RAISE + WHEN + FOR EACH ROW    | Error: in prepare, near "SELECT": syntax error
  yees', OLD. |
| sqlglot[c] | T14: REINDEX                                         | Error: in prepare, near "AS": syntax error
  REINDEX AS idx_ |
| sleek      | T19: GLOB and LIKE with ESCAPE                       | EXPLAIN bytecode differs from original                       |
| sqlglot[c] | T23: SAVEPOINT / RELEASE / ROLLBACK TO               | Error: in prepare, near "AS": syntax error
  SAVEPOINT AS my |
| sleek      | T33: Numeric literals with underscores               | EXPLAIN failed on formatted SQL                              |
| sleek      | T37: Blob literals + CAST chains                     | EXPLAIN failed on formatted SQL                              |

## Speed details

- `bench.sql`: 117 lines, 3,545 bytes
- `bench_30x.sql`: 3510 lines, 106,350 bytes

### bench.sql (1×)

| Command | Mean Wall Time [ms] | Change | Factor |
|:---|---:|---:|:---|
| `syntaqlite` | 2.7 ± 4.1 |  |  |
| `prettier-cst` | 174.2 ± 8.0 | +6440.8% | (65.4x slower) |
| `sql-formatter` | 98.8 ± 2.6 | +3610.6% | (37.1x slower) |
| `sqlglot[c]` | 88.2 ± 2.1 | +3209.3% | (33.1x slower) |
| `sleek` | 16.5 ± 7.1 | +521.0% | (6.2x slower) |
| `sqruff` | 72.7 ± 2.5 | +2629.0% | (27.3x slower) |

### bench_30x.sql (30×)

| Command | Mean Wall Time [ms] | Change | Factor |
|:---|---:|---:|:---|
| `syntaqlite` | 8.0 ± 1.9 |  |  |
| `prettier-cst` | 485.8 ± 18.3 | +5965.4% | (60.7x slower) |
| `sql-formatter` | 305.7 ± 9.4 | +3716.4% | (38.2x slower) |
| `sqlglot[c]` | 340.5 ± 8.7 | +4150.9% | (42.5x slower) |
| `sleek` | 24.6 ± 7.0 | +207.3% | (3.1x slower) |
| `sqruff` | 2404.1 ± 23.0 | +29913.6% | (300.1x slower) |


### Slow tools (single timed run)

| Tool          |   Time |
| ------------- | -----: |
| sqlfmt (1x)   |  160ms |
| sqlfmt (30x)  | failed |
| sqlfluff (1x) | 1914ms |

---

# Validator details

## Diagnostic quality showcase

A realistic query with 2 subtle errors. How does each tool report them?

**Query** (CTE declares 3 columns but SELECT produces 2; typo `ROUDN`):

```sql
WITH
  monthly_stats(month, revenue, order_count) AS (
    SELECT
      STRFTIME('%Y-%m', o.created_at) AS month,
      SUM(o.total) AS revenue
    FROM orders o
    WHERE o.status = 'completed'
    GROUP BY STRFTIME('%Y-%m', o.created_at)
  )
SELECT
  ms.month,
  ms.revenue,
  ms.order_count,
  ROUDN(ms.revenue / ms.order_count, 2) AS avg_order
FROM monthly_stats ms
ORDER BY ms.month DESC
LIMIT 12;
```

### syntaqlite

Static analysis runs offline and reports **both** errors in one pass:

```
error: table 'monthly_stats' has 2 values for 3 columns
  --> /tmp/tmp14kfwjx1.sql:30:3
   |
30 |   monthly_stats(month, revenue, order_count) AS (
   |   ^~~~~~~~~~~~~
warning: unknown function 'ROUDN'
  --> /tmp/tmp14kfwjx1.sql:42:3
   |
42 |   ROUDN(ms.revenue / ms.order_count, 2) AS avg_order
   |   ^~~~~
   = help: did you mean 'round'?
note: no schema provided; unresolved names are reported as warnings. Add a `syntaqlite.toml` with `schema = ["schema.sql"]` or pass `--schema` to treat them as errors.
```

### sqlite3

Runtime execution stops at the first error:

```
Error: in prepare, table monthly_stats has 2 values for 3 columns
```

### sql-lint

Structural checks only:

```
/tmp/tmp6hjs71wv.sql:1 sql-lint was unable to lint the following query "WITH  monthly_stats(month, revenue, order_count)...
```


## Per-case error detection

Schema: `users`, `orders`, `products`, `order_items`. Ground truth: sqlite3.

| Test                      | Expect | syntaqlite | sqlite3 | sql-lint |
| ------------------------- | :----: | :--------: | :-----: | :------: |
| keyword typo (SELEC)      | error  |   FOUND    |  FOUND  |  FOUND   |
| missing close paren       | error  |   FOUND    |  FOUND  |  FOUND   |
| double comma              | error  |   FOUND    |  FOUND  |   MISS   |
| unterminated string       | error  |   FOUND    |  FOUND  |   MISS   |
| trailing comma in VALUES  | error  |   FOUND    |  FOUND  |  FOUND   |
| unknown table             | error  |   FOUND    |  FOUND  |   MISS   |
| unknown table in JOIN     | error  |   FOUND    |  FOUND  |   MISS   |
| unknown column            | error  |   FOUND    |  FOUND  |   MISS   |
| unknown qualified column  | error  |   FOUND    |  FOUND  |   MISS   |
| unknown column in SELECT  | error  |   FOUND    |  FOUND  |   MISS   |
| SUBSTR: too few args      | error  |   FOUND    |  FOUND  |   MISS   |
| REPLACE: too few args     | error  |   FOUND    |  FOUND  |   MISS   |
| LENGTH: too many args     | error  |   FOUND    |  FOUND  |   MISS   |
| COALESCE: zero args       | error  |    MISS    |  FOUND  |   MISS   |
| CTE: 3 declared, 2 actual | error  |   FOUND    |  FOUND  |  FOUND   |
| valid: simple SELECT      | valid  |     OK     |   OK    |    OK    |
| valid: JOIN + aggregate   | valid  |     OK     |   OK    |    OK    |
| valid: SUBSTR with 3 args | valid  |     OK     |   OK    |    OK    |
| valid: COALESCE variadic  | valid  |     OK     |   OK    |    OK    |
| valid: CTE columns match  | valid  |     OK     |   OK    |    FP    |
| valid: built-in functions | valid  |     OK     |   OK    |    OK    |
| valid: INSERT             | valid  |     OK     |   OK    |    OK    |
| valid: UPDATE             | valid  |     OK     |   OK    |    OK    |
| valid: DELETE with WHERE  | valid  |     OK     |   OK    |    OK    |

### Scoreboard

| Tool       | Approach          | Correct                    | Missed | FP |
| ---------- | ----------------- | -------------------------- | -----: | -: |
| sqlite3    | runtime execution | 24/24 ████████████████████ |      - |  - |
| syntaqlite | static semantic   | 23/24 ███████████████████  |      1 |  - |
| sql-lint   | structural checks | 12/24 ██████████           |     11 |  1 |

## Speed details

- `bench.sql`: 117 lines, 3,545 bytes (+ schema preamble)
- `bench_30x.sql`: 3510 lines, 106,350 bytes (+ schema preamble)

### bench.sql (1×)

| Command | Mean Wall Time [ms] | Change | Factor |
|:---|---:|---:|:---|
| `syntaqlite` | 3.4 ± 4.6 |  |  |
| `sqlite3` | 6.2 ± 3.5 | +82.9% | (1.8x slower) |
| `sql-lint` | 86.5 ± 3.1 | +2444.2% | (25.4x slower) |

### bench_30x.sql (30×)

| Command | Mean Wall Time [ms] | Change | Factor |
|:---|---:|---:|:---|
| `syntaqlite` | 9.9 ± 1.6 |  |  |
| `sqlite3` | 11.0 ± 1.5 | +11.8% |  |
| `sql-lint` | 135.0 ± 3.6 | +1267.8% | (13.7x slower) |

---

# LSP details

## Tested capabilities

Each server is started, sent a test file, and probed for completion, hover,
diagnostics, and formatting via the LSP protocol.

| Feature                |    syntaqlite   |        sqls ¹        | sql-language-server |
| ---------------------- | :-------------: | :------------------: | :-----------------: |
| Completion             | Yes (133 items) | Advertised (0 items) |   Yes (11 items)    |
| Hover                  |       Yes       |          No          |         No          |
| Go to definition       |       Yes       |         Yes          |         No          |
| Find references        |       Yes       |          No          |         No          |
| Diagnostics: syntax    |       Yes       |          No          |         Yes         |
| Diagnostics: semantic  |       Yes       |          No          |         Yes         |
| Formatting             |       Yes       |         Yes          |         No          |
| Rename                 |       Yes       |         Yes          |         Yes         |
| Signature help         |       Yes       |         Yes          |         No          |
| Requires DB connection |       No        |         Yes          |         No          |

¹ sqls requires a live database connection. Completion and hover results come
from the connected database schema, not static analysis. Without a database,
these features return no results.

### Completion depth

| Tool                | Items                    |
| ------------------- | ------------------------ |
| syntaqlite          | 133 ████████████████████ |
| sql-language-server | 11 █                     |

## Diagnostic detail

What each server reports for `SELEC * FROM users;` (syntax error):

### syntaqlite

```
1:1 error syntax error near 'SELEC'
2:15 error unknown table 'nonexistent_table'
```

### sqls

```
(no diagnostics)
```

### sql-language-server

```
1:2 error Expected "$", "(", "--", "/*", "ALTER", "CREATE TABLE", "CREATE", "DELETE", "DROP TABLE", "DROP VIEW", "DROP", "INSERT", "REPLACE", "SELECT", "UPDATE", "WITH", "return", [ \t\n\r], or end of input but "S" found.
```


## Speed details

Time to start server, send document, receive diagnostics, and exit:

| Command | Mean Wall Time [ms] | Change | Factor |
|:---|---:|---:|:---|
| `syntaqlite` | 25.8 ± 3.6 |  |  |
| `sql-language-server` | 231.0 ± 13.5 | +796.1% | (9.0x slower) |
