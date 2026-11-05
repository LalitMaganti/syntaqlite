+++
title = "libsyntaqlite for C"
description = "Parse, format, and statically analyze SQLite SQL from C or C++."
weight = 5
+++

# Using libsyntaqlite from C

libsyntaqlite is syntaqlite's C library. In this tutorial you download it,
format a query, check the query against a schema, and print its syntax tree.

> **Only parsing SQL?** Use [libsyntaqlite-parser](@/getting-started/c-parser.md)
> instead. It is a few hundred kilobytes rather than a megabyte or more, and you
> can build it from source with just a C compiler.

## 1. Download libsyntaqlite

Download the archive for the latest release:

```bash
mkdir sql-check && cd sql-check
curl -L https://github.com/LalitMaganti/syntaqlite/releases/download/v0.14.0/libsyntaqlite-0.14.0.tar.gz | tar xz
```

The archive has the header, `syntaqlite.h`, and a static and a shared library
for each platform. This tutorial links the Linux x64 shared library. For the
static library, macOS, and Windows, see
[Installing the C libraries](@/guides/c-install.md).

## 2. Format a query

Create `format.c`:

```c
#include "syntaqlite.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    SyntaqliteFormatter* f = syntaqlite_formatter_create_sqlite();

    const char* sql = "select id,name,email from users where active=1 order by name";
    if (syntaqlite_formatter_format(f, sql, strlen(sql)) == 0) {
        printf("%.*s",
            (int)syntaqlite_formatter_output_len(f),
            syntaqlite_formatter_output(f));
    }

    syntaqlite_formatter_destroy(f);
    return 0;
}
```

Compile and run it:

```bash
cc -o format format.c -I. -Llinux-x64 -lsyntaqlite -Wl,-rpath,'$ORIGIN/linux-x64'
./format
```

```sql
SELECT id, name, email FROM users WHERE active = 1 ORDER BY name;
```

You can keep one formatter and call `syntaqlite_formatter_format` for every
query you need to format.

## 3. Validate against a schema

Create `analyze.c`:

```c
#include "syntaqlite.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    SyntaqliteAnalyzer* a = syntaqlite_analyzer_create_sqlite();
    syntaqlite_analyzer_set_strict_schema(a, 1);

    // Register the schema from CREATE TABLE statements.
    const char* schema =
        "CREATE TABLE users (id INTEGER, name TEXT, email TEXT, active INTEGER);";
    syntaqlite_analyzer_load_schema_ddl(a, schema, strlen(schema));

    // Analyze a query against the schema.
    const char* sql = "SELECT id, nme FROM users WHERE active = 1";
    syntaqlite_analyzer_analyze(a, sql, strlen(sql));

    const char* diagnostics = syntaqlite_analyzer_render_diagnostics(a, "query.sql");
    if (diagnostics[0] == '\0') {
        printf("No errors found.\n");
    } else {
        printf("%s", diagnostics);
    }

    syntaqlite_analyzer_destroy(a);
    return 0;
}
```

Compile and run it:

```bash
cc -o analyze analyze.c -I. -Llinux-x64 -lsyntaqlite -Wl,-rpath,'$ORIGIN/linux-x64'
./analyze
```

```text
error: unknown column 'nme'
 --> query.sql:1:12
  |
1 | SELECT id, nme FROM users WHERE active = 1
  |            ^~~
  = help: did you mean 'name'?
```

`syntaqlite_analyzer_set_strict_schema` makes unknown tables and columns
errors. Without it, they are reported as warnings.

## 4. Parse and inspect the AST

Create `parse.c`:

```c
#include "syntaqlite.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    const char* sql = "SELECT 1 + 2; SELECT 'hello';";
    SyntaqliteParser* p = syntaqlite_parser_create(NULL);
    syntaqlite_parser_reset(p, sql, strlen(sql));

    int stmt = 0;
    for (;;) {
        int32_t rc = syntaqlite_parser_next(p);
        if (rc == SYNTAQLITE_PARSE_DONE) {
            break;
        }
        stmt++;
        if (rc == SYNTAQLITE_PARSE_ERROR) {
            fprintf(stderr, "error: %s\n", syntaqlite_result_error_msg(p));
            continue;
        }
        char* dump = syntaqlite_dump_node(p, syntaqlite_result_root(p), 0);
        printf("--- statement %d ---\n%s\n", stmt, dump);
        free(dump);
    }

    syntaqlite_parser_destroy(p);
    return 0;
}
```

Compile and run it:

```bash
cc -o parse parse.c -I. -Llinux-x64 -lsyntaqlite -Wl,-rpath,'$ORIGIN/linux-x64'
./parse
```

```text
--- statement 1 ---
SelectStmt
  flags: (none)
  columns:
    ResultColumnList [1 items]
      ResultColumn
        flags: (none)
        alias: (none)
        alias_as: FALSE
        expr:
          BinaryExpr
            op: PLUS
            left:
              Literal
                literal_type: INTEGER
                source: "1"
            right:
              Literal
                literal_type: INTEGER
                source: "2"
  ...
```

libsyntaqlite-parser has the same parser functions, so this program also
builds against it if you include `libsyntaqlite_parser.h` instead.

## Next steps

- [Using libsyntaqlite from C](@/guides/c-api.md): formatter configuration,
  table registration, and column lineage
- [Installing the C libraries](@/guides/c-install.md): linking on macOS and
  Windows
- [libsyntaqlite C API reference](@/reference/c-api.md): all functions, types,
  and the memory model
