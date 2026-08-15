+++
title = "C parser amalgamation"
description = "Parse SQLite SQL from C with the dependency-free source amalgamation."
weight = 5
+++

# Parsing SQLite SQL from C

This tutorial walks you through parsing SQL from C. By the end you'll have a
small program that parses queries, prints their AST, recovers from errors, and
lists tokens — with nothing to install beyond a C compiler.

There are two C distributions. This tutorial uses the **source amalgamation**,
which provides the parser and tokenizer as two C files you compile directly into
your project. The prebuilt libsyntaqlite library also adds formatting and static
analysis; see the [libsyntaqlite C guide](@/guides/c-api.md)
once you're done here.

## 1. Download the amalgamation

The source amalgamation is two files: `syntaqlite_syntax.h` (header) and
`syntaqlite_syntax.c` (implementation). Download them from the latest release:

```bash
mkdir sql-parser && cd sql-parser
curl -L https://github.com/LalitMaganti/syntaqlite/releases/latest/download/syntaqlite-syntax-amalgamation.tar.gz | tar xz
```

You should have:

```bash
ls
# syntaqlite_syntax.c  syntaqlite_syntax.h
```

## 2. Write the program

Create `parse.c`:

```c
#include "syntaqlite_syntax.h"
#include <stdio.h>
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

```bash
cc -O2 -o parse parse.c syntaqlite_syntax.c
```

Run it with the default query:

```bash
./parse
```

```text
--- statement 1 ---
SelectStmt
  columns:
    ResultColumn
      expr:
        ColumnRef
          column: "id"
    ResultColumn
      expr:
        ColumnRef
          column: "name"
  from_clause:
    TableRef
      table_name: "users"
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
  columns:
    ResultColumn
      expr:
        Literal: 1
```

Statement 1 failed but statement 2 still parsed successfully.

## 5. Access tokens

Enable the token side-table to see what the tokenizer produced:

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

## Next steps

- [Using libsyntaqlite from C](@/guides/c-api.md): switch to the prebuilt
  library to format and statically analyze SQLite SQL from C
- [libsyntaqlite C API](@/reference/c-api.md): all parser, tokenizer, formatter,
  and analyzer functions
