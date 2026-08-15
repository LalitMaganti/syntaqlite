# Semantic Roles Plan

## Overview

The semantic analyzer currently uses two separate, partial systems to understand what SQL statements
mean:

1. **`AstTypes<'a>`**: a generated bundle of ~30 associated types representing the syntactic shape
   of a dialect's AST. Used by the `Walker` for expression validation, scope tracking, and column
   inference inside SELECT.

2. **`SchemaContribution`**: a small data-driven structure (node tag → field indices) recording
   which nodes contribute tables/views/functions to the catalog, and where to find the name and
   column list.

Both are wrong in the same way: they make the semantic layer responsible for understanding
dialect-specific structure. `AstTypes` does this via a Rust trait that encodes SQLite's AST shape.
`SchemaContribution` does this by handing the semantic layer field indices and then expecting the
semantic layer to walk them using `AstTypes`.

The fix is to invert this. The grammar files already carry `fmt { ... }` blocks declaring how to
*format* each node. They should also carry `semantic { ... }` blocks declaring what each node *does*.
The semantic engine becomes a data-driven role interpreter that reads those annotations and acts on
them. It never navigates AST nodes via typed Rust traits. `AstTypes`, `SchemaContribution`, and the
`Walker<A: AstTypes>` struct are all deleted.

The proposal generalizes an approach that the formatter already uses successfully.

---

## What's Wrong With `AstTypes`

### The two-halves problem

`DocumentCatalog::accumulate<A: AstTypes<'a>>` already has a split personality:

```rust
// Half 1: data-driven: asks the dialect "does this tag contribute to the schema?"
let Some(contrib) = dialect.schema_contribution_for_tag(tag) else { return; };
let name = fields[contrib.name_field as usize];

// Half 2: trait-driven: uses AstTypes to walk column structures and SELECT results
if let Some(col_field) = contrib.columns_field {
    columns_from_column_list::<A>(stmt_result, col_list_id, dialect, &mut columns);
}
if let Some(sel_field) = contrib.select_field {
    columns_from_select::<A>(stmt_result, sel_id, known, database, &mut columns);
}
```

The first half is right: the dialect owns the mapping from node tag to semantic intent. The second
half is wrong: `columns_from_column_list::<A>` and `columns_from_select::<A>` are generic functions
that assume column definitions look like SQLite's `ColumnDef` nodes (with `column_name`, `type_name`,
`constraints` fields containing `ColumnConstraint` nodes with `NotNull`/`PrimaryKey` kinds). This is
SQLite's structure. It is not Perfetto's structure.

### The concrete Perfetto example

```sql
-- SQLite
CREATE TABLE foo AS SELECT 1

-- Perfetto
CREATE PERFETTO TABLE foo(x INT, y LONG) AS SELECT 1 AS x, 2 AS y
```

Both add a table to the catalog. Their root AST nodes are `CreateTableStmt` and
`CreatePerfettoTableStmt` respectively. Both have `session_schema` annotations. But `CreatePerfettoTableStmt`
has a `schema` field (`PerfettoArgDefList`) that is its explicit column list: nodes of type
`PerfettoArgDef` with `arg_name` and `arg_type` fields, not `ColumnDef` nodes. The current
`columns_from_column_list::<A>` would fail to extract these correctly because it looks for a
`column_name` span field on each child, not `arg_name`.

More precisely: the current `session_schema` for `CreatePerfettoTableStmt` only passes
`as_select: select`: not the explicit column list at all. It relies entirely on SELECT inference,
which works for now, but loses the explicit type information (`INT`, `LONG`) that Perfetto declared.

This is a bug today and it illustrates the deeper issue: the semantic layer cannot correctly handle
dialect-specific column forms because it is trying to do so through a SQLite-shaped generic.

### The `walk_other_node` tell

The `Walker` already knows it cannot handle everything via `AstTypes`. The `walk_stmt` dispatch ends
with:

```rust
StmtKind::Other(node) => self.walk_other_node(node, scope),
```

`walk_other_node` recursively pokes at children, trying to match them as `A::Stmt` or `A::Expr`.
This already uses the transparent generic traversal represented by `Transparent` nodes in the proposed
system. For dialect-specific statements like `CREATE PERFETTO TABLE`, the walker falls into `Other`
and descends generically. The typed trait provides no value for those nodes. The fallback is
already doing the right thing, just implicitly.

### `AstTypes` is a redundant layer

The grammar files already have semantic field naming conventions. Fields are named `func_name`,
`column_name`, `table_name`, `alias`: not `field_0`, `field_1`. `AstTypes` is just a compiled Rust
view over information the grammar already carries. It adds a type-safe dispatch mechanism, but the
underlying data (which fields have what names and what roles) is already present in the grammar.
The question is whether the semantic layer reads it through a generated Rust trait or through
explicit grammar annotations. The latter puts the information where it belongs: in the grammar.

---

## The Existing Proof of Concept: `session_schema`

The `.synq` grammar files already have semantic annotations for DDL:

```synq
-- syntaqlite-syntax/parser-nodes/create_table.synq
node CreateTableStmt {
  table_name: inline SyntaqliteTextSpan
  columns: index ColumnDefList
  as_select: index Select
  ...
  session_schema { table(name: table_name, columns: columns, as_select: as_select) }
}

-- syntaqlite-syntax/parser-nodes/utility_stmts.synq
node CreateViewStmt {
  view_name: inline SyntaqliteTextSpan
  select: index Select
  ...
  session_schema { view(name: view_name, as_select: select) }
}

-- dialects/perfetto/nodes/perfetto.synq
node CreatePerfettoTableStmt {
  table_name: inline SyntaqliteTextSpan
  schema: index PerfettoArgDefList      -- explicit column list
  select: index Select
  ...
  session_schema { table(name: table_name, as_select: select) }
                                        -- bug: schema field ignored
}

node CreatePerfettoFunctionStmt {
  function_name: inline SyntaqliteTextSpan
  args: index PerfettoArgDefList
  ...
  session_schema { function(name: function_name, args: args) }
}

node IncludePerfettoModuleStmt {
  module_name: inline SyntaqliteTextSpan
  ...
  session_schema { import(name: module_name) }
}
```

The concept works. The problem is that it only covers DDL catalog contributions and the syntax
(`session_schema`) is narrow. The plan is to:

1. Rename `session_schema` to `semantic` (or just expand it inline)
2. Extend the vocabulary to cover expressions, source bindings, and scope structure
3. Drive the entire semantic engine from the resulting annotations
4. Fix the `CreatePerfettoTableStmt` bug by including the `schema` column field

---

## The `semantic { ... }` Block

Every node can optionally carry a `semantic { ... }` declaration. It replaces `session_schema { ... }`
everywhere and adds new roles for the full semantic surface. Nodes with no declaration are
implicitly **transparent**: the engine recurses into their children without special handling.

The vocabulary is **fixed and finite**. Dialects cannot invent new roles. A new dialect maps its
nodes onto the existing vocabulary. If a node has no matching role it is `transparent` and the
engine finds whatever annotated nodes are inside it.

### Catalog roles (replaces `session_schema`)

These declare what a DDL statement contributes to the catalog. Same concept as `session_schema`,
same codegen output, just a different syntax and corrected field references.

```synq
node CreateTableStmt {
  ...
  semantic { define_table(name: table_name, columns: columns, select: as_select) }
}

node CreateViewStmt {
  ...
  semantic { define_view(name: view_name, select: select) }
}

-- Perfetto: now correctly includes the explicit column list
node CreatePerfettoTableStmt {
  ...
  semantic { define_table(name: table_name, columns: schema, select: select) }
}

node CreatePerfettoFunctionStmt {
  ...
  semantic { define_function(name: function_name, args: args) }
}

node IncludePerfettoModuleStmt {
  ...
  semantic { import(module: module_name) }
}
```

The `define_table` role accepts an optional `columns` field and an optional `select` field. When
both are present (Perfetto with explicit columns AND an AS SELECT), explicit columns take precedence
for catalog storage: they are the declared schema. When only `select` is present (SQLite's `CREATE
TABLE foo AS SELECT ...`), the engine infers columns from the SELECT result.

**Column list heterogeneity.** SQLite's column list contains `ColumnDef` nodes; Perfetto's contains
`PerfettoArgDef` nodes. Both need to produce `ColumnDef` entries for the catalog. The engine does
not hardcode "how to read a column list": instead, column-list item nodes carry their own
annotation:

```synq
node ColumnDef {
  column_name: index Name
  type_name: inline SyntaqliteTextSpan
  constraints: index ColumnConstraintList
  ...
  semantic { column_def(name: column_name, type: type_name, constraints: constraints) }
}

node PerfettoArgDef {
  arg_name: inline SyntaqliteTextSpan
  arg_type: inline SyntaqliteTextSpan
  ...
  semantic { column_def(name: arg_name, type: arg_type) }
}
```

The engine, when processing a `define_table` with a `columns` field, walks the child list and
collects all `column_def` roles it finds. Each dialect's column node maps onto the same role with
the appropriate field pointers. The consumer is dialect-agnostic.

**`import` is a catalog effect, not a scope directive.** `INCLUDE PERFETTO MODULE 'foo.bar'` is a
statement that runs and brings whatever `foo.bar` exports into the database engine: similar to how
`CREATE TABLE` makes a table available for subsequent statements. It is not a file-level include or
a compile-time scope annotation. Semantically it is a catalog mutation. The analyzer handles it by
calling a **module resolver** (pluggable, provided by the caller) that returns what `foo.bar`
exports. If the resolver is not available, the import is acknowledged and the contributed names are
treated as unknown: analysis of subsequent statements continues without erroring on the import
itself.

### Expression roles

These are the nodes that produce values or reference names. They are the primary validation targets.

```synq
node FunctionCall {
  func_name: inline SyntaqliteTextSpan
  args: index ExprList
  filter_clause: index Expr
  over_clause: index WindowDef
  ...
  semantic { call(name: func_name, args: args) }
}

node ColumnRef {
  column: inline SyntaqliteTextSpan
  table: inline SyntaqliteTextSpan
  ...
  semantic { column_ref(column: column, table: table) }
}
```

`call` → the engine extracts the function name span and the argument count, then validates against
the catalog (unknown function, wrong arity). Argument expressions are also visited for nested
validation.

`column_ref` → the engine validates the column name against the current scope. If a table qualifier
is present, it also validates that the qualifier refers to a table in scope and that the column
exists on it.

Note: `AggregateFunctionCall` and `OrderedSetFunctionCall` are separate node types today but map
onto the same `call` role. The distinction between aggregate/scalar/window is enforced by the
catalog (via `FunctionCategory`) not by the role.

### Source roles

These are nodes that appear in FROM clauses and introduce names into the current scope.

```synq
node TableRef {
  table_name: inline SyntaqliteTextSpan
  alias: index Name
  ...
  semantic { source_ref(kind: table, name: table_name, alias: alias) }
}

node SubqueryTableSource {
  select: index Select
  alias: index Name
  ...
  semantic { scoped_source(body: select, alias: alias) }
}
```

`source_ref(kind: table, ...)` → the engine validates that `table_name` exists in the catalog as a
`table`-kinded relation. It then adds the table's columns to the current scope under the alias (or
table name if no alias). The `kind` field governs what catalog entries are valid here and what
diagnostic is emitted on mismatch.

`scoped_source` → the engine opens a fresh scope, visits the subquery body inside it, then
closes that scope. The columns produced by the subquery are bound under the alias in the *outer*
scope. The alias is mandatory for subquery sources (enforced by the grammar, not the engine).

`JoinClause` and `JoinPrefix` have no `semantic` block: they are `transparent`. The engine
recurses into their children and finds the `source_ref` and `scoped_source` nodes inside them, each
of which independently introduces its bindings into the current scope.

### Scope roles

These nodes structure the scoping environment for the statements inside them. They do not themselves
introduce names but control when and where inner bindings become visible.

```synq
node SelectStmt {
  from_clause: index TableSource
  columns: index ResultColumnList
  where_clause: index Expr
  groupby: index ExprList
  having: index Expr
  ...
  semantic { query(from: from_clause, exprs: [columns, where_clause, groupby, having]) }
}

node CteDefinition {
  cte_name: inline SyntaqliteTextSpan
  select: index Select
  ...
  semantic { cte_binding(name: cte_name, body: select) }
}

node WithClause {
  recursive: inline Bool
  ctes: index CteList
  select: index Select
  ...
  semantic { cte_scope(recursive: recursive, bindings: ctes, body: select) }
}
```

`query` → the engine first processes `from` (populating the scope with source bindings), then
validates all `exprs`. This ordering matters: column refs in `where_clause` can reference tables
introduced in `from_clause`.

`cte_binding` and `cte_scope` together implement the CTE model (see next section).

### Trigger scope

`CreateTriggerStmt` is the one node that introduces *implicit* bindings that do not come from
table refs or CTEs. The trigger body executes in a scope where `OLD` and `NEW` are bound with the
columns of the target table.

```synq
node CreateTriggerStmt {
  table: index QualifiedName
  when_expr: index Expr
  body: index TriggerCmdList
  ...
  semantic { trigger_scope(target: table, when: when_expr, body: body) }
}
```

`trigger_scope` → the engine resolves the target table's columns from the catalog, then opens a
scope with `OLD` and `NEW` bound to those columns. It validates `when_expr` and all statements in
`body` within that scope.

`trigger_scope` is the only role that injects implicit bindings. There is no general mechanism for
a dialect to declare "this scope has these magic names": `trigger_scope` is a fixed, named concept
in the engine vocabulary. Any future construct that injects implicit bindings would need its own
named role.

---

## Scoping Semantics

Scoping rules are **fixed and not configurable by dialects**. Any SQL dialect that diverges from
standard SQL scoping is not meaningfully a SQL dialect. syntaqlite's value, and
the reason users chose SQL, depends on predictable scope semantics. Making them extensible would add
complexity without benefit.

### CTE scope

The CTE model is the universal SQL scoping model for named subqueries:

- **Sequential binding.** In `WITH a AS (...), b AS (...) SELECT ...`, the body of `b` can
  reference `a` but not vice versa. Each binding is visible to all subsequent bindings in the same
  `WITH`.
- **Recursive opt-in.** `WITH RECURSIVE a AS (... UNION ALL ... FROM a ...)` makes `a` visible
  inside its own body. Non-recursive CTEs do not see themselves.
- **Main query visibility.** All CTEs are visible in the main query body regardless of order.
- **Shadowing.** A CTE name shadows any catalog name of the same name for the duration of the query.

The engine handles `cte_scope` by iterating `bindings` in order. For each `cte_binding`, it
processes the body with the current accumulated scope (previous CTEs visible, current CTE visible
only if `recursive` is set), then adds the new name to the accumulated scope before moving to the
next binding. After all bindings are processed, it visits the main `body` with the full CTE scope
active.

### Subquery scope

Subqueries (via `scoped_source`) have a fundamentally different relationship to the outer query
than CTEs. A bare subquery in FROM is anonymous until aliased. It runs in a **fresh scope** that
does not inherit from the outer query's bindings. After it closes, the alias becomes available in
the outer scope with the columns the subquery produces.

The exception is correlated subqueries in WHERE/HAVING: `SELECT ... WHERE id IN (SELECT id FROM
other WHERE other.parent_id = outer.id)`. Here `outer.id` references the outer query's scope.
Correlated subquery support is a future concern. For now, subqueries are treated as fully
independent scopes; any outer-column references inside them are not validated (they fall through
as unknown, which produces no diagnostic under non-strict mode).

### Why these rules are in the engine, not the grammar

CTE ordering semantics cannot be expressed as a simple per-node annotation because the rules govern
relationships *between* nodes in a list (each CTE sees previous CTEs). This is procedural logic, not
declarative metadata. The `cte_scope` annotation tells the engine "this is a CTE context" and the
engine applies the fixed rules. The annotation does not parameterise the rules.

### Why the rules are fixed for syntaqlite

All dialects syntaqlite supports are derived from SQLite and run on SQLite's query engine. SQLite
enforces standard CTE semantics at the engine level. A dialect overlay cannot change what the
underlying engine does: Perfetto runs on SQLite, and any future syntaqlite dialect will too. There
is no realistic scenario where a SQLite-derived dialect would deviate from the standard CTE scoping
model, because deviating would mean breaking the engine underneath it.

Some non-SQLite engines do deviate: BigQuery's `WITH RECURSIVE` allows forward references to CTEs
defined later in the same `WITH` clause, and SQL Server does not require the `WITH RECURSIVE` keyword
at all (recursion is detected implicitly). These are intentional design choices in those engines.
They are irrelevant to syntaqlite. If syntaqlite ever supported a non-SQLite-derived dialect, this
assumption would need revisiting, although that is not a current or planned concern.

---

## Relation Kinds

Every binding in scope has a **relation kind** alongside its column list. The engine-known kinds are:

```rust
enum RelationKind {
    Table,      // standard SQL table or view
    View,       // kept separate for catalog queries (e.g. "show all views")
    Interval,   // Perfetto: interval-structured data
    Tree,       // Perfetto: tree-structured data
    Graph,      // Perfetto: graph-structured data
}
```

These are not opaque labels. The engine uses them to validate usage context, drive completions, and
format diagnostics. A `source_ref(kind: table, ...)` in the grammar means "only a `table`-kinded
relation is valid here". If the user writes:

```sql
SELECT * FROM my_interval_data
```

where `my_interval_data` is an interval-kinded relation defined as a `CREATE PERFETTO TABLE`, the
engine emits:

```
error: `my_interval_data` is an interval, not a table
  → use FLATTEN(my_interval_data) to convert it to a table
```

**Explicit conversion only.** There is no implicit coercion between kinds. This is a deliberate
user-experience choice: interval/tree/graph objects are conceptually distinct from tables. A user
who stumbles across one deserves to be told what it is and how to use it, not silently given a
table-shaped view of it. Explicit conversion via dialect-provided functions (`FLATTEN`, or whatever
the Perfetto convention ends up being) surfaces the concept and teaches the model.

**Dialect extensibility.** New relation kinds are added to the `RelationKind` enum as dialects
introduce new structured data types. This requires an engine change, not just a grammar annotation.
The vocabulary of kinds is bounded and engine-known because the engine must know how to emit
meaningful diagnostics for each. An unknown kind cannot produce a useful error message.

**Kind-aware completions.** In a position that accepts intervals, the completion list filters to
interval-kinded names. In a standard FROM, it filters to table/view-kinded names. This falls out
naturally from the `source_ref(kind: ...)` annotations.

---

## Column Inference for `define_table` with `select`

When a `define_table` has a `select` field but no `columns` field (e.g. `CREATE TABLE foo AS
SELECT ...`), the engine needs to infer the columns that `foo` will expose to subsequent
statements. This is the forward-pass accumulation that currently lives in `columns_from_select`.

The engine handles this during the `DefineRelation` processing pass (the forward pass that builds
`DocumentCatalog`). When it encounters `DefineTable { select: some_id, columns: None }`:

1. It visits the SELECT subtree looking for the top-level result columns (not recursing into
   subqueries).
2. For each result column with an explicit alias, the alias becomes a column name.
3. For each result column that is a bare `column_ref` with no alias, the column name is used.
4. For `SELECT *` or `SELECT t.*`, it expands using columns from the `known` map (relations
   accumulated so far in the document) or the external `DatabaseCatalog`.
5. Result columns that are expressions without aliases (e.g. `SELECT 1`, `SELECT a + b`) produce
   no column entry because they are not usefully addressable by name.

This logic is identical to what `columns_from_select` does today. The difference is that it no
longer requires `A: AstTypes`: instead it walks the SELECT subtree using the role table, finding
`column_ref` and `star_expansion` roles rather than calling typed accessors on `A::SelectStmt`.

`ResultColumn` needs an annotation to support star expansion:

```synq
node ResultColumn {
  flags: inline ResultColumnFlags   -- STAR flag
  alias: index Name
  expr: index Expr
  ...
  semantic { result_column(star: flags.star, alias: alias, expr: expr) }
}
```

The engine's column inference reads `result_column` roles and applies the alias/column-ref/star
logic above.

---

## Generated Role Table

The codegen processes all `semantic { ... }` blocks and emits a `SemanticRole` table per dialect:
a static array indexed by node tag, one entry per node type:

```rust
pub(crate) enum SemanticRole {
    // Catalog: replaces SchemaContribution
    DefineTable    { name: FieldIdx, columns: Option<FieldIdx>, select: Option<FieldIdx> },
    DefineView     { name: FieldIdx, select: FieldIdx },
    DefineFunction { name: FieldIdx, args: Option<FieldIdx> },
    Import         { module: FieldIdx },

    // Column list items: used during define_table column extraction
    ColumnDef      { name: FieldIdx, type_: Option<FieldIdx>, constraints: Option<FieldIdx> },

    // Result columns: used during SELECT column inference
    ResultColumn   { star: FieldIdx, alias: FieldIdx, expr: FieldIdx },

    // Expressions
    Call           { name: FieldIdx, args: FieldIdx },
    ColumnRef      { column: FieldIdx, table: FieldIdx },

    // Sources
    SourceRef      { kind: RelationKind, name: FieldIdx, alias: FieldIdx },
    ScopedSource   { body: FieldIdx, alias: FieldIdx },

    // Scope structure
    Query          { from: FieldIdx, exprs: &'static [FieldIdx] },
    CteBinding     { name: FieldIdx, body: FieldIdx },
    CteScope       { recursive: FieldIdx, bindings: FieldIdx, body: FieldIdx },
    TriggerScope   { target: FieldIdx, when: FieldIdx, body: FieldIdx },

    // No semantic role: recurse into children
    Transparent,
}
```

`FieldIdx` is the index into the node's field array (same indexing used by `extract_fields` today).
The codegen also emits constraint-kind metadata for `ColumnConstraint` nodes so the engine can
extract `NOT NULL` and `PRIMARY KEY` without needing `A::ColumnConstraint`:

```rust
pub(crate) enum ConstraintRole {
    PrimaryKey,
    NotNull,
    Other,
}
```

The dialect's static data gains:

```rust
pub(crate) struct AnyDialect {
    grammar: AnyGrammar,
    fmt_strings: ...,
    fmt_ops: ...,
    fmt_dispatch: ...,
    roles: &'static [SemanticRole],          // new: replaces schema_contributions
    constraint_roles: &'static [ConstraintRole], // new: for column constraint extraction
}
```

`schema_contributions: &'d [SchemaContribution]` and `dialect.schema_contribution_for_tag(tag)` are
removed from `AnyDialect` and the public API.

---

## The Semantic Engine

`Walker<A: AstTypes>` is deleted. The semantic engine is a single struct that interprets the role
table:

```rust
struct SemanticEngine<'a> {
    stmt: AnyParsedStatement<'a>,
    roles: &'static [SemanticRole],
    catalog: &'a CatalogStack<'a>,
    scope: ScopeStack,
    config: ValidationConfig,
    diagnostics: Vec<Diagnostic>,
}

impl SemanticEngine<'_> {
    fn visit(&mut self, node_id: AnyNodeId) {
        if node_id.is_null() { return; }
        let tag = self.stmt.tag_of(node_id);
        match self.roles.get(tag as usize).unwrap_or(&SemanticRole::Transparent) {
            SemanticRole::Call { name, args } => self.visit_call(node_id, *name, *args),
            SemanticRole::ColumnRef { column, table } => self.visit_column_ref(node_id, *column, *table),
            SemanticRole::SourceRef { kind, name, alias } => self.visit_source_ref(node_id, *kind, *name, *alias),
            SemanticRole::ScopedSource { body, alias } => self.visit_scoped_source(node_id, *body, *alias),
            SemanticRole::Query { from, exprs } => self.visit_query(node_id, *from, exprs),
            SemanticRole::CteScope { recursive, bindings, body } => self.visit_cte_scope(node_id, *recursive, *bindings, *body),
            SemanticRole::TriggerScope { target, when, body } => self.visit_trigger_scope(node_id, *target, *when, *body),
            SemanticRole::Transparent => self.visit_children(node_id),
            // Catalog roles handled separately in the accumulation pass, not here.
            _ => {}
        }
    }

    fn visit_children(&mut self, node_id: AnyNodeId) {
        let children: Vec<_> = self.stmt.child_node_ids(node_id).collect();
        for child in children {
            self.visit(child);
        }
    }
}
```

The engine runs in two passes over the statement list:

1. **Accumulation pass.** Forward pass over all statements. For each statement, reads catalog roles
   (`DefineTable`, `DefineView`, `DefineFunction`, `Import`) and populates `DocumentCatalog`. This
   pass must happen before validation so that forward references within a file work (a view defined
   on line 50 referencing a table defined on line 10 is valid).

2. **Validation pass.** For each statement, runs the `visit` method to validate expressions, column
   refs, function calls, and scope structure against the now-complete `DocumentCatalog`.

The two-pass model replaces the current combined accumulate-and-validate loop. The current approach
accumulates as it validates, which means the `DocumentCatalog` is only partially built at any point
during validation. Two explicit passes is cleaner.

The same non-generic engine code runs for every dialect without `AstTypes`.

---

## What Gets Deleted

| Deleted | Replaced by |
|---------|-------------|
| `AstTypes<'a>` trait and all ~30 `*Like`/`*View` sub-traits in `ast_traits.rs` | `semantic { ... }` grammar annotations + generated `SemanticRole` table |
| `Walker<A: AstTypes>` struct and all `walk_*` methods | `SemanticEngine::visit` and role-specific handlers |
| `checks.rs` (called from `Walker`) | Inlined into `SemanticEngine` role handlers |
| `SchemaContribution` struct in `dialect/schema.rs` | `SemanticRole::Define*` variants |
| `dialect.schema_contribution_for_tag(tag)` | `dialect.role_for_tag(tag)` |
| `schema_contributions: &[SchemaContribution]` field on `AnyDialect` | `roles: &[SemanticRole]` |
| `session_schema { ... }` syntax in `.synq` files | `semantic { define_* ... }` |
| `DocumentCatalog::accumulate::<A: AstTypes>` | `accumulate` with no type parameter, reads `SemanticRole` |
| `columns_from_column_list::<A>` | Engine reads `column_def` roles from the column list |
| `columns_from_select::<A>` | Engine reads `result_column` roles from the SELECT |
| `extract_column_constraints::<A>` | Engine reads `constraint_roles` table from dialect |

The generated typed node accessor structs (`FunctionCallView`, `ColumnRefView`, etc.) may be kept
as a convenience API for formatter code and dialect-specific tests: they are generated from the
same grammar and remain accurate. But they are no longer part of the semantic interface. The
`AstTypes` bundle trait that groups them together is deleted because nothing in the semantic layer
needs it.

---

## Migration Path

The migration is staged so each step compiles and tests pass throughout.

1. **Extend the `.synq` parser** to accept `semantic { ... }` blocks alongside `session_schema`.
   Parse `session_schema` as a deprecated alias for `semantic { define_* }` so no existing files
   break.

2. **Extend codegen** to emit `SemanticRole` tables. Initially all non-DDL nodes emit `Transparent`.
   The dialect gains a `roles` field. Both `roles` and `schema_contributions` coexist.

3. **Rewrite `DocumentCatalog::accumulate`** to read `SemanticRole::Define*` instead of
   `SchemaContribution`. Delete `SchemaContribution` and `session_schema` parsing. Migrate all
   `.synq` files from `session_schema` to `semantic { define_* }`.

4. **Write `SemanticEngine`** with the accumulation pass and validation pass. Initially it handles
   only catalog roles. Run it alongside `Walker<A>` and verify identical diagnostic output.

5. **Annotate expression and source nodes** (`FunctionCall`, `ColumnRef`, `TableRef`,
   `SubqueryTableSource`, `ResultColumn`, `ColumnDef`, `ColumnConstraint`) with `semantic { ... }`
   blocks. Extend codegen to emit the full role vocabulary.

6. **Implement all `SemanticEngine` role handlers** and extend the validation pass to handle
   expressions, sources, and scope roles.

7. **Delete `Walker<A: AstTypes>`** once `SemanticEngine` produces identical diagnostics.

8. **Annotate scope nodes** (`SelectStmt`, `WithClause`, `CteDefinition`, `CreateTriggerStmt`).
   Verify scope-sensitive diagnostics (unknown column, wrong table qualifier) still work.

9. **Delete `AstTypes`** trait and all generated `*View` structs that were only used by the
   semantic layer. The formatter uses its own bytecode system and does not need them.

---

## Implementation Journal

### Step 1: ✅ Done (commit `65f734e`)

`.synq` parser extended to accept `semantic { ... }` blocks. `session_schema { ... }` is parsed as
a deprecated alias that maps to the same `SemanticRole` variants (`DefineTable`, `DefineView`,
`DefineFunction`, `Import`). All existing `.synq` files continue to use `session_schema` syntax
without change.

`SemanticRole` enum defined in `syntaqlite/src/dialect/schema.rs` with catalog roles only.
`AnyDialect` gains a `roles: &'static [SemanticRole]` field and `roles()` accessor.
`SchemaContribution` struct deleted; `accumulate_ddl` in `catalog.rs` reads `SemanticRole::Define*`
from `dialect.roles()`. The `Walker<A: AstTypes>` remains as the sole validation engine.

### Step 2: ✅ Done (commit `42e2c6c`)

`generate_rust_semantic_roles()` wired into the `codegen-sqlite` stage 2 pipeline:

- `RustCodegenArtifacts` gains `semantic_roles_rs: Option<String>`; generated in
  `generate_codegen_artifacts` by calling `generate_rust_semantic_roles(&ast_model, "SQLITE")`
- `OutputLayout` gains `semantic_roles_rs: Option<String>`; set to
  `Some("syntaqlite/src/sqlite/semantic_roles.rs")` in `for_sqlite`, `None` elsewhere
- `syntaqlite/src/sqlite/semantic_roles.rs` is now a real generated file (82 entries):
  `CreateTableStmt` → `DefineTable { name: 0, columns: Some(5), select: Some(7) }`;
  `CreateViewStmt` → `DefineView { name: 0, select: 5 }`; all others → `Transparent`
- Array is 1-indexed (sentinel `Transparent` at index 0) to match the node-tag convention used by
  `fmt_dispatch` in `AnyDialect`
- `generated-files.txt` updated

Three `#[ignore]` tests in `analyzer.rs` (DDL accumulation) are now live and passing. All 53
`syntaqlite` unit tests pass.

**What step 2 does NOT do** (per the plan, deferred to step 3):
- `.synq` files are still on `session_schema` syntax; not migrated to `semantic { define_* }`
- `accumulate_ddl` still carries an `<A: AstTypes<'a>>` generic parameter used only by the
  `columns_from_select` SELECT-inference fallback
- `session_schema` parsing in the `.synq` parser is still present (needed until step 3 migrates
  all files)

### Step 3: ✅ Done (commit `0c8c150`)

All `.synq` files migrated from `session_schema { ... }` to `semantic { define_* }` syntax:

- `create_table.synq`: `session_schema { table(name: table_name, columns: columns, as_select: as_select) }`
  → `semantic { define_table(name: table_name, columns: columns, select: as_select) }`
- `utility_stmts.synq`: `session_schema { view(name: view_name, as_select: select) }`
  → `semantic { define_view(name: view_name, select: select) }`
- `perfetto.synq` (4 nodes): all `session_schema` blocks replaced with `semantic { define_* }`.
  `CreatePerfettoTableStmt` bug fixed: `columns: schema` now included so the explicit
  `PerfettoArgDefList` is captured rather than relying solely on AS SELECT inference.

`PerfettoArgDef.arg_name` changed from `inline SyntaqliteTextSpan` to `index Name` to match
`ColumnDef.column_name`, giving `columns_from_column_list` a single NodeId-based code path for
both dialects. The four `perfetto_arg_def_list_ne` parser action rules updated to wrap the name
token with `synq_parse_ident_name(pCtx, synq_span(pCtx, N))`.

`session_schema` parsing deleted from `synq_parser.rs`: `parse_legacy_schema` method removed,
call site removed, associated tests removed. `semantic_roles_codegen.rs` legacy test removed.

`accumulate_ddl` and `accumulate_ddl_into_database` are no longer generic:
- `<A: AstTypes<'a>>` parameter removed from both functions
- `extract_columns` simplified: explicit column-list path retained; SELECT-based inference
  removed (deferred to step 5 when `result_column` role annotations are added). AS-SELECT-only
  definitions register with `None` columns: column refs against them are conservatively accepted.
- `columns_from_select`, `collect_from_sources`, `name_str`, `FromSource`, `expand_star`,
  `lookup_columns` all deleted from `catalog.rs`
- All `ast_traits` imports removed from `catalog.rs`

Codegen re-run; `syntaqlite/src/sqlite/semantic_roles.rs` unchanged (field indices identical).
101 unit tests pass.

### Step 4: ✅ Done

`SemanticEngine` struct introduced in `syntaqlite/src/semantic/engine.rs`. It holds a reference to
`AnyParsedStatement`, the `&'static [SemanticRole]` table, the `Catalog`, `ValidationConfig`, and
a `Vec<Diagnostic>`. The `run` entry point calls `visit(root_id)`, which dispatches on
`SemanticRole`:

- `DefineTable / DefineView / DefineFunction / Import`: handled by the accumulation pass only;
  validation pass is a no-op (returns without recursing).
- All other roles → `visit_children`, recursing via `stmt.child_node_ids(node_id)`.

`engine.rs` added to `semantic/mod.rs` under `#[cfg(feature = "validation")]`.
`analyzer.rs` calls `SemanticEngine::run` after `Walker<A>::run` and extends `diagnostics` with the
result. At this step the engine produces no new diagnostics and runs in parallel with the Walker
and agrees on all 53 unit tests.

### Step 5: ✅ Done

Full semantic role vocabulary annotated in `.synq` files and emitted by codegen:

- `aggregate.synq`: `AggregateFunctionCall`, `OrderedSetFunctionCall` → `semantic { call(name: func_name, args: args) }`
- `column_ref.synq`: `ColumnRef` → `semantic { column_ref(column: column, table: table) }`
- `create_table.synq`: `ColumnDef` → `semantic { column_def(name: column_name, type: type_name, constraints: constraints) }`
- `cte.synq`: `CteDefinition` → `semantic { cte_binding(name: cte_name, body: select) }`,
  `WithClause` → `semantic { cte_scope(recursive: recursive, bindings: ctes, body: select) }`
- `functions.synq`: `FunctionCall` → `semantic { call(name: func_name, args: args) }`
- `select.synq`: `ResultColumn` → `semantic { result_column(flags: flags, alias: alias, expr: expr) }`,
  `SelectStmt` → `semantic { query(from: from_clause, columns: columns, where_clause: where_clause, groupby: groupby, having: having, orderby: orderby, limit_clause: limit_clause) }`
- `table_source.synq`: `TableRef` → `semantic { source_ref(kind: table, name: table_name, alias: alias) }`,
  `SubqueryTableSource` → `semantic { scoped_source(body: select, alias: alias) }`
- `trigger.synq`: `CreateTriggerStmt` → `semantic { trigger_scope(target: table, when: when_expr, body: body) }`
- `dialects/perfetto/nodes/perfetto.synq`: `PerfettoArgDef` → `semantic { column_def(name: arg_name, type: arg_type) }`

`SemanticRole` enum extended with: `ColumnDef`, `ResultColumn`, `Call`, `ColumnRef`, `SourceRef`,
`ScopedSource`, `Query`, `CteBinding`, `CteScope`, `TriggerScope`. `RelationKind` enum added
(`Table`, `View`, `Interval`, `Tree`, `Graph`).

`synq_parser.rs` extended to parse all new role names. `source_ref`'s `kind` param is a literal
enum value (not a field name): handled via `literal_keys: &["kind"]` in `parse_semantic_params`.
`semantic_roles_codegen.rs` updated to emit all new variants, with `SourceRef` mapping the literal
string to `RelationKind::*`.

`engine.rs` match arm extended: all new variants fall through to `visit_children` as placeholder
until Step 6 handlers are added. `catalog.rs` match arms use `_ => {}` wildcard to stay
non-exhaustive-safe.

Codegen regenerated; `semantic_roles.rs` now has 13 non-Transparent entries. All 53 unit tests pass.

### Steps 6+7: ✅ Done (commit `861eb24`)

All role handlers implemented in `SemanticEngine`:

- `visit_source_ref`: validates table/function name against catalog, emits `UnknownTable` with
  fuzzy suggestion, registers alias in query scope via `add_query_table`.
- `visit_call`: counts args via `list_children`, checks via `check_function`, emits
  `UnknownFunction` / `FunctionArity`, then recurses into children.
- `visit_column_ref`: calls `resolve_column(table, column)`, emits `UnknownColumn` for
  `TableFoundColumnMissing` and `NotFound` results.
- `visit_scoped_source`: pushes scope, visits body, pops scope, registers alias in outer scope.
- `visit_query`: visits FROM first (to populate scope with table bindings), then columns,
  WHERE, GROUP BY, HAVING, ORDER BY, LIMIT in order.
- `visit_cte_scope`: iterates CTE bindings; for recursive, pre-registers name; visits each body
  in a fresh scope; registers name in outer scope after body; then visits the main body.
- `visit_trigger_scope`: pushes scope, adds OLD/NEW pseudo-tables, visits when-expr and body.

Key fix: list nodes (`ResultColumnList`, `ExprList`, `OrderByList`, etc.) are detected at the top
of `visit` via `list_children`. If found, their children are visited directly rather than going
through the empty `child_node_ids` path. This ensures nested expressions inside SELECT columns,
ORDER BY, GROUP BY etc. are reachable.

`Walker<A: AstTypes>` removed from `analyzer.rs`. `AstTypes` type parameter removed from
`analyze_inner`. `SemanticEngine` is now the sole validation engine.

All 53 unit tests pass.

### Step 8: ✅ Done (commit `26898de`)

`walker.rs` deleted. `mod walker` removed from `semantic/mod.rs`. `Walker<A: AstTypes>` was the
only caller of the `AstTypes` trait inside the `syntaqlite` crate.

All 53 unit tests pass.

### Step 9: ✅ Done

`AstTypes` trait and all generated `*Like` / `*View` / `*Kind` trait infrastructure removed:

**`syntaqlite-syntax`:**
- `src/ast_traits.rs` deleted (the generated trait file)
- `pub mod ast_traits;` removed from `src/lib.rs`
- All 33 `impl crate::ast_traits::*Like/View/AstTypes/NodeLike` blocks removed from `src/sqlite/ast.rs`
- `pub struct SqliteAstMarker;` removed from `src/sqlite/ast.rs`

**`syntaqlite-buildtools`:**
- `generate_ast_traits()` function and its call site removed from `commands.rs`
- `actions_dir` / `nodes_dir` fields removed from `SqliteParserCodegen` struct and CLI args
- `ast_traits_rs: Option<String>` removed from `RustCodegenArtifacts` and `OutputLayout`
- `emit_rust_value_enum_like_trait`, `emit_rust_flags_like_trait`, `emit_rust_value_enum_like_impl`,
  `emit_rust_flags_like_impl` functions removed from `rust_ast.rs`
- `generate_ast_traits()` method and its helpers (`resolve_kind_enum_list_type`,
  `resolve_generic_element_type`, `trait_field_return_type`) removed from `rust_ast.rs`
- Trait impl emission (`AstTypes`, `NodeLike`, `XxxLike`, `XxxView`) removed from
  `generate_rust_ast()` in `rust_ast.rs`
- `grammar_fn_path` field removed from `RustAstPaths`; `dialect_name` param removed from
  `generate_rust_ast` signature

**`python/tools/run_codegen.py`:**
- `--actions-dir` and `--nodes-dir` args removed from stage 1b (`codegen-sqlite-parser`) invocation

All 53 unit tests pass.

---

## Open Questions

**Virtual tables.** ✅ Done. `CreateVirtualTableStmt` now carries
`semantic { define_table(name: table_name) }`. The table is registered in the catalog as
known-to-exist with unknown columns; `SELECT * FROM fts` no longer triggers `UnknownTable` after
a `CREATE VIRTUAL TABLE fts USING fts5(...)` in the same document.

**`PerfettoArgDef` for function return types.** `CreatePerfettoFunctionStmt` has a `return_type`
field that is either a scalar type or a `TABLE(col1 TYPE, col2 TYPE, ...)`. Table-returning
functions in Perfetto expose named columns. These should be stored in the catalog so that
`SELECT * FROM my_table_function(...)` can infer columns. This requires either a `define_table_function`
role or extending `define_function` to carry return-column info. Deferred.

**Correlated subqueries.** The current two-pass/fresh-scope model treats subqueries as fully
independent. A future pass could support correlated references by making the outer scope available
(read-only) inside subquery validation. Not needed for initial implementation.

**`DROP` and `ALTER` effects.** `DropStmt` and `AlterTableStmt` currently have no `session_schema`
and the accumulation pass ignores them. In a multi-file or incremental analysis context, drops and
renames matter. For now, `Transparent` is correct because these nodes contribute nothing to the forward catalog.
Future work: `semantic { drop_table(target: target) }` etc.
