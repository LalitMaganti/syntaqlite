+++
title = "MCP tools"
description = "Tools exposed by the syntaqlite MCP server."
weight = 8
+++

# MCP tools reference

The syntaqlite MCP server exposes three tools. It is built into the
`syntaqlite` binary and runs over stdio via `syntaqlite mcp`.

For setup instructions, see
[MCP server setup](@/guides/mcp.md).

## `format_sql`

Format a SQL string.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `sql` | string | *(required)* | The SQL to format |
| `line_width` | integer | `80` | Maximum line width |
| `keyword_case` | string | `"upper"` | `"upper"` or `"lower"` |
| `semicolons` | boolean | `true` | Append trailing semicolons |

**Returns:** The formatted SQL as a string. On parse error, returns
`"Error: <message>"`.

**Example input:**

```json
{
  "sql": "select a,b from t where x=1",
  "line_width": 80,
  "keyword_case": "upper"
}
```

**Example output:**

```sql
SELECT a, b FROM t WHERE x = 1;
```

## `parse_sql`

Parse a SQL string and return its AST as an indented text dump.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `sql` | string | *(required)* | The SQL to parse |

**Returns:** A text representation of the abstract syntax tree. On parse
error, returns `"Error: <message>"`.

**Example input:**

```json
{
  "sql": "SELECT id FROM users"
}
```

**Example output:**

```
SelectStmt
  flags: (none)
  columns:
    ResultColumnList [1 items]
      ResultColumn
        flags: (none)
        alias: (none)
        expr:
          ColumnRef
            column: "id"
            table: (none)
            schema: (none)
  from_clause:
    TableRef
      table_name: "users"
      schema: (none)
      alias: (none)
      args: (none)
  where_clause: (none)
  groupby: (none)
  having: (none)
  orderby: (none)
  limit_clause: (none)
  window_clause: (none)
```

## `validate_sql`

Check whether a SQL string is syntactically valid.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `sql` | string | *(required)* | The SQL to validate |

**Returns:** A JSON string with two fields:

| Field | Type | Description |
|-------|------|-------------|
| `valid` | boolean | `true` if the SQL parsed without errors |
| `errors` | string | Error messages (empty string if valid) |

**Example (valid SQL):**

```json
{"valid": true, "errors": ""}
```

**Example (invalid SQL):**

```json
{"valid": false, "errors": "error: syntax error near 'SELEC'\n --> <stdin>:1:1\n  |\n1 | SELEC 1\n  | ^~~~~\n0 statements parsed, 1 errors\nerror: 1 syntax error(s)"}
```

Note: `validate_sql` checks syntax only (successful parse). It does not run
analysis (unknown table/column detection). For schema-aware
validation, use the CLI directly:

```bash
syntaqlite analyze schema.sql
```
