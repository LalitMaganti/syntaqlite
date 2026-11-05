+++
title = "libsyntaqlite-parser for C"
description = "Parse SQLite SQL from C with libsyntaqlite-parser."
weight = 6
+++

# Using libsyntaqlite-parser from C

libsyntaqlite-parser is the part of libsyntaqlite that parses SQL. In this
tutorial you use it to parse a few queries and print their syntax trees, see
how the parser handles errors, and list the tokens it read.

If you also want to format or analyze SQL, use
[libsyntaqlite](@/getting-started/c.md). Its parser functions are the same.

## 1. Get libsyntaqlite-parser

You can download libsyntaqlite-parser as a prebuilt library or as C source.
The code in this tutorial is the same for both.

<div class="tabs" data-tab-group="parser-get">
  <div class="tab-buttons">
    <button class="active" data-tab="prebuilt" onclick="switchTab('parser-get','prebuilt')">Prebuilt library</button>
    <button data-tab="source" onclick="switchTab('parser-get','source')">C source</button>
  </div>
  <div class="tab-panel active" data-tab="prebuilt">
    <pre><code class="language-bash">mkdir sql-parser &amp;&amp; cd sql-parser
curl -L https://github.com/LalitMaganti/syntaqlite/releases/download/v0.13.5/libsyntaqlite-parser-0.13.5.tar.gz | tar xz</code></pre>
    <p>The header, plus a static and a shared library for each platform.</p>
  </div>
  <div class="tab-panel" data-tab="source">
    <pre><code class="language-bash">mkdir sql-parser &amp;&amp; cd sql-parser
curl -L https://github.com/LalitMaganti/syntaqlite/releases/download/v0.13.5/libsyntaqlite-parser-0.13.5-src.tar.gz | tar xz</code></pre>
    <p>The header and <code>libsyntaqlite_parser.c</code>, which you compile with your program.</p>
  </div>
</div>

## 2. Write the program

Create `parse.c`:

```c
#include "libsyntaqlite_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    const char* sql = "SELECT id, name FROM users WHERE active = 1";
    if (argc > 1) {
        sql = argv[1];
    }

    // Create a parser for the SQLite dialect.
    SyntaqliteParser* p = syntaqlite_parser_create(NULL);

    // Feed the source text.
    syntaqlite_parser_reset(p, sql, strlen(sql));

    // Parse each statement (a source string can contain multiple).
    int stmt = 0;
    for (;;) {
        int32_t rc = syntaqlite_parser_next(p);
        if (rc == SYNTAQLITE_PARSE_DONE)
            break;
        stmt++;
        if (rc == SYNTAQLITE_PARSE_ERROR) {
            fprintf(stderr, "error in statement %d: %s\n",
                stmt, syntaqlite_result_error_msg(p));
            continue;
        }

        // Print the AST.
        uint32_t root = syntaqlite_result_root(p);
        char* dump = syntaqlite_dump_node(p, root, 0);
        printf("--- statement %d ---\n%s\n", stmt, dump);
        free(dump);
    }

    syntaqlite_parser_destroy(p);
    return 0;
}
```

## 3. Compile and run

If you downloaded the prebuilt library, link the static library for your
platform:

```bash
cc -o parse parse.c -I. linux-x64/libsyntaqlite_parser.a -ldl
```

If you downloaded the source, compile it with your program:

```bash
cc -O2 -o parse parse.c libsyntaqlite_parser.c -ldl
```

`-ldl` is only needed on Linux systems with glibc older than 2.34. For macOS,
Windows, and the shared library, see
[Installing the C libraries](@/guides/c-install.md).

Run it with the default query:

```bash
./parse
```

```text
--- statement 1 ---
SelectStmt
  flags: (none)
  columns:
    ResultColumnList [2 items]
      ResultColumn
        flags: (none)
        alias: (none)
        alias_as: FALSE
        expr:
          ColumnRef
            column: "id"
            table: (none)
            schema: (none)
      ...
  from_clause:
    TableRef
      table_name: "users"
      ...
  where_clause:
    BinaryExpr
      op: EQ
      ...
```

Pass your own SQL:

```bash
./parse "CREATE TABLE t(x INTEGER PRIMARY KEY, y TEXT NOT NULL)"
```

## 4. Handle errors

The parser recovers from errors and keeps going. Try invalid SQL:

```bash
./parse "SELECT FROM; SELECT 1"
```

```text
error in statement 1: syntax error near 'FROM'
--- statement 2 ---
SelectStmt
  flags: (none)
  columns:
    ResultColumnList [1 items]
      ResultColumn
        flags: (none)
        alias: (none)
        alias_as: FALSE
        expr:
          Literal
            literal_type: INTEGER
            source: "1"
  ...
```

The first statement fails, but the parser carries on and parses the second.

## 5. Access tokens

To see the tokens the parser read, turn on token collection before parsing:

```c
syntaqlite_parser_set_collect_tokens(p, 1);
syntaqlite_parser_reset(p, sql, strlen(sql));
syntaqlite_parser_next(p);

uint32_t count;
const SyntaqliteParserToken* tokens =
    syntaqlite_result_tokens(p, &count);
for (uint32_t i = 0; i < count; i++) {
    printf("token %u: type=%u offset=%u len=%u\n",
        i, tokens[i].type,
        tokens[i].offset, tokens[i].length);
}
```

For `SELECT id FROM users`, this prints:

```text
token 0: type=161 offset=0 len=6
token 1: type=40 offset=7 len=2
token 2: type=127 offset=10 len=4
token 3: type=40 offset=15 len=5
```

## Next steps

- [libsyntaqlite for C](@/getting-started/c.md): format and analyze SQL as well
  as parse it
- [Installing the C libraries](@/guides/c-install.md): linking on macOS and
  Windows
- [libsyntaqlite C API reference](@/reference/c-api.md): all parser and
  tokenizer functions
