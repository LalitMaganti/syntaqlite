+++
title = "Using libsyntaqlite from C"
description = "Parse, format, and statically analyze SQLite SQL from C or C++."
weight = 2
+++

# Using libsyntaqlite from C

This guide covers libsyntaqlite's formatter and analyzer from C and C++. To
get started, follow the [libsyntaqlite for C tutorial](@/getting-started/c.md).
To download and link the library, see
[Installing the C libraries](@/guides/c-install.md).

## Format a query

```c
#include "syntaqlite.h"
#include <string.h>
#include <stdio.h>

int main(void) {
    SyntaqliteFormatter* f = syntaqlite_formatter_create_sqlite();

    const char* sql = "select a,b from t where x=1";
    if (syntaqlite_formatter_format(f, sql, strlen(sql)) == 0) {
        printf("%.*s\n",
            syntaqlite_formatter_output_len(f),
            syntaqlite_formatter_output(f));
    }

    syntaqlite_formatter_destroy(f);
    return 0;
}
```

The formatter handle is reusable: create once, call `format()` repeatedly.
Output is borrowed and valid until the next call to format or destroy.

## Format with custom config

```c
SyntaqliteFormatConfig config = {
    .line_width   = 120,
    .indent_width = 4,
    .keyword_case = SYNTAQLITE_KEYWORD_LOWER,
    .semicolons   = 1,
};
SyntaqliteFormatter* f =
    syntaqlite_formatter_create_sqlite_with_config(&config);

const char* sql = "select a,b from t where x=1";
if (syntaqlite_formatter_format(f, sql, strlen(sql)) == 0) {
    printf("%.*s\n",
        syntaqlite_formatter_output_len(f),
        syntaqlite_formatter_output(f));
}

syntaqlite_formatter_destroy(f);
```

## Validate a query

```c
#include "syntaqlite.h"
#include <string.h>
#include <stdio.h>

int main(void) {
    SyntaqliteAnalyzer* v = syntaqlite_analyzer_create_sqlite();

    // Register your schema
    SyntaqliteTableDef tables[] = {
        {"users", (SyntaqliteColumnDef[]){{"id"}, {"name"}, {"email"}}, 3},
        {"posts", (SyntaqliteColumnDef[]){{"id"}, {"user_id"}, {"body"}}, 3},
    };
    syntaqlite_analyzer_add_tables(v, tables, 2);

    // Analyze
    const char* sql = "SELECT nme FROM users";
    syntaqlite_analyzer_analyze(v, sql, strlen(sql));

    // Print human-readable diagnostics
    const char* rendered =
        syntaqlite_analyzer_render_diagnostics(v, "query.sql");
    if (rendered[0] != '\0') {
        printf("%s\n", rendered);
    }

    syntaqlite_analyzer_destroy(v);
    return 0;
}
```

## Column lineage

After `analyze()`, you can inspect which tables and columns each result column
traces back to:

```c
#include "syntaqlite.h"
#include <string.h>
#include <stdio.h>

int main(void) {
    SyntaqliteAnalyzer* v = syntaqlite_analyzer_create_sqlite();

    // Register schema
    SyntaqliteTableDef tables[] = {
        {"users", (SyntaqliteColumnDef[]){{"id"}, {"name"}, {"email"}}, 3},
        {"posts", (SyntaqliteColumnDef[]){{"id"}, {"user_id"}, {"body"}}, 3},
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

Output:

```
Lineage complete: yes
  column 0: name <- users.name
  column 1: body <- posts.body
  table: posts
  table: users
```

Lineage is "complete" when all sources resolve to physical tables. It becomes
"partial" when a view is referenced but its body is unavailable. For
non-SELECT statements, all count functions return 0 and pointer functions
return NULL.

## Next steps

- See the [libsyntaqlite C API reference](@/reference/c-api.md) for all functions, types,
  and the memory model
- To only parse SQL, see [libsyntaqlite-parser for C](@/getting-started/c-parser.md)
