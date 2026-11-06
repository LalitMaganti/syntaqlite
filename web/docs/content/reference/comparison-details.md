+++
title = "Comparison Details"
weight = 10
+++
# Comparison: detailed results

Per-statement breakdowns, reproduction instructions, and methodology for the
[competitive comparison](@/reference/comparison.md).

Generated on `x86_64-linux` with syntaqlite `0.14.0` on 2026-11-06.

> To reproduce these results: `tools/run-comparison --setup && tools/run-comparison --all`.
> Every timed command is listed, one table per category, at the top of
> `python/dev/comparison/collect.py`. Tool versions are pinned by the lock files
> in `tests/comparison/` and by `tools/run-comparison --setup`.

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

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `syntaqlite` | 1.7 ± 3.1 | 1.0 | 31.4 | 1.09 ± 2.14 |
| `lemon-rs` | 1.5 ± 1.0 | 0.8 | 5.0 | 1.00 |
| `sql-parser-cst` | 105.0 ± 2.8 | 100.2 | 111.8 | 67.97 ± 43.21 |
| `sqlglot[c]` | 83.3 ± 1.5 | 81.6 | 87.3 | 53.93 ± 34.27 |
| `sqlparser-rs` | 3.0 ± 1.7 | 1.3 | 7.5 | 1.95 ± 1.65 |
| `node-sql-parser` | 90.2 ± 1.9 | 86.7 | 94.0 | 58.41 ± 37.12 |
| `sqlfluff` | 505.9 ± 6.2 | 496.1 | 516.2 | 327.45 ± 208.02 |

### bench_30x.sql (30×)

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `syntaqlite` | 3.4 ± 4.9 | 2.1 | 42.2 | 1.00 |
| `lemon-rs` | 5.0 ± 1.9 | 3.4 | 14.9 | 1.46 ± 2.20 |
| `sql-parser-cst` | 254.4 ± 14.5 | 238.5 | 275.3 | 74.95 ± 109.14 |
| `sqlglot[c]` | 232.9 ± 8.6 | 226.7 | 254.5 | 68.61 ± 99.86 |
| `sqlparser-rs` | 16.7 ± 0.4 | 15.7 | 17.7 | 4.91 ± 7.14 |
| `node-sql-parser` | 245.3 ± 14.6 | 234.0 | 280.6 | 72.24 ± 105.20 |
| `sqlfluff` | 8075.4 ± 29.9 | 8036.7 | 8125.4 | 2378.71 ± 3461.03 |

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

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `syntaqlite` | 2.1 ± 3.1 | 1.2 | 31.3 | 1.00 |
| `prettier-cst` | 168.8 ± 4.9 | 163.5 | 178.9 | 82.19 ± 124.81 |
| `sql-formatter` | 98.7 ± 3.0 | 94.8 | 104.0 | 48.07 ± 73.00 |
| `sqlglot[c]` | 89.6 ± 1.6 | 86.7 | 94.0 | 43.64 ± 66.26 |
| `sleek` | 2.9 ± 1.7 | 1.1 | 7.1 | 1.43 ± 2.33 |
| `sqruff` | 56.1 ± 0.9 | 54.5 | 58.6 | 27.31 ± 41.46 |

### bench_30x.sql (30×)

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `syntaqlite` | 7.4 ± 3.5 | 6.2 | 36.7 | 1.00 |
| `prettier-cst` | 485.8 ± 24.7 | 462.8 | 529.4 | 65.41 ± 31.15 |
| `sql-formatter` | 307.2 ± 6.8 | 297.3 | 315.2 | 41.37 ± 19.61 |
| `sqlglot[c]` | 339.2 ± 9.5 | 333.3 | 365.5 | 45.68 ± 21.67 |
| `sleek` | 17.8 ± 0.5 | 16.9 | 21.0 | 2.39 ± 1.13 |
| `sqruff` | 2301.5 ± 13.5 | 2280.7 | 2324.2 | 309.91 ± 146.76 |


### Slow tools (single timed run)

| Tool          |   Time |
| ------------- | -----: |
| sqlfmt (1x)   |  164ms |
| sqlfmt (30x)  | failed |
| sqlfluff (1x) | 1720ms |

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
  --> /tmp/tmpxh24_5pv.sql:30:3
   |
30 |   monthly_stats(month, revenue, order_count) AS (
   |   ^~~~~~~~~~~~~
warning: unknown function 'ROUDN'
  --> /tmp/tmpxh24_5pv.sql:42:3
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

### sqlite-runner-lsp

The LSP wraps sqlite3 at runtime and reports the same single error:

```
29:1 error Parse error: table monthly_stats has 2 values for 3 columns
```

### sql-lint

Structural checks only:

```
/tmp/tmp13bhrl7f.sql:1 sql-lint was unable to lint the following query "WITH  monthly_stats(month, revenue, order_count)...
```


## Per-case error detection

Schema: `users`, `orders`, `products`, `order_items`. Ground truth: sqlite3.

| Test                      | Expect | syntaqlite | sqlite3 | sqlite-runner-lsp | sql-lint |
| ------------------------- | :----: | :--------: | :-----: | :---------------: | :------: |
| keyword typo (SELEC)      | error  |   FOUND    |  FOUND  |       FOUND       |  FOUND   |
| missing close paren       | error  |   FOUND    |  FOUND  |       FOUND       |  FOUND   |
| double comma              | error  |   FOUND    |  FOUND  |       FOUND       |   MISS   |
| unterminated string       | error  |   FOUND    |  FOUND  |       FOUND       |   MISS   |
| trailing comma in VALUES  | error  |   FOUND    |  FOUND  |       FOUND       |  FOUND   |
| unknown table             | error  |   FOUND    |  FOUND  |       FOUND       |   MISS   |
| unknown table in JOIN     | error  |   FOUND    |  FOUND  |       FOUND       |   MISS   |
| unknown column            | error  |   FOUND    |  FOUND  |       FOUND       |   MISS   |
| unknown qualified column  | error  |   FOUND    |  FOUND  |       FOUND       |   MISS   |
| unknown column in SELECT  | error  |   FOUND    |  FOUND  |       FOUND       |   MISS   |
| SUBSTR: too few args      | error  |   FOUND    |  FOUND  |       FOUND       |   MISS   |
| REPLACE: too few args     | error  |   FOUND    |  FOUND  |       FOUND       |   MISS   |
| LENGTH: too many args     | error  |   FOUND    |  FOUND  |       FOUND       |   MISS   |
| COALESCE: zero args       | error  |    MISS    |  FOUND  |       FOUND       |   MISS   |
| CTE: 3 declared, 2 actual | error  |   FOUND    |  FOUND  |       FOUND       |  FOUND   |
| valid: simple SELECT      | valid  |     OK     |   OK    |        OK         |    OK    |
| valid: JOIN + aggregate   | valid  |     OK     |   OK    |        OK         |    OK    |
| valid: SUBSTR with 3 args | valid  |     OK     |   OK    |        OK         |    OK    |
| valid: COALESCE variadic  | valid  |     OK     |   OK    |        OK         |    OK    |
| valid: CTE columns match  | valid  |     OK     |   OK    |        OK         |    FP    |
| valid: built-in functions | valid  |     OK     |   OK    |        OK         |    OK    |
| valid: INSERT             | valid  |     OK     |   OK    |        OK         |    OK    |
| valid: UPDATE             | valid  |     OK     |   OK    |        OK         |    OK    |
| valid: DELETE with WHERE  | valid  |     OK     |   OK    |        OK         |    OK    |

### Scoreboard

| Tool              | Approach          | Correct                    | Missed | FP |
| ----------------- | ----------------- | -------------------------- | -----: | -: |
| sqlite3           | runtime execution | 24/24 ████████████████████ |      - |  - |
| sqlite-runner-lsp | runtime via LSP   | 24/24 ████████████████████ |      - |  - |
| syntaqlite        | static semantic   | 23/24 ███████████████████  |      1 |  - |
| sql-lint          | structural checks | 12/24 ██████████           |     11 |  1 |

## Speed details

- `bench.sql`: 117 lines, 3,545 bytes (+ schema preamble)
- `bench_30x.sql`: 3510 lines, 106,350 bytes (+ schema preamble)

### bench.sql (1×)

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `syntaqlite` | 4.3 ± 3.5 | 1.8 | 31.9 | 1.69 ± 1.56 |
| `sqlite3` | 2.6 ± 1.1 | 1.9 | 8.1 | 1.00 |
| `sqlite-runner-lsp` | 254.5 ± 12.9 | 239.4 | 275.6 | 98.93 ± 42.84 |
| `sql-lint` | 83.7 ± 1.5 | 81.7 | 87.8 | 32.55 ± 14.01 |

### bench_30x.sql (30×)

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `syntaqlite` | 9.7 ± 1.1 | 8.7 | 16.9 | 1.04 ± 0.16 |
| `sqlite3` | 9.3 ± 1.0 | 8.4 | 14.9 | 1.00 |
| `sqlite-runner-lsp` | 249.3 ± 9.8 | 239.5 | 273.9 | 26.82 ± 3.02 |
| `sql-lint` | 136.7 ± 8.3 | 130.3 | 165.8 | 14.72 ± 1.79 |

---

# LSP details

## Tested capabilities

Each server is started, sent a test file, and probed for completion, hover,
diagnostics, and formatting via the LSP protocol.

| Feature                |    syntaqlite   |     sqls ¹    | sql-language-server |
| ---------------------- | :-------------: | :-----------: | :-----------------: |
| Completion             | Yes (133 items) | Yes (6 items) |   Yes (11 items)    |
| Hover                  |       Yes       |      Yes      |         No          |
| Go to definition       |       Yes       |      Yes      |         No          |
| Find references        |       Yes       |      No       |         No          |
| Diagnostics: syntax    |       Yes       |      No       |         Yes         |
| Diagnostics: semantic  |       Yes       |      No       |         Yes         |
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
| syntaqlite          | 133 ████████████████████ |
| sql-language-server | 11 █                     |
| sqls ¹              | 6 █                      |

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

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `syntaqlite` | 24.3 ± 3.3 | 21.7 | 47.0 | 1.00 |
| `sql-language-server` | 222.8 ± 2.9 | 216.6 | 227.1 | 9.18 ± 1.25 |
| `sqls` | publishes no diagnostics | | | |
