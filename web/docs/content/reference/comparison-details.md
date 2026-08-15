+++
title = "Comparison Details"
weight = 10
+++
# Comparison: detailed results

Per-statement breakdowns, reproduction instructions, and methodology for the
[competitive comparison](@/reference/comparison.md).

Generated on `arm64-darwin` with syntaqlite `0.1.0` on 2026-03-14.

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

| Test                                   | syntaqlite | prettier-cst | sql-formatter | sqlglot[c] |  sleek  |  sqruff |
| -------------------------------------- | :--------: | :----------: | :-----------: | :--------: | :-----: | :-----: |
| T01: Multi ON CONFLICT UPSERT + RETURN |     OK     |      OK      |      OK       |    FAIL    |   OK    |   OK    |
| T02: Recursive CTE + MATERIALIZED / NO |     OK     |      OK      |      OK       |  CORRUPT   |   OK    |   OK    |
| T03: CREATE TABLE STRICT + WITHOUT ROW |     OK     |      OK      |      OK       |    FAIL    |   OK    |   OK    |
| T04: UPDATE FROM + INDEXED BY          |     OK     |      OK      |      OK       |     OK     |   OK    |   OK    |
| T05: CREATE TRIGGER + RAISE + WHEN + F |     OK     |      OK      |      OK       |  CORRUPT   |   OK    |   OK    |
| T06: FILTER clause + IIF + NULLS LAST  |     OK     |      OK      |      OK       |     OK     |   OK    |  FAIL   |
| T07: ATTACH DATABASE                   |     OK     |      OK      |      OK       |     OK     |   OK    |   OK    |
| T08: INSERT OR REPLACE                 |     OK     |      OK      |      OK       |     OK     |   OK    |   OK    |
| T09: CREATE VIRTUAL TABLE (FTS5)       |     OK     |      OK      |      OK       |     OK     |   OK    |   OK    |
| T10: PRAGMA                            |     OK     |      OK      |      OK       |     OK     |   OK    |   OK    |
| T11: EXPLAIN QUERY PLAN                |     OK     |      OK      |      OK       |     OK     |   OK    |   OK    |
| T12: ALTER TABLE DROP COLUMN           |     OK     |      OK      |      OK       |     OK     |   OK    |   OK    |
| T13: ALTER TABLE RENAME COLUMN         |     OK     |      OK      |      OK       |     OK     |   OK    |   OK    |
| T14: REINDEX                           |     OK     |      OK      |      OK       |  CORRUPT   |   OK    |   OK    |
| T15: Window frame RANGE BETWEEN        |     OK     |      OK      |      OK       |     OK     |   OK    |  FAIL   |
| T16: CREATE INDEX with WHERE (partial  |     OK     |      OK      |      OK       |     OK     |   OK    |   OK    |
| T17: REPLACE statement                 |     OK     |      OK      |      OK       |     OK     |   OK    |   OK    |
| T18: Nested window functions + EXCLUDE |     OK     |      OK      |      OK       |     OK     |   OK    |  FAIL   |
| T19: GLOB and LIKE with ESCAPE         |     OK     |      OK      |      OK       |     OK     | CORRUPT |   OK    |
| T20: INSERT with multiple VALUES + ON  |     OK     |      OK      |      OK       |     OK     |   OK    |   OK    |
| T21: Complex subquery expressions      |     OK     |      OK      |      OK       |     OK     |   OK    |   OK    |
| T22: ANALYZE                           |     OK     |      OK      |      OK       |     OK     |   OK    |   OK    |
| T23: SAVEPOINT / RELEASE / ROLLBACK TO |     OK     |      OK      |      OK       |  CORRUPT   |   OK    |   OK    |
| T24: DROP TABLE IF EXISTS              |     OK     |      OK      |      OK       |     OK     |   OK    |   OK    |
| T25: CREATE TABLE AS SELECT            |     OK     |      OK      |      OK       |     OK     |   OK    |   OK    |
| T26: DETACH DATABASE                   |     OK     |      OK      |      OK       |     OK     |   OK    |   OK    |
| T27: UPSERT with complex expressions i |     OK     |      OK      |      OK       |     OK     |   OK    |   OK    |
| T28: WITH (non-recursive) + DELETE ... |     OK     |      OK      |      OK       |     OK     |   OK    |   OK    |
| T29: UPDATE ... RETURNING              |     OK     |      OK      |      OK       |     OK     |   OK    |   OK    |
| T30: RIGHT JOIN + IS DISTINCT FROM     |     OK     |      OK      |      OK       |     OK     |   OK    |   OK    |
| T31: FULL OUTER JOIN                   |     OK     |      OK      |      OK       |     OK     |   OK    |   OK    |
| T32: JSON -> and ->> operators         |     OK     |      OK      |      OK       |     OK     |   OK    | CORRUPT |
| T33: Numeric literals with underscores |     OK     |     FAIL     |     FAIL      |    FAIL    | CORRUPT |   OK    |
| T34: Multiple WINDOW definitions + nth |     OK     |      OK      |      OK       |     OK     |   OK    |  FAIL   |
| T35: HAVING without GROUP BY (3.39+)   |     OK     |      OK      |      OK       |     OK     |   OK    |   OK    |
| T36: IS NOT DISTINCT FROM in complex e |     OK     |      OK      |      OK       |     OK     |   OK    |   OK    |
| T37: Blob literals + CAST chains       |     OK     |      OK      |      OK       |     OK     | CORRUPT |   OK    |
| T38: GENERATED ALWAYS AS (VIRTUAL vs S |     OK     |      OK      |      OK       |    FAIL    |   OK    | CORRUPT |
| T39: Deeply nested CTE + compound SELE |     OK     |      OK      |      OK       |     OK     |   OK    |  FAIL   |
| T40: Window GROUPS frame + EXCLUDE TIE |     OK     |      OK      |      OK       |    FAIL    |   OK    |   OK    |

### Scoreboard

| Tool          | Correct                           | Corrupt | Refused |
| ------------- | --------------------------------- | ------: | ------: |
| syntaqlite    | 40/40 (100%) ████████████████████ |       - |       - |
| prettier-cst  | 39/40 (97%) ███████████████████   |       - |       1 |
| sql-formatter | 39/40 (97%) ███████████████████   |       - |       1 |
| sleek         | 37/40 (92%) ██████████████████    |       3 |       - |
| sqruff        | 33/40 (82%) ████████████████      |       2 |       5 |
| sqlglot[c]    | 31/40 (77%) ███████████████       |       4 |       5 |

### Corruption details

| Tool       | Test                                                               | Error                                                        |
| ---------- | ------------------------------------------------------------------ | ------------------------------------------------------------ |
| sqlglot[c] | T02: Recursive CTE + MATERIALIZED / NOT MATERIALIZED               | EXPLAIN failed on formatted SQL                              |
| sqlglot[c] | T05: CREATE TRIGGER + RAISE + WHEN + FOR EACH ROW                  | Error: in prepare, near "SELECT": syntax error
  yees', OLD. |
| sqlglot[c] | T14: REINDEX                                                       | Error: in prepare, near "AS": syntax error
  REINDEX AS idx_ |
| sleek      | T19: GLOB and LIKE with ESCAPE                                     | EXPLAIN bytecode differs from original                       |
| sqlglot[c] | T23: SAVEPOINT / RELEASE / ROLLBACK TO                             | Error: in prepare, near "AS": syntax error
  SAVEPOINT AS my |
| sqruff     | T32: JSON -> and ->> operators                                     | EXPLAIN failed on formatted SQL                              |
| sleek      | T33: Numeric literals with underscores                             | EXPLAIN failed on formatted SQL                              |
| sleek      | T37: Blob literals + CAST chains                                   | EXPLAIN failed on formatted SQL                              |
| sqruff     | T38: GENERATED ALWAYS AS (VIRTUAL vs STORED) + complex expressions | Error: in prepare, near "|": syntax error
  abel TEXT GENERA |

## Speed details

- `bench.sql`: 117 lines, 3,545 bytes
- `bench_30x.sql`: 3510 lines, 106,350 bytes

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


### Slow tools (single timed run)

| Tool          |  Time |
| ------------- | ----: |
| sqlfmt (1x)   | 517ms |
| sqlfmt (30x)  | 309ms |
| sqlfluff (1x) | 185ms |

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

```text
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

### sqlite3

Runtime execution stops at the first error:

```text
Error: in prepare, table monthly_stats has 2 values for 3 columns
```

### sqlite-runner-lsp

The LSP wraps sqlite3 at runtime and reports the same single error:

```text
(no diagnostics)
```

### sql-lint

Structural checks only:

```text
/var/folders/rx/t6_rqmqx0f15l7kgp7yjhcbc0000gn/T/tmpxbgeg7p3.sql:1 sql-lint was unable to lint the following query "WITH...
```


## Per-case error detection

Schema: `users`, `orders`, `products`, `order_items`. Ground truth: sqlite3.

| Test                      | Expect | syntaqlite | sqlite3 | sqlite-runner-lsp | sql-lint |
| ------------------------- | :----: | :--------: | :-----: | :---------------: | :------: |
| keyword typo (SELEC)      | error  |   FOUND    |  FOUND  |       MISS        |  FOUND   |
| missing close paren       | error  |   FOUND    |  FOUND  |       MISS        |  FOUND   |
| double comma              | error  |   FOUND    |  FOUND  |       MISS        |   MISS   |
| unterminated string       | error  |   FOUND    |  FOUND  |       MISS        |   MISS   |
| trailing comma in VALUES  | error  |   FOUND    |  FOUND  |       MISS        |  FOUND   |
| unknown table             | error  |   FOUND    |  FOUND  |       MISS        |   MISS   |
| unknown table in JOIN     | error  |   FOUND    |  FOUND  |       MISS        |   MISS   |
| unknown column            | error  |   FOUND    |  FOUND  |       MISS        |   MISS   |
| unknown qualified column  | error  |   FOUND    |  FOUND  |       MISS        |   MISS   |
| unknown column in SELECT  | error  |   FOUND    |  FOUND  |       MISS        |   MISS   |
| SUBSTR: too few args      | error  |   FOUND    |  FOUND  |       MISS        |   MISS   |
| REPLACE: too few args     | error  |   FOUND    |  FOUND  |       MISS        |   MISS   |
| LENGTH: too many args     | error  |   FOUND    |  FOUND  |       MISS        |   MISS   |
| COALESCE: zero args       | error  |    MISS    |  FOUND  |       MISS        |   MISS   |
| CTE: 3 declared, 2 actual | error  |   FOUND    |  FOUND  |       MISS        |  FOUND   |
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
| syntaqlite        | static semantic   | 23/24 ███████████████████  |      1 |  - |
| sql-lint          | structural checks | 12/24 ██████████           |     11 |  1 |
| sqlite-runner-lsp | runtime via LSP   | 9/24 ███████               |     15 |  - |

## Speed details

- `bench.sql`: 117 lines, 3,545 bytes (+ schema preamble)
- `bench_30x.sql`: 3510 lines, 106,350 bytes (+ schema preamble)

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

# LSP details

## Tested capabilities

Each server is started, sent a test file, and probed for completion, hover,
diagnostics, and formatting via the LSP protocol.

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

## Diagnostic detail

What each server reports for `SELEC * FROM users;` (syntax error):

### syntaqlite

```text
1:1 error syntax error near 'SELEC'
```

### sqls

```text
(no diagnostics)
```

### sql-language-server

```text
1:2 error Expected "$", "(", "--", "/*", "ALTER", "CREATE TABLE", "CREATE", "DELETE", "DROP TABLE", "DROP VIEW", "DROP", "INSERT", "REPLACE", "SELECT", "UPDATE", "WITH", "return", [ \t\n\r], or end of input but "S" found.
```


## Speed details

Time to start server, send document, receive diagnostics, and exit:

| Command | Mean [ms] | Min [ms] | Max [ms] | Relative |
|:---|---:|---:|---:|---:|
| `syntaqlite` | 32.3 ± 0.9 | 30.3 | 34.7 | 1.00 |
| `sqls` | 10065.8 ± 2.8 | 10063.0 | 10069.3 | 311.28 ± 8.73 |
| `sql-language-server` | 470.5 ± 5.1 | 464.1 | 478.6 | 14.55 ± 0.44 |
