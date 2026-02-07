# Dialect Extension Workflow

Build a single amalgamated header for SQLite dialect extensions and inject it at compile time with one define.

## Building the Extension Header

```bash
tools/build-extension-grammar --grammar my_dialect.y --output my_dialect.h
```

The generated header contains all three data sections:
1. Token definitions (`#define SYNTAQLITE_TOKEN_*`)
2. Keyword hash data (for tokenizer)
3. Parser tables and extension reduce function (for parser)

## Compiling with the Extension

Pass a single define pointing at the amalgamated header:

```bash
clang -c sqlite_tokenize_gen.c sqlite_parse_gen.c \
    -DSYNTAQLITE_EXTENSION_GRAMMAR=\"my_dialect.h\" \
    -I/path/to/syntaqlite
```

The `SYNTAQLITE_EXTENSION_GRAMMAR` macro is honored by all three injection sites:
- `include/syntaqlite/sqlite_tokens_gen.h` — token defines
- `src/tokenizer/sqlite_tokenize_gen.c` — keyword hash data
- `src/parser/sqlite_parse_gen.c` — parser tables and reduce actions

## Extension Grammar Format

The `.y` file uses standard Lemon syntax. Declare new tokens with `%token`:

```
%token MYkeyword ANOTHER_KEYWORD
```

Add grammar rules that reference both new and existing nonterminals:

```
cmd ::= MYkeyword expr(E). { /* action */ }
```

## Tokenizer-Only Mode

To skip parser data (e.g., for keyword-only extensions):

```bash
tools/build-extension-grammar --grammar my_dialect.y --output my_dialect.h --tokenizer-only
```
