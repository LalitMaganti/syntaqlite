# .synq: Node Definition Language

These files define the AST that syntaqlite's parser produces. From
them, the codegen tool generates:

- **C structs** for every node, list, enum, and flags type
- **Builder functions** that parser actions call to construct the tree
- **Debug printer** for dumping the AST
- **Formatter recipes** for pretty-printing SQL (the `fmt` blocks)

The data model (fields, types, storage) is the core. Formatting is an
optional annotation layered on top.

## A complete small example

Here's everything you need to represent and format `CAST(expr AS type)`:

```synq
node CastExpr {
  expr: index Expr
  type_name: inline SyntaqliteTextSpan

  fmt { "CAST" "(" child(expr) "AS" span(type_name) ")" }
}
```

This generates a C struct with two fields, a builder function the
parser calls when it reduces a CAST expression, a debug printer case,
and a formatter recipe.

**Fields** describe what data the node stores. Each has a name, a
storage class, and a type:

- `expr: index Expr`: an `index` field is a reference to another
  node in the AST arena. The generated struct stores a node index; the
  builder function takes a node index parameter.
- `type_name: inline SyntaqliteTextSpan`: an `inline` field is
  stored directly in the node struct. Source spans, enums, flags, and
  bools are all inline.

**The fmt block** is a recipe for the formatter. Items emit left to
right:

- `"CAST"`: a bare string emits a single SQL keyword
- `"("`: punctuation, sits flush against the previous keyword
- `child(expr)`: recursively formats a child node
- `"AS"`: another keyword; the formatter inserts spaces between
  adjacent word-class atoms automatically
- `span(type_name)`: prints the original source text of a span field
- `")"`: closing paren

Each keyword is a single SQL word: no embedded whitespace. The
formatter places one space between two adjacent word-class tokens at
render time; punctuation transitions need an explicit `line` if a
space is wanted there. Multiple items next to each other form a
sequence automatically.

## Enums

Enums define a fixed set of values. They become C enums in the
generated code:

```synq
enum SortOrder { ASC DESC }
```

Ordinal is the position (first = 0). Use them as field types:

```synq
node OrderingTerm {
  expr: index Expr
  sort_order: inline SortOrder
}
```

Wrap to multiple lines when there are many variants:

```synq
enum BinaryOp {
  PLUS MINUS STAR SLASH REM
  LT GT LE GE EQ NE
  AND OR BITAND BITOR LSHIFT RSHIFT
  CONCAT PTR
}
```

## Flags

When a node needs multiple independent boolean options packed into a
single integer, use flags:

```synq
flags SelectStmtFlags { DISTINCT = 1 }
flags CreateTableStmtFlags { WITHOUT_ROWID = 1 STRICT = 2 }
```

Values are bit positions. In the generated struct the field is a
`uint8_t`; the builder ORs bits together.

## Lists

A list is a linked sequence of child nodes. It just names its child
type:

```synq
list ExprList { Expr }
list OrderByList { OrderingTerm }
```

The generated code handles the linked-list plumbing. Lists without a
`fmt` block get default comma-separated formatting.

## Conditionals

Most SQL nodes vary their output based on which fields are populated.
Four conditional forms, all with the same shape:

```
if_xxx(args) { then_body }
if_xxx(args) { then_body } else { else_body }
```

**if_set**: is an index field non-null?

```synq
if_set(columns) { "(" child(columns) ")" }
```

**if_flag**: is a bool true or a flag bit set?

```synq
# Bool field
if_flag(is_temp) { "TEMP" }

# Flag bit: use dot notation
if_flag(flags.distinct) { "SELECT" "DISTINCT" } else { "SELECT" }
```

**if_enum**: does an enum equal a specific variant?

```synq
if_enum(sort_order, DESC) { "DESC" }
if_enum(kind, REINDEX) { "REINDEX" } else { "ANALYZE" }
```

**if_span**: is a source span non-empty?

```synq
if_span(schema) { span(schema) "." }
```

(No space appears between `span(schema)` and `"."` because `.` is
not a word-class character.)

Conditionals nest freely:

```synq
if_flag(flags.star) {
  if_set(expr) { child(expr) ".*" } else { "*" }
} else {
  child(expr)
}
```

## Line breaks

Three kinds, controlling how the formatter breaks lines:

- `line`: a space when the group fits on one line, a newline when broken
- `softline`: nothing when flat, a newline when broken
- `hardline`: always a newline

## group and nest: controlling layout

`group` marks a region the formatter tries to keep on one line. If it
doesn't fit, all `line`/`softline` breaks inside become newlines:

```synq
group { child(left) line "AND" line child(right) }
```

`nest` increases the indent level:

```synq
group { nest { line child(columns) } }
```

When the group breaks, `line` becomes an indented newline.

## clause: the SQL clause shorthand

SQL statements repeat this pattern constantly: "if a field is
present, start a new line, print the keyword, indent the body":

```synq
clause("WHERE", where)
```

This expands to:

```synq
if_set(where) { hardline "WHERE" nest { line child(where) } }
```

Multi-word clauses pass each keyword as its own argument:

```synq
clause("GROUP", "BY", groupby)
clause("ORDER", "BY", orderby)
```

Each keyword is a single SQL word: no embedded whitespace. The
formatter inserts the inter-word space at render time. This makes
SELECT-style statements very clean:

```synq
node SelectStmt {
  flags: inline SelectStmtFlags
  columns: index ResultColumnList
  from_clause: index Expr
  where: index Expr
  groupby: index ExprList
  having: index Expr
  orderby: index OrderByList
  limit_clause: index LimitClause
  window_clause: index NamedWindowDefList

  fmt {
    group {
      if_flag(flags.distinct) { "SELECT" "DISTINCT" } else { "SELECT" }
      if_set(columns) { group { nest { line child(columns) } } }
      clause("FROM", from_clause)
      clause("WHERE", where)
      clause("GROUP", "BY", groupby)
      clause("HAVING", having)
      clause("ORDER", "BY", orderby)
      clause("LIMIT", limit_clause)
      clause("WINDOW", window_clause)
    }
  }
}
```

## switch: branching on enum values

When different enum values need entirely different output, use switch.
Each case is `VARIANT { body }`:

```synq
switch(raise_type) {
  IGNORE   { "IGNORE" }
  ROLLBACK { "ROLLBACK" }
  ABORT    { "ABORT" }
  FAIL     { "FAIL" }
}
```

Cases can have multi-item bodies:

```synq
switch(op) {
  ISNULL  { child(left) "ISNULL" }
  IS      { child(left) "IS" child(right) }
  IS_NOT  { child(left) "IS" "NOT" child(right) }
}
```

Add `default` for a fallthrough:

```synq
switch(op) {
  AND { child(left) line "AND" line child(right) }
  OR  { child(left) line "OR"  line child(right) }
  default {
    group {
      child(left) line
      enum_display(op, { PLUS="+" MINUS="-" STAR="*" })
      line child(right)
    }
  }
}
```

## enum_display: mapping enum values to text

Emits a string based on the current value of an enum field:

```synq
enum_display(op, {
  PLUS="+" MINUS="-" STAR="*" SLASH="/"
  CONCAT="||" PTR="->"
})
```

Variants not listed produce no output.

## for_each: iterating list children

Use `child(_item)` for the current child during iteration.

Without a separator:

```synq
for_each { child(_item) }
```

With a separator (items between `sep:` and `)` are emitted between
children):

```synq
for_each(sep: "," line) { child(_item) }
```

The template can wrap each child:

```synq
for_each(sep: "," line) { "(" child(_item) ")" }
```

## Keyword spacing

Keyword literals are single SQL words: no leading, trailing, or
embedded whitespace. The renderer inserts a space between two
adjacent word-class atoms automatically:

```synq
"ORDER" "BY"            # → "ORDER BY"
"PRIMARY" "KEY"         # → "PRIMARY KEY"
```

Punctuation transitions do not auto-space:

```synq
"FOREIGN" "KEY" "("     # → "FOREIGN KEY("
"FILTER" "(" ...        # → "FILTER(..."  (use line if a space is wanted)
"FILTER" line "("       # → "FILTER ("    in flat mode
```

## Quick reference

```
"text"                                  single SQL keyword (no whitespace)
span(field)                             source span
child(field)                            child node (or _item in for_each)
line  softline  hardline                line breaks
group { ... }                           formatting group
nest { ... }                            indent nesting
if_set(f) { ... } [else { ... }]        test index field non-null
if_flag(f) { ... } [else { ... }]       test bool or flags.bit
if_enum(f, VAL) { ... } [else { ... }]  test enum == value
if_span(f) { ... } [else { ... }]       test span non-empty
clause("KW" [,"KW"]*, field)            SQL clause shorthand
switch(f) { CASE { ... } ... }          branch on enum value
enum_display(f, { K="v" ... })          map enum to string
for_each [(sep: items...)] { ... }      iterate list children
```
