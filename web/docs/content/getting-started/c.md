+++
title = "libsyntaqlite for C"
description = "Parse, format, and statically analyze SQLite SQL from C or C++."
weight = 5
+++

# Using libsyntaqlite from C

libsyntaqlite provides C APIs for parsing, formatting, and analyzing SQLite
SQL. This tutorial downloads the prebuilt library, formats a query, validates
it against a schema, and prints its syntax tree.

> **Only need to parse SQL?** [libsyntaqlite-parser](@/getting-started/c-parser.md)
> is libsyntaqlite's parser and tokenizer on their own. It has the same
> functions, needs no Rust runtime, and is available as prebuilt libraries or
> as two C files to compile into your project.

## 1. Download libsyntaqlite

Download the archive for the latest release:

```bash
mkdir sql-check && cd sql-check
curl -L https://github.com/LalitMaganti/syntaqlite/releases/download/v0.13.5/libsyntaqlite-0.13.5.tar.gz | tar xz
```

It contains the header, `syntaqlite.h`, and a shared library for each
platform. The commands below use the Linux x64 library; see
[Installing the C libraries](@/guides/c-install.md) for macOS and Windows.

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

The formatter is reusable: create it once and call
`syntaqlite_formatter_format` for each query.

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

Strict schema mode reports unknown tables and columns as errors rather than
warnings.

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

These parser functions are the same ones libsyntaqlite-parser provides, so this
code also works with libsyntaqlite-parser after changing the header to
`libsyntaqlite_parser.h`.

## Next steps

- [Using libsyntaqlite from C](@/guides/c-api.md): formatter configuration,
  table registration, and column lineage
- [Installing the C libraries](@/guides/c-install.md): linking on macOS and
  Windows, and the libsyntaqlite-parser downloads
- [libsyntaqlite C API reference](@/reference/c-api.md): all functions, types,
  and the memory model
