+++
title = "Using libsyntaqlite from C"
description = "Parse, format, and statically analyze SQLite SQL from C or C++."
weight = 2
+++

# Using libsyntaqlite from C

libsyntaqlite is an embeddable parser, formatter, and static analyzer for SQLite
SQL. It provides a C API for C and C++ projects, with two distribution options:

| | Source amalgamation | Prebuilt shared library |
|---|---|---|
| **Package** | `syntaqlite-syntax-amalgamation` | `libsyntaqlite` |
| **Contents** | Two C files to compile yourself | Shared library + header per platform |
| **Parser & tokenizer** | Yes | Yes |
| **Formatter** | No | Yes |
| **Validator** | No | Yes |
| **Dependencies** | None (just a C compiler) | Link against the shared library |

## Option 1: Source amalgamation (parser and tokenizer only)

The source amalgamation contains the **parser and tokenizer** as two compilable C
files. It does **not** include the formatter or analyzer; those require the
Rust runtime and are only available via the prebuilt shared library (option 2).

Download from the
[latest release](https://github.com/LalitMaganti/syntaqlite/releases):

```bash
curl -LO https://github.com/LalitMaganti/syntaqlite/releases/latest/download/syntaqlite-syntax-amalgamation.tar.gz
tar xf syntaqlite-syntax-amalgamation.tar.gz
```

The archive contains `syntaqlite_syntax.h` and `syntaqlite_syntax.c`. Add both
to your project and compile them:

```bash
cc -c -O2 syntaqlite_syntax.c -o syntaqlite_syntax.o
cc -o my_program my_program.c syntaqlite_syntax.o
```

See the [C parser tutorial](@/getting-started/c-parser.md) for a complete
walkthrough.

## Option 2: Prebuilt shared library (full API)

The prebuilt shared library includes the **full API**: parser, tokenizer,
formatter, and analyzer. Download `libsyntaqlite` from the
[latest release](https://github.com/LalitMaganti/syntaqlite/releases). The
archive contains a single `syntaqlite.h` header and shared libraries for each
platform:

```bash
curl -LO https://github.com/LalitMaganti/syntaqlite/releases/latest/download/libsyntaqlite.tar.gz
tar xf libsyntaqlite.tar.gz
```

```
syntaqlite.h
macos-arm64/libsyntaqlite.dylib
macos-x64/libsyntaqlite.dylib
linux-x64/libsyntaqlite.so
linux-arm64/libsyntaqlite.so
windows-x64/syntaqlite.dll
windows-arm64/syntaqlite.dll
```

Compile and link against the library for your platform:

```bash
# Linux x64
cc -o my_program my_program.c -I. -Llinux-x64 -lsyntaqlite -Wl,-rpath,'$ORIGIN/linux-x64'

# macOS arm64
cc -o my_program my_program.c -I. -Lmacos-arm64 -lsyntaqlite -Wl,-rpath,@executable_path/macos-arm64
```

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
    SyntaqliteRelationDef tables[] = {
        {"users",  (const char*[]){"id", "name", "email"}, 3},
        {"posts",  (const char*[]){"id", "user_id", "body"}, 3},
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
    SyntaqliteRelationDef tables[] = {
        {"users", (const char*[]){"id", "name", "email"}, 3},
        {"posts", (const char*[]){"id", "user_id", "body"},  3},
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
