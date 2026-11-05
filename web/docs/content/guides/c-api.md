+++
title = "Using libsyntaqlite from C"
description = "Formatter options, schemas, diagnostics, and column lineage in libsyntaqlite's C API."
weight = 3
+++

# Using libsyntaqlite from C

This guide picks up where the [libsyntaqlite for C tutorial](@/getting-started/c.md)
leaves off. It covers formatter options, describing a schema with C structs,
reading diagnostics, column lineage, and who owns the memory the API returns.
For downloads and link commands, see
[Installing the C libraries](@/guides/c-install.md).

## Customize formatting

Pass a `SyntaqliteFormatConfig` to change the line width, indentation, keyword
case, and trailing semicolons:

```c
#include "syntaqlite.h"
#include <stdio.h>
#include <string.h>

static void format(SyntaqliteFormatter* f, const char* sql) {
    if (syntaqlite_formatter_format(f, sql, strlen(sql)) == SYNTAQLITE_FORMAT_OK) {
        printf("%s", syntaqlite_formatter_output(f));
    } else {
        printf("error: %s\n", syntaqlite_formatter_error_msg(f));
    }
}

int main(void) {
    SyntaqliteFormatConfig config = {
        .line_width = 40,
        .indent_width = 4,
        .keyword_case = SYNTAQLITE_KEYWORD_LOWER,
        .semicolons = 1,
    };
    SyntaqliteFormatter* f = syntaqlite_formatter_create_sqlite_with_config(&config);

    format(f, "SELECT id, name, email FROM users WHERE active = 1 AND role = 'admin'");
    format(f, "SELECT FROM users");

    syntaqlite_formatter_destroy(f);
    return 0;
}
```

```text
select id, name, email
from users
where
    active = 1
    and role = 'admin';
error: syntax error near 'FROM'
```

SQL that does not parse is not formatted. `syntaqlite_formatter_format` returns
`SYNTAQLITE_FORMAT_ERROR` and `syntaqlite_formatter_error_msg` says why. See
[Formatting options](@/reference/formatting-options.md) for what each setting
does.

## Describe a schema with structs

The tutorial loads the schema from `CREATE TABLE` statements. If your program
already knows its tables, for example from a database's catalog, it can
describe them directly. Column kinds mark generated columns, which `SELECT *`
includes but an `INSERT` without a column list does not fill, and hidden
virtual-table columns. Functions your application defines can be registered
too, so the analyzer checks how many arguments they get:

```c
#include "syntaqlite.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    SyntaqliteAnalyzer* a = syntaqlite_analyzer_create_sqlite();
    syntaqlite_analyzer_set_strict_schema(a, 1);

    SyntaqliteSchemaColumn user_columns[] = {
        {"id", SYNTAQLITE_COLUMN_NORMAL},
        {"name", SYNTAQLITE_COLUMN_NORMAL},
        {"name_lower", SYNTAQLITE_COLUMN_GENERATED},
    };
    SyntaqliteSchemaTable tables[] = {
        {"users", user_columns, 3},
    };
    syntaqlite_analyzer_add_tables(a, tables, 1);

    SyntaqliteSchemaColumn view_columns[] = {{"name", SYNTAQLITE_COLUMN_NORMAL}};
    SyntaqliteSchemaView views[] = {{"user_names", view_columns, 1}};
    syntaqlite_analyzer_add_views(a, views, 1);

    // An application-defined function taking exactly one argument.
    syntaqlite_analyzer_add_function_overload(
        a, "slugify", SYNTAQLITE_FUNCTION_SCALAR, SYNTAQLITE_ARITY_EXACT, 1);

    const char* sql =
        "SELECT slugify(name, 1) FROM users;\n"
        "SELECT nmae FROM user_names;";
    uint32_t count = syntaqlite_analyzer_analyze(a, sql, strlen(sql));

    const SyntaqliteDiagnostic* d = syntaqlite_analyzer_diagnostics(a);
    for (uint32_t i = 0; i < count; i++) {
        const char* severity =
            d[i].severity == SYNTAQLITE_SEVERITY_ERROR ? "error" : "warning";
        printf("%s at bytes %u-%u: %s\n", severity,
            d[i].start_offset, d[i].end_offset, d[i].message);
    }

    syntaqlite_analyzer_destroy(a);
    return 0;
}
```

```text
error at bytes 7-14: function 'slugify' expects 1 argument(s), got 2
error at bytes 43-47: unknown column 'nmae'
```

Leave a table's `columns` as `NULL` when you know the table exists but not its
columns. The analyzer then accepts any column of it.

## Read diagnostics one by one

`syntaqlite_analyzer_render_diagnostics`, which the tutorial uses, formats every
diagnostic for a terminal. To show them some other way, such as underlines in an
editor, read them as structs instead, as the example above does. Each has a
severity, a message, and the byte range it applies to, measured from the start
of the source passed to `syntaqlite_analyzer_analyze`. `kind_code` identifies
the check that produced it, so you can filter or count by kind.

## Column lineage

After `syntaqlite_analyzer_analyze`, you can see which table and column each
result column comes from, and which tables the query reads:

```c
#include "syntaqlite.h"
#include <string.h>
#include <stdio.h>

int main(void) {
    SyntaqliteAnalyzer* v = syntaqlite_analyzer_create_sqlite();

    // Register schema
    SyntaqliteSchemaTable tables[] = {
        {"users", (SyntaqliteSchemaColumn[]){{"id"}, {"name"}, {"email"}}, 3},
        {"posts", (SyntaqliteSchemaColumn[]){{"id"}, {"user_id"}, {"body"}}, 3},
    };
    syntaqlite_analyzer_add_tables(v, tables, 2);

    // Analyze a query
    const char* sql =
        "SELECT u.name, p.body "
        "FROM users u JOIN posts p ON u.id = p.user_id";
    syntaqlite_analyzer_analyze(v, sql, strlen(sql));

    // Column lineage
    printf("Lineage complete: %s\n",
        syntaqlite_analyzer_lineage_complete(v) ? "yes" : "no");

    uint32_t col_count = syntaqlite_analyzer_column_lineage_count(v);
    const SyntaqliteColumnLineage* cols =
        syntaqlite_analyzer_column_lineage(v);
    for (uint32_t i = 0; i < col_count; i++) {
        printf("  column %u: %s", cols[i].index, cols[i].name);
        if (cols[i].origin.table) {
            printf(" <- %s.%s",
                cols[i].origin.table, cols[i].origin.column);
        }
        printf("\n");
    }

    // Physical tables accessed
    uint32_t tbl_count = syntaqlite_analyzer_physical_table_count(v);
    const SyntaqlitePhysicalTableAccess* tbls = syntaqlite_analyzer_physical_tables(v);
    for (uint32_t i = 0; i < tbl_count; i++) {
        printf("  table: %s\n", tbls[i].name);
    }

    syntaqlite_analyzer_destroy(v);
    return 0;
}
```

```text
Lineage complete: yes
  column 0: name <- users.name
  column 1: body <- posts.body
  table: posts
  table: users
```

Lineage is complete when every source resolves to a table. It is partial when
the query reads a view whose definition the analyzer doesn't have. For
statements other than `SELECT`, the counts are 0 and the pointers are `NULL`.

## Memory

- Create a formatter, analyzer, or parser once and reuse it. Each one keeps
  its buffers between calls.
- Strings and arrays the API returns belong to the handle. They stay valid
  until the next call that produces new results on the same handle, or until
  you destroy it. Copy anything you need to keep longer.
- `syntaqlite_dump_node` is the exception: it returns a string you own. Free
  it with `free`, or with your allocator's free function if you passed one to
  `syntaqlite_parser_create`.
- SQL passed to the formatter or analyzer only needs to stay valid during that
  call. The parser keeps using the SQL passed to `syntaqlite_parser_reset`, so
  keep it valid until the next reset or until you destroy the parser.

The [C API reference](@/reference/c-api.md#memory-model) has the details for
each function.

## Next steps

- [libsyntaqlite C API reference](@/reference/c-api.md): every function and
  type
- [libsyntaqlite-parser for C](@/getting-started/c-parser.md): if you only
  need to parse SQL
