+++
title = "AST and token reference"
description = "Node types, token types, and parse tree structure."
weight = 7
+++

# AST and token reference

syntaqlite's parser produces a flat arena of typed nodes. This page lists all
node types and token types. For a guide on using the parser from Rust, see
[Using libsyntaqlite from Rust](@/guides/rust-api.md#parse-sql).

## Parse tree structure

All nodes for a statement are allocated into a contiguous arena. Each node has
a **tag** (its type) and a fixed set of **fields**:

| Field kind | Storage | Examples |
|------------|---------|----------|
| Index | Reference to another node in the arena | Child nodes (expressions, clauses) |
| Inline span | `(offset, length)` into the source text | Identifiers, literals |
| Inline enum | Discriminant value | `BinaryOp`, `SortOrder` |
| Inline flags | Bit field (up to 8 bits) | `DISTINCT`, `STAR` |
| Inline bool | Boolean | `IF NOT EXISTS` |

List nodes (e.g., `ResultColumnList`) store a count followed by that many
child references.

## Statement nodes

| Node | SQL |
|------|-----|
| `SelectStmt` | `SELECT ...` |
| `CompoundSelect` | `SELECT ... UNION SELECT ...` |
| `InsertStmt` | `INSERT INTO ...` |
| `UpdateStmt` | `UPDATE ...` |
| `DeleteStmt` | `DELETE FROM ...` |
| `CreateTableStmt` | `CREATE TABLE ...` |
| `CreateIndexStmt` | `CREATE INDEX ...` |
| `CreateViewStmt` | `CREATE VIEW ...` |
| `CreateTriggerStmt` | `CREATE TRIGGER ...` |
| `CreateVirtualTableStmt` | `CREATE VIRTUAL TABLE ...` |
| `DropStmt` | `DROP TABLE/INDEX/VIEW/TRIGGER ...` |
| `AlterTableStmt` | `ALTER TABLE ...` |
| `PragmaStmt` | `PRAGMA ...` |
| `AnalyzeOrReindexStmt` | `ANALYZE ...` / `REINDEX ...` |
| `AttachStmt` | `ATTACH DATABASE ...` |
| `DetachStmt` | `DETACH DATABASE ...` |
| `VacuumStmt` | `VACUUM ...` |
| `ExplainStmt` | `EXPLAIN ...` |
| `TransactionStmt` | `BEGIN/COMMIT/ROLLBACK ...` |
| `SavepointStmt` | `SAVEPOINT/RELEASE ...` |
| `WithClause` | `WITH ... SELECT/INSERT/UPDATE/DELETE ...` |
| `ValuesClause` | `VALUES (...)` |

## Expression nodes

| Node | Example |
|------|---------|
| `BinaryExpr` | `a + b`, `x = 1`, `a AND b` |
| `UnaryExpr` | `-x`, `NOT flag` |
| `Literal` | `42`, `'hello'`, `NULL`, `X'FF'` |
| `ColumnRef` | `t.col`, `col` |
| `Variable` | `?`, `?1`, `:name`, `@var`, `$param` |
| `FunctionCall` | `length(name)` |
| `AggregateFunctionCall` | `count(*)`, `sum(DISTINCT x)` |
| `OrderedSetFunctionCall` | `percentile(x, 0.5)` |
| `CastExpr` | `CAST(x AS TEXT)` |
| `CollateExpr` | `name COLLATE NOCASE` |
| `CaseExpr` | `CASE WHEN ... THEN ... END` |
| `CaseWhen` | `WHEN ... THEN ...` branch |
| `SubqueryExpr` | `(SELECT ...)` as expression |
| `ExistsExpr` | `EXISTS (SELECT ...)` |
| `InExpr` | `x IN (1, 2, 3)` |
| `IsExpr` | `x IS NULL` |
| `BetweenExpr` | `x BETWEEN 1 AND 10` |
| `LikeExpr` | `name LIKE 'A%'` |

## Clause and source nodes

| Node | Description |
|------|-------------|
| `ResultColumn` | Single item in SELECT list |
| `ResultColumnList` | Full SELECT list |
| `TableRef` | Table reference in FROM |
| `SubqueryTableSource` | Subquery in FROM |
| `JoinClause` | `JOIN ... ON ...` |
| `JoinPrefix` | `INNER`, `LEFT OUTER`, `CROSS`, etc. |
| `OrderingTerm` | Single `ORDER BY` item |
| `OrderByList` | Full `ORDER BY` clause |
| `LimitClause` | `LIMIT ... OFFSET ...` |
| `CteDefinition` | Single CTE in WITH |
| `CteList` | List of CTEs |
| `WindowDef` | `OVER (PARTITION BY ... ORDER BY ...)` |
| `NamedWindowDef` | `WINDOW name AS (...)` |
| `FrameSpec` | `ROWS/RANGE/GROUPS BETWEEN ...` |
| `FrameBound` | `CURRENT ROW`, `N PRECEDING`, etc. |

## Schema nodes

| Node | Description |
|------|-------------|
| `ColumnDef` | Column definition in CREATE TABLE |
| `ColumnDefList` | List of column definitions |
| `ColumnConstraint` | `PRIMARY KEY`, `NOT NULL`, `UNIQUE`, `CHECK`, `DEFAULT`, etc. |
| `ColumnConstraintList` | List of column constraints |
| `TableConstraint` | Table-level `PRIMARY KEY`, `UNIQUE`, `FOREIGN KEY` |
| `TableConstraintList` | List of table constraints |
| `ForeignKeyClause` | `REFERENCES table(col)` with actions |

## Enums

Inline enum fields used across node types:

| Enum | Values |
|------|--------|
| `BinaryOp` | `PLUS`, `MINUS`, `STAR`, `SLASH`, `REM`, `LT`, `GT`, `LE`, `GE`, `EQ`, `NE`, `AND`, `OR`, `BIT_AND`, `BIT_OR`, `LSHIFT`, `RSHIFT`, `CONCAT`, `PTR`, `PTR2` |
| `UnaryOp` | `MINUS`, `PLUS`, `BIT_NOT`, `NOT` |
| `LiteralType` | `INTEGER`, `FLOAT`, `STRING`, `BLOB`, `NULL`, `CURRENT`, `QNUMBER` |
| `SortOrder` | `ASC`, `DESC` |
| `NullsOrder` | `NONE`, `FIRST`, `LAST` |
| `ConflictAction` | `DEFAULT`, `ROLLBACK`, `ABORT`, `FAIL`, `IGNORE`, `REPLACE` |
| `CompoundOp` | `UNION`, `UNION_ALL`, `INTERSECT`, `EXCEPT` |
| `ForeignKeyAction` | `NO_ACTION`, `SET_NULL`, `SET_DEFAULT`, `CASCADE`, `RESTRICT` |
| `Materialized` | `DEFAULT`, `MATERIALIZED`, `NOT_MATERIALIZED` |

## Flags

Compact bit fields stored inline in nodes:

| Flag field | Bits |
|------------|------|
| `ResultColumnFlags` | `STAR` (bit 0) |
| `SelectStmtFlags` | `DISTINCT` (bit 0), `ALL` (bit 1) |
| `CreateTableStmtFlags` | `IF_NOT_EXISTS` (bit 0), `TEMP` (bit 1), `WITHOUT_ROWID` (bit 2), `STRICT` (bit 3) |

## Token types

The tokenizer produces 187 token types. The main categories:

### Keywords

| Token | Token | Token | Token |
|-------|-------|-------|-------|
| `Abort` | `Action` | `Add` | `After` |
| `All` | `Alter` | `Always` | `Analyze` |
| `And` | `As` | `Asc` | `Attach` |
| `Autoincr` | `Before` | `Begin` | `Between` |
| `By` | `Cascade` | `Case` | `Cast` |
| `Check` | `Collate` | `Column` | `Commit` |
| `Conflict` | `Constraint` | `Create` | `Cross` |
| `Current` | `CurrentDate` | `CurrentTime` | `CurrentTimestamp` |
| `Database` | `Default` | `Deferrable` | `Deferred` |
| `Delete` | `Desc` | `Detach` | `Distinct` |
| `Do` | `Drop` | `Each` | `Else` |
| `End` | `Escape` | `Except` | `Exclude` |
| `Exclusive` | `Exists` | `Explain` | `Fail` |
| `Filter` | `First` | `Following` | `For` |
| `Foreign` | `From` | `Full` | `Glob` |
| `Group` | `Groups` | `Having` | `If` |
| `Ignore` | `Immediate` | `In` | `Index` |
| `Indexed` | `Initially` | `Inner` | `Insert` |
| `Instead` | `Intersect` | `Into` | `Is` |
| `Isnull` | `Join` | `Key` | `Last` |
| `Left` | `Like` | `Limit` | `Match` |
| `Materialized` | `Natural` | `No` | `Not` |
| `Nothing` | `Notnull` | `Null` | `Nulls` |
| `Of` | `Offset` | `On` | `Or` |
| `Order` | `Others` | `Outer` | `Over` |
| `Partition` | `Plan` | `Pragma` | `Preceding` |
| `Primary` | `Query` | `Raise` | `Range` |
| `Recursive` | `References` | `Reindex` | `Release` |
| `Rename` | `Replace` | `Restrict` | `Returning` |
| `Right` | `Rollback` | `Row` | `Rows` |
| `Savepoint` | `Select` | `Set` | `Table` |
| `Temp` | `Then` | `Ties` | `To` |
| `Transaction` | `Trigger` | `Unbounded` | `Union` |
| `Unique` | `Update` | `Using` | `Vacuum` |
| `Values` | `View` | `Virtual` | `When` |
| `Where` | `Window` | `With` | `Without` |

### Operators and punctuation

| Token | Symbol | Token | Symbol |
|-------|--------|-------|--------|
| `Plus` | `+` | `Minus` | `-` |
| `Star` | `*` | `Slash` | `/` |
| `Rem` | `%` | `Eq` | `=` / `==` |
| `Ne` | `!=` / `<>` | `Lt` | `<` |
| `Gt` | `>` | `Le` | `<=` |
| `Ge` | `>=` | `Concat` | `\|\|` |
| `Ptr` | `->` | `Ptr2` | `->>` |
| `BitAnd` | `&` | `BitOr` | `\|` |
| `BitNot` | `~` | `LShift` | `<<` |
| `RShift` | `>>` | `Lp` | `(` |
| `Rp` | `)` | `Comma` | `,` |
| `Dot` | `.` | `Semi` | `;` |

### Literals and identifiers

| Token | Description |
|-------|-------------|
| `Integer` | Integer literal (`42`) |
| `Float` | Floating-point literal (`3.14`) |
| `String` | String literal (`'hello'`) |
| `Blob` | Blob literal (`X'FF'`) |
| `Id` | Identifier (`users`, `"quoted id"`) |
| `Variable` | Bind parameter (`?`, `?1`, `:name`, `@var`, `$param`) |

### Whitespace, comments, and errors

| Token | Description |
|-------|-------------|
| `Space` | Whitespace (spaces, tabs, newlines) |
| `Comment` | Line (`--`) or block (`/* */`) comment |
| `Error` | Unrecognized or malformed token |
| `Illegal` | Token not valid in current context |
