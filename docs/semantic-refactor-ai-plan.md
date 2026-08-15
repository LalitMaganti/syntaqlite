# Semantic Module Refactor Plan

## Context

The `semantic/` module is mid-refactor. The active surface (`catalog.rs`, `diagnostics.rs`,
`schema.rs`, `mod.rs`) has a catalog data layer but no analysis engine. The engine lives in
`semantic/legacy/`: parked, unregistered, not compiling. There are several concrete problems:

1. **Too many catalog abstractions.** `DatabaseCatalog`, `DocumentCatalog`, `StaticCatalog`,
   `CatalogStack`, and `CatalogLayer` all exist. The first four are redundant wrappers that the
   refactor intended to replace with `CatalogLayer` + `Catalog` directly, but this was never
   completed.

2. **`CatalogStack` rebuilds the world on every lookup.** Every call to `check_function`,
   `resolve_relation`, etc. calls `build_catalog()`, which clones all three layers into a fresh
   `Catalog`. This is per-lookup, not per-analysis-pass.

3. **Double parse.** The analyzer parses SQL twice: once with `with_collect_tokens(true)` in
   `prepare()` to collect token positions, and again in `diagnostics_prepared()` to walk the AST.
   A single parse session provides both `stmt.tokens()` and `stmt.root()` on the same statement.

4. **`ScopeStack` duplicates catalog logic.** The scope stack tracks query-local tables (CTEs,
   aliases) with `Option<Vec<String>>` column lists. This is structurally identical to what
   `CatalogLayer` does, but lives in a separate abstraction with its own resolution path.

5. **`Walker::run` signature mismatch.** `legacy/analyzer.rs` calls `Walker::run` with 5 arguments
   (catalog and config separate). `legacy/walker.rs` defines `run` with 4 (catalog and config
   bundled in `WalkContext`). This is a mid-refactor breakage: the legacy code does not compile.

6. **`render.rs` hardcodes stderr.** `DiagnosticRenderer` calls `eprintln!` directly. The caller
   should control where output goes.

7. **`schema.rs` exists only to serve the deleted types.** `RelationDef`, `ColumnDef`,
   `FunctionDef` are DTOs for `DatabaseCatalog` and `DocumentCatalog`. Once those go away, so does
   the file.

This refactor completes the intended direction: one named-layer `Catalog`, single-pass analysis,
no redundant abstractions.

---

## Design Decisions

| Decision | Choice |
|----------|--------|
| Catalog representation | `CatalogLayer` (`pub(crate)`) + `Catalog` with named layer slots (public) |
| Named layers | `dialect`, `database`, `document`, `query`: fixed slots, not a raw `Vec` |
| `CatalogLayer` visibility | `pub(crate)`: callers never touch it |
| Public schema input API | `Catalog::add_table` / `add_view` / `add_function`: no DTOs, no `DatabaseCatalog` |
| Scope resolution | Merged into `Catalog`: query layer stack replaces `ScopeStack` entirely |
| Unknown columns | `Option<Vec<String>>` in `RelationEntry`: `None` means "table known, columns unknown, suppress column errors" |
| Parse passes | Single pass: tokens and AST walk in the same session loop |
| `SemanticModel` | Stores tokens + diagnostics from one pass; no prepare/diagnostics split |
| `checks.rs` | Inlined into `walker.rs`: three thin functions, no independent reuse |
| `scope.rs` | Deleted: replaced by `Catalog` query layer |
| `schema.rs` | Deleted: types served only the deleted catalog wrappers |
| `render.rs` | Write to `impl Write`, not stderr |
| LSP module | Out of scope for this refactor |

---

## Part 1: Catalog

### Internal representation

```rust
// pub(crate): callers never see this type
#[derive(Debug, Default, Clone)]
pub(crate) struct CatalogLayer {
    relations:       HashMap<String, RelationEntry>,
    functions:       HashMap<String, FunctionSet>,
    table_functions: HashMap<String, TableFunctionSet>,
}

#[derive(Debug, Clone)]
struct RelationEntry {
    name:    String,
    columns: Option<Vec<String>>, // None = exists but columns unknown
}

#[derive(Debug, Clone)]
struct FunctionSet {
    name:      String,
    overloads: Vec<FunctionOverload>,
}

/// A table-valued function: callable in FROM position, exposes named output columns.
#[derive(Debug, Clone)]
struct TableFunctionSet {
    name:           String,
    overloads:      Vec<FunctionOverload>,
    output_columns: Vec<String>, // columns produced when used in FROM
}

#[derive(Debug, Clone, Copy)]
pub(crate) struct FunctionOverload {
    pub category: FunctionCategory,
    pub arity:    AritySpec,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum FunctionCategory { Scalar, Aggregate, Window }

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub(crate) enum AritySpec {
    Exact(usize),
    AtLeast(usize),
    Any,
}
```

`columns: Option<Vec<String>>` is the key change from today. `Some(vec)` means the column list is
known and will be validated. `None` means the table exists but column references should not be
flagged: used for subquery aliases and CTE names where we can't statically infer output columns.

### Named-layer Catalog

```rust
pub struct Catalog {
    dialect:  CatalogLayer,       // dialect built-ins; set once at construction, never mutated
    database: CatalogLayer,       // user-provided schema
    document: CatalogLayer,       // DDL accumulated during current analysis pass
    query:    Vec<CatalogLayer>,  // stack of query-local scopes (subqueries, CTEs, aliases)
}
```

Resolution order for all lookups: `query` (innermost frame first) → `document` → `database` →
`dialect`. This matches the expected SQL scoping: a CTE name shadows a table name; a locally
created table shadows a user-provided one.

### Resolution results

```rust
#[derive(Debug, PartialEq, Eq)]
pub(crate) enum ColumnResolution {
    /// Column found in the expected table (or in any table if unqualified).
    Found,
    /// Table was found in scope but the column is not in its column list.
    TableFoundColumnMissing,
    /// The qualifier table is not in scope at all: the table check already
    /// reported this, so suppress the column error.
    TableNotFound,
    /// Unqualified column not found in any table in scope.
    NotFound,
}

#[derive(Debug)]
pub(crate) enum FunctionCheckResult {
    Ok,
    Unknown,
    WrongArity { expected: Vec<usize> },
}
```

### Public API (caller-facing)

Callers build a `Catalog` and pass it to the analyzer. They never see `CatalogLayer`.

```rust
impl Catalog {
    /// Create an empty catalog (no dialect builtins: those are added internally
    /// by the analyzer at construction from the dialect).
    pub fn new() -> Self;

    // ── Database layer: caller populates these ──────────────────────────────

    pub fn add_table(&mut self, name: &str, columns: &[&str]);
    pub fn add_view(&mut self, name: &str, columns: &[&str]);
    pub fn add_function(&mut self, name: &str, args: Option<usize>);
    /// Register a table-valued function. `output_columns` lists the column names
    /// the function exposes when used in a FROM clause (e.g. `json_each`).
    /// Pass an empty slice when output columns are not statically known.
    pub fn add_table_function(&mut self, name: &str, output_columns: &[&str]);
    pub fn clear_database(&mut self);

    // ── Convenience constructors ─────────────────────────────────────────────

    /// Parse DDL statements from `source` and populate the database layer.
    #[cfg(feature = "sqlite")]
    pub fn from_ddl(source: &str) -> Self;

    /// Parse a JSON schema description into the database layer.
    ///
    /// Expected format:
    /// ```json
    /// {
    ///   "tables": [{ "name": "users", "columns": ["id", "name"] }],
    ///   "views":  [{ "name": "active_users", "columns": ["id"] }],
    ///   "functions": [{ "name": "my_func", "args": 2 }]
    /// }
    /// ```
    #[cfg(feature = "json")]
    pub fn from_json(s: &str) -> Result<Self, String>;
}
```

### Internal API (used by analyzer and walker)

```rust
impl Catalog {
    // ── Dialect / database layers: set by analyzer ──────────────────────────

    /// Set the dialect layer. Called once at `SemanticAnalyzer` construction.
    pub(crate) fn set_dialect_layer(&mut self, layer: CatalogLayer);

    /// Replace the database layer with the caller's schema for the current pass.
    pub(crate) fn set_database_layer(&mut self, layer: CatalogLayer);

    // ── Document layer: managed by analyzer between statements ─────────────
    pub(crate) fn accumulate_ddl<A: AstTypes>(
        &mut self,
        stmt: AnyParsedStatement<'_>,
        root: AnyNodeId,
        dialect: Dialect,
    );
    pub(crate) fn clear_document(&mut self);

    // ── Query layer: managed by walker during AST traversal ────────────────

    /// Push a new empty scope frame. Called on subquery / CTE entry.
    pub(crate) fn push_query_scope(&mut self);

    /// Pop the innermost scope frame. Called on subquery / CTE exit.
    pub(crate) fn pop_query_scope(&mut self);

    /// Register a table or alias in the current (innermost) query scope.
    /// `columns = None` means the table exists but column list is unknown
    /// (e.g. a subquery alias we couldn't type, a CTE with complex body).
    pub(crate) fn add_query_table(&mut self, name: &str, columns: Option<Vec<String>>);

    // ── Resolution: searches all layers in priority order ───────────────────

    pub(crate) fn resolve_relation(&self, name: &str) -> bool;
    /// Returns `true` if `name` is a known table-valued function in any layer.
    pub(crate) fn resolve_table_function(&self, name: &str) -> bool;
    pub(crate) fn resolve_column(
        &self,
        table: Option<&str>,
        column: &str,
    ) -> ColumnResolution;
    pub(crate) fn check_function(
        &self,
        name: &str,
        arg_count: usize,
    ) -> FunctionCheckResult;

    /// Return the column list for a relation or table-valued function.
    /// `Some(cols)`: found with known columns.
    /// `Some([])`: found, columns unknown (suppress column errors).
    /// `None`    : not found in any layer.
    ///
    /// Used by the walker when registering a table reference in the query scope.
    pub(crate) fn columns_for_table_source(&self, name: &str) -> Option<Vec<String>>;

    // ── Enumeration: used for fuzzy "did you mean?" suggestions ────────────
    pub(crate) fn all_relation_names(&self) -> Vec<String>;
    pub(crate) fn all_column_names(&self, table: Option<&str>) -> Vec<String>;
    pub(crate) fn all_function_names(&self) -> Vec<String>;
    pub(crate) fn all_table_function_names(&self) -> Vec<String>;
}
```

### Resolution mechanics

**`resolve_relation(name)`**: iterate from innermost query frame outward, then document, database,
dialect. Return `true` on first hit. Case-insensitive (canonical key = `name.to_ascii_lowercase()`).

**`resolve_column(table, column)`**:
- If `table` is `Some(tbl)`: find `tbl` in scope. If found with `Some(cols)`, check `cols`
  contains `column`. Return `Found`, `TableFoundColumnMissing`, or `TableNotFound`. If found with
  `None` (unknown columns), return `Found` conservatively.
- If `table` is `None`: scan all scope frames + all catalog layers. If any table has `None`
  columns, return `Found` conservatively (unknown columns → can't rule the column out). If all
  known and column missing, return `NotFound`.

**`check_function(name, arg_count)`**: find first matching `FunctionSet` across all layers. If not
found, `Unknown`. If found, check each overload: `Exact(n) → n == arg_count`, `AtLeast(min) →
arg_count >= min`, `Any → true`. If any overload matches, `Ok`. Otherwise `WrongArity` with the
sorted list of exact fixed arities from that set.

### Building the dialect layer

The analyzer builds the dialect layer at construction:

```rust
fn build_dialect_layer(dialect: Dialect) -> CatalogLayer {
    let mut layer = CatalogLayer::default();

    #[cfg(feature = "sqlite")]
    for entry in crate::sqlite::functions_catalog::SQLITE_FUNCTIONS {
        if is_function_available(entry, &dialect) {
            layer.insert_function_arities(
                entry.info.name,
                map_category(entry.info.category),
                entry.info.arities,
            );
        }
    }

    for ext in dialect.function_extensions() {
        if is_function_available(ext, &dialect) {
            layer.insert_function_arities(
                ext.info.name,
                map_category(ext.info.category),
                ext.info.arities,
            );
        }
    }

    layer
}
```

This runs once. The result is stored in `Catalog::dialect` and never touched again.

### DDL accumulation into the document layer

`accumulate_ddl` extracts table/view/function definitions from a parsed DDL statement and inserts
them into `Catalog::document`. The extraction logic (currently in `catalog.rs` serving
`DocumentCatalog`) is rewired to produce a `CatalogLayer` insertion directly:

```rust
pub(crate) fn accumulate_ddl<A: AstTypes>(
    &mut self,
    stmt: AnyParsedStatement<'_>,
    root: AnyNodeId,
    dialect: Dialect,
) {
    let Some((tag, fields)) = stmt.extract_fields(root) else { return };
    let Some(contrib) = dialect.schema_contribution_for_tag(tag) else { return };

    let name = match fields[contrib.name_field as usize] {
        FieldValue::Span(s) if !s.is_empty() => s.to_string(),
        _ => return,
    };

    match contrib.kind {
        SchemaKind::Table | SchemaKind::View => {
            // Pass both the document layer (tables seen so far in this file)
            // and the database layer (user-provided schema) so that
            // CREATE TABLE AS SELECT can resolve its source table columns.
            // Returns None when source columns cannot be determined:
            // the RelationEntry is still inserted so the name is known,
            // but column errors against it are suppressed.
            let columns = extract_columns::<A>(
                &stmt, &fields, &contrib, dialect,
                &self.database, &self.document,
            );
            self.document.relations.insert(
                canonical(&name),
                RelationEntry { name, columns },
            );
        }
        SchemaKind::Function => {
            let arity = extract_function_arity(&stmt, &fields, &contrib);
            self.document.functions
                .entry(canonical(&name))
                .and_modify(|set| set.overloads.push(FunctionOverload {
                    category: FunctionCategory::Scalar,
                    arity,
                }))
                .or_insert_with(|| FunctionSet {
                    name,
                    overloads: vec![FunctionOverload {
                        category: FunctionCategory::Scalar,
                        arity,
                    }],
                });
        }
        SchemaKind::Import => {}
    }
}
```

`extract_columns` returns `Option<Vec<String>>` and handles both column-list DDL
(`CREATE TABLE t (a INT, b TEXT)`) and `CREATE TABLE AS SELECT` / `CREATE VIEW AS SELECT` paths,
using the existing helpers (`columns_from_column_list`, `columns_from_select`). It receives
`&database: &CatalogLayer` and `&document: &CatalogLayer` to resolve source table columns when
processing `AS SELECT` bodies. The column names in the returned `Vec` are lowercased. If the SELECT
references tables not present in either layer, `extract_columns` returns `None`: better to
suppress column errors than to report false positives.

---

## Part 2: Fuzzy Matching

Promoted from `legacy/fuzzy.rs` unchanged. Self-contained, well-tested, no external dependencies.

```rust
// semantic/fuzzy.rs

/// Levenshtein distance, case-insensitive, O(n) space.
pub(crate) fn levenshtein_distance(a: &str, b: &str) -> usize;

/// Closest candidate within `threshold` edits, or `None`.
pub(crate) fn best_suggestion(
    name: &str,
    candidates: &[String],
    threshold: usize,
) -> Option<String>;
```

Used by the walker when emitting `UnknownTable`, `UnknownColumn`, and `UnknownFunction`
diagnostics to populate `Help::Suggestion`.

---

## Part 3: Walker

The walker traverses the AST of a single parsed statement, resolves names against the catalog, and
collects diagnostics. It replaces `legacy/walker.rs` with the inline check logic from
`legacy/checks.rs` merged in.

### Entry point

```rust
// semantic/walker.rs

// Not Copy: holds a mutable reference. The current WalkContext is #[derive(Clone, Copy)]
// because it holds two immutable references; that derive must be removed here.
pub(crate) struct WalkContext<'a> {
    pub catalog: &'a mut Catalog,
    pub config:  &'a ValidationConfig,
}

pub(crate) struct Walker<'a, A: AstTypes<'a>> {
    stmt:        AnyParsedStatement<'a>,
    ctx:         WalkContext<'a>,
    diagnostics: Vec<Diagnostic>,
    _ast:        PhantomData<A>,
}

impl<'a, A: AstTypes<'a>> Walker<'a, A> {
    pub(crate) fn run(
        stmt: AnyParsedStatement<'a>,
        root: A::Stmt,
        ctx: WalkContext<'a>,
    ) -> Vec<Diagnostic> {
        let mut walker = Walker { stmt, ctx, diagnostics: Vec::new(), _ast: PhantomData };
        walker.walk_stmt(root);
        walker.diagnostics
    }
}
```

`WalkContext` holds `&'a mut Catalog` so the walker can push/pop query scopes in-place without
rebuilding anything on each lookup.

### Statement dispatch

```rust
fn walk_stmt(&mut self, stmt: A::Stmt) {
    match stmt.kind() {
        StmtKind::SelectStmt(s)       => self.walk_select_stmt(s),
        StmtKind::CompoundSelect(c)   => self.walk_compound_select(c),
        StmtKind::WithClause(w)       => self.walk_with_clause(w),
        StmtKind::InsertStmt(i)       => self.walk_insert(i),
        StmtKind::UpdateStmt(u)       => self.walk_update(u),
        StmtKind::DeleteStmt(d)       => self.walk_delete(d),
        StmtKind::CreateTableStmt(ct) => self.walk_opt_select(ct.as_select()),
        StmtKind::CreateViewStmt(cv)  => self.walk_opt_select(cv.select()),
        StmtKind::CreateTriggerStmt(t)=> self.walk_trigger(t),
        StmtKind::Other(node)         => self.walk_other_node(node),
        _                             => {}
    }
}
```

### Scope management: direct catalog push/pop

Subqueries and CTEs use `catalog.push_query_scope()` / `pop_query_scope()` in place of the old
`ScopeStack::push()` / `pop()`. A helper wraps the pattern:

```rust
fn with_scope<F>(&mut self, f: F)
where
    F: FnOnce(&mut Self),
{
    self.ctx.catalog.push_query_scope();
    f(self);
    self.ctx.catalog.pop_query_scope();
}
```

Subquery:
```rust
TableSourceKind::SubqueryTableSource(sub) => {
    self.with_scope(|this| this.walk_opt_select(sub.select()));
    let alias = sub.alias();
    if !alias.is_empty() {
        // columns=None: we don't statically know the subquery's output schema
        self.ctx.catalog.add_query_table(alias, None);
    }
}
```

CTE registration (non-recursive):
```rust
fn walk_with_clause(&mut self, with: A::WithClause) {
    let is_recursive = with.recursive();
    for cte in with.ctes().iter().flatten() {
        let name = cte.cte_name();
        if is_recursive && !name.is_empty() {
            self.ctx.catalog.add_query_table(name, None);
        }
        self.with_scope(|this| this.walk_opt_select(cte.select()));
        if !name.is_empty() {
            self.ctx.catalog.add_query_table(name, None);
        }
    }
    self.walk_opt_select(with.select());
}
```

### Table reference resolution (inlined from checks.rs)

When the walker encounters a table reference, it checks the catalog, emits a diagnostic if the
table is unknown, then registers the table (or its alias) in the query scope:

```rust
fn check_and_add_table_ref(&mut self, table_ref: &A::TableRef) {
    let name = table_ref.table_name();
    if name.is_empty() { return; }

    let offset = self.str_offset(name);

    let is_known = self.ctx.catalog.resolve_relation(name)
        || self.ctx.catalog.resolve_table_function(name);
    if !is_known {
        let mut candidates = self.ctx.catalog.all_relation_names();
        candidates.extend(self.ctx.catalog.all_table_function_names());
        let suggestion = best_suggestion(name, &candidates, self.ctx.config.suggestion_threshold);
        self.diagnostics.push(Diagnostic {
            start_offset: offset,
            end_offset:   offset + name.len(),
            message:      DiagnosticMessage::UnknownTable { name: name.to_string() },
            severity:     self.ctx.config.severity(),
            help:         suggestion.map(Help::Suggestion),
        });
    }

    let alias      = table_ref.alias();
    let scope_name = if alias.is_empty() { name } else { alias };
    // columns_for_table_source checks both relations and table-valued functions.
    // Returns None if the name is not found (already reported above).
    let columns = self.ctx.catalog.columns_for_table_source(name);
    self.ctx.catalog.add_query_table(scope_name, columns);
}
```

`columns_for_table_source(name)` is an internal method that checks relations and table-valued
functions across all layers. It returns `Some(cols)` when the entry is found with a known column
list, `Some([])` when the entry is found but columns are not tracked (treated as unknown:
column errors against it are suppressed), and `None` if not found at all.

### Column reference resolution (inlined from checks.rs)

```rust
fn check_column_ref(&mut self, col: A::ColumnRef) {
    let column = col.column();
    if column.is_empty() { return; }

    let table  = col.table();
    let table  = if table.is_empty() { None } else { Some(table) };
    let offset = self.str_offset(column);

    match self.ctx.catalog.resolve_column(table, column) {
        ColumnResolution::Found | ColumnResolution::TableNotFound => {}
        ColumnResolution::TableFoundColumnMissing => {
            let tbl = table.unwrap();
            let candidates = self.ctx.catalog.all_column_names(Some(tbl));
            let suggestion = best_suggestion(column, &candidates, self.ctx.config.suggestion_threshold);
            self.diagnostics.push(Diagnostic {
                start_offset: offset,
                end_offset:   offset + column.len(),
                message:      DiagnosticMessage::UnknownColumn {
                    column: column.to_string(),
                    table:  Some(tbl.to_string()),
                },
                severity:     self.ctx.config.severity(),
                help:         suggestion.map(Help::Suggestion),
            });
        }
        ColumnResolution::NotFound => {
            let candidates = self.ctx.catalog.all_column_names(None);
            let suggestion = best_suggestion(column, &candidates, self.ctx.config.suggestion_threshold);
            self.diagnostics.push(Diagnostic {
                start_offset: offset,
                end_offset:   offset + column.len(),
                message:      DiagnosticMessage::UnknownColumn {
                    column: column.to_string(),
                    table:  None,
                },
                severity:     self.ctx.config.severity(),
                help:         suggestion.map(Help::Suggestion),
            });
        }
    }
}
```

`TableNotFound` is silently ignored: the missing table was already reported by `check_and_add_table_ref`. Reporting the column error too would double-warn the user about a single root cause.

### Function call resolution

```rust
fn walk_function(
    &mut self,
    name: &str,
    args: Option<TypedNodeList<'a, A::Grammar, A::Expr>>,
    filter: Option<A::Expr>,
) {
    if !name.is_empty() {
        let offset    = self.str_offset(name);
        let arg_count = args.as_ref().map_or(0, TypedNodeList::len);
        match self.ctx.catalog.check_function(name, arg_count) {
            FunctionCheckResult::Ok => {}
            FunctionCheckResult::Unknown => {
                let candidates = self.ctx.catalog.all_function_names();
                let suggestion = best_suggestion(name, &candidates, self.ctx.config.suggestion_threshold);
                self.diagnostics.push(Diagnostic {
                    start_offset: offset,
                    end_offset:   offset + name.len(),
                    message:      DiagnosticMessage::UnknownFunction { name: name.to_string() },
                    severity:     self.ctx.config.severity(),
                    help:         suggestion.map(Help::Suggestion),
                });
            }
            FunctionCheckResult::WrongArity { expected } => {
                self.diagnostics.push(Diagnostic {
                    start_offset: offset,
                    end_offset:   offset + name.len(),
                    message:      DiagnosticMessage::FunctionArity {
                        name: name.to_string(),
                        expected,
                        got: arg_count,
                    },
                    severity:     self.ctx.config.severity(),
                    help:         None,
                });
            }
        }
    }
    if let Some(args) = args { self.walk_expr_list(args); }
    self.walk_opt_expr(filter);
}
```

### Trigger special scope

Trigger bodies need `OLD` and `NEW` pseudo-tables in scope with unknown columns:

```rust
fn walk_trigger(&mut self, trigger: A::CreateTriggerStmt) {
    self.with_scope(|this| {
        this.ctx.catalog.add_query_table("OLD", None);
        this.ctx.catalog.add_query_table("NEW", None);
        this.walk_opt_expr(trigger.when_expr());
        for stmt in trigger.body().iter().flatten() {
            this.walk_stmt(stmt);
        }
    });
}
```

---

## Part 4: SemanticModel and Single-Pass Analysis

### Single-pass loop

One parse session with `with_collect_tokens(true)` gives both `stmt.tokens()` (for syntax
highlighting) and `stmt.root()` (for semantic walking) on the same statement object. No re-parse:

```rust
let parser  = syntaqlite_syntax::Parser::with_config(
    &ParserConfig::default().with_collect_tokens(true),
);
let mut session = parser.parse(source);

let mut tokens      = Vec::new();
let mut comments    = Vec::new();
let mut diagnostics = Vec::new();

while let Some(stmt) = session.next() {
    match stmt {
        Ok(stmt) => {
            // collect token positions for syntax highlighting
            for tok in stmt.tokens() {
                tokens.push(StoredToken {
                    offset:     str_offset(source, tok.text()),
                    length:     tok.text().len(),
                    token_type: tok.token_type(),
                    flags:      tok.flags(),
                });
            }
            for c in stmt.comments() {
                comments.push(StoredComment {
                    offset: str_offset(source, c.text),
                    length: c.text.len(),
                });
            }
            // semantic walk
            if let Some(root) = stmt.root() {
                let root_id: AnyNodeId = root.node_id().into();
                catalog.accumulate_ddl::<A>(stmt.erase(), root_id, dialect);
                let diags = Walker::<A>::run(
                    stmt.erase(),
                    A::Stmt::from_result(stmt.erase(), root_id).unwrap(),
                    WalkContext { catalog, config },
                );
                diagnostics.extend(diags);
            }
        }
        Err(err) => {
            let (start, end) = parse_error_span(&err, source);
            diagnostics.push(Diagnostic {
                start_offset: start,
                end_offset:   end,
                message:      DiagnosticMessage::Other(err.message().to_string()),
                severity:     Severity::Error,
                help:         None,
            });
        }
    }
}
```

### SemanticModel

`SemanticModel` stores the complete result of one analysis pass. There is no longer a
`prepare()` / `diagnostics_prepared()` split: the model is built by the analyzer's main method
and contains everything the caller might query.

```rust
// semantic/model.rs

pub(crate) struct SemanticModel {
    source:      String,
    tokens:      Vec<StoredToken>,
    comments:    Vec<StoredComment>,
    diagnostics: Vec<Diagnostic>,
}

impl SemanticModel {
    pub(crate) fn source(&self) -> &str { &self.source }
    pub(crate) fn diagnostics(&self) -> &[Diagnostic] { &self.diagnostics }

    /// Semantic tokens for syntax highlighting. Consumes token + comment lists,
    /// maps each to a TokenCategory via the dialect, sorts by offset.
    pub(crate) fn semantic_tokens(&self, dialect: Dialect) -> Vec<SemanticToken> {
        let mut out = Vec::new();
        for t in &self.tokens {
            let cat = dialect.classify_token(t.token_type.into(), t.flags);
            if cat != TokenCategory::Other {
                out.push(SemanticToken { offset: t.offset, length: t.length, category: cat });
            }
        }
        for c in &self.comments {
            out.push(SemanticToken { offset: c.offset, length: c.length, category: TokenCategory::Comment });
        }
        out.sort_by_key(|t| t.offset);
        out
    }

    /// Completion info at `offset`. Uses the typed incremental parser to
    /// replay tokens up to the cursor and return expected token types + context.
    pub(crate) fn completion_info(&self, offset: usize) -> CompletionInfo { ... }
}
```

Support types:

```rust
pub(crate) struct StoredToken {
    pub offset:     usize,
    pub length:     usize,
    pub token_type: TokenType,
    pub flags:      ParserTokenFlags,
}

pub(crate) struct StoredComment {
    pub offset: usize,
    pub length: usize,
}

pub(crate) struct SemanticToken {
    pub offset:   usize,
    pub length:   usize,
    pub category: TokenCategory,
}

pub(crate) enum CompletionContext { Unknown, Expression, TableRef }

pub(crate) struct CompletionInfo {
    pub tokens:  Vec<TokenType>,
    pub context: CompletionContext,
}
```

---

## Part 5: SemanticAnalyzer

The analyzer is long-lived, reused across inputs. It holds the catalog with its pre-built dialect
layer and manages the document layer between statements.

```rust
// semantic/analyzer.rs

pub(crate) struct SemanticAnalyzer {
    dialect: Dialect,
    catalog: Catalog,  // dialect layer built at construction; database + document layers mutable
}

impl SemanticAnalyzer {
    #[cfg(feature = "sqlite")]
    pub(crate) fn new() -> Self {
        Self::with_dialect(crate::dialect::sqlite())
    }

    pub(crate) fn with_dialect(dialect: impl Into<Dialect>) -> Self {
        let dialect = dialect.into();
        let mut catalog = Catalog::new();
        catalog.set_dialect_layer(build_dialect_layer(dialect));
        SemanticAnalyzer { dialect, catalog }
    }

    pub(crate) fn dialect(&self) -> Dialect { self.dialect }

    /// Run a complete analysis pass: parse, collect tokens, walk AST, return model.
    /// The caller's `user_catalog` database layer is applied for this pass only.
    pub(crate) fn analyze(
        &mut self,
        source: &str,
        user_catalog: &Catalog,
        config: &ValidationConfig,
    ) -> SemanticModel {
        self.catalog.clear_document();
        // Merge user's database layer into our catalog for this pass.
        self.catalog.set_database_layer(user_catalog.database.clone());

        self.analyze_inner::<SqliteAstMarker>(source, config)
    }

    #[cfg(feature = "sqlite")]
    fn analyze_inner<A: for<'a> AstTypes<'a>>(
        &mut self,
        source: &str,
        config: &ValidationConfig,
    ) -> SemanticModel {
        // single-pass loop as described in Part 4
        ...
        SemanticModel { source: source.to_string(), tokens, comments, diagnostics }
    }
}
```

`clear_document()` is called at the start of each `analyze()` call, resetting accumulated DDL.
The document layer accumulates as statements are processed in order within the single-pass loop:
a `CREATE TABLE` statement seen earlier in the file makes the table visible to queries later in
the same file.

The query layer is managed entirely by the walker. At the end of each statement walk, the query
stack is guaranteed to be empty (walker pops whatever it pushes). No cleanup needed between
statements.

---

## Part 6: DiagnosticRenderer

```rust
// semantic/render.rs

pub(crate) struct DiagnosticRenderer<'a> {
    source: &'a str,
    file:   &'a str,
}

impl<'a> DiagnosticRenderer<'a> {
    pub(crate) fn new(source: &'a str, file: &'a str) -> Self;

    /// Render a single diagnostic in rustc style to `out`.
    ///
    /// ```text
    /// error: unknown table 'usr'
    ///  --> query.sql:1:15
    ///   |
    /// 1 | SELECT id FROM usr WHERE id = 1
    ///   |               ^~~
    ///   = help: did you mean 'users'?
    /// ```
    pub(crate) fn render_diagnostic(
        &self,
        diag: &Diagnostic,
        out: &mut impl Write,
    ) -> io::Result<()>;

    /// Render all diagnostics to `out`. Returns `true` if any had `Severity::Error`.
    pub(crate) fn render_diagnostics(
        &self,
        diags: &[Diagnostic],
        out: &mut impl Write,
    ) -> io::Result<bool>;
}

pub(crate) type SourceContext<'a> = DiagnosticRenderer<'a>;
```

Callers that want stderr:
```rust
renderer.render_diagnostics(&diags, &mut std::io::stderr())?;
```

Callers that want a string (e.g. tests):
```rust
let mut buf = Vec::new();
renderer.render_diagnostics(&diags, &mut buf)?;
let s = String::from_utf8(buf).unwrap();
```

---

## Module Structure

All files under `semantic/` are gated on `#[cfg(feature = "semantic")]`. `mod.rs` declares
each sub-module under that gate; the individual files do not repeat it.

```
semantic/
    mod.rs         : ValidationConfig, public re-exports (Diagnostic, Severity, Help,
                      DiagnosticMessage, Catalog); all sub-modules declared under
                      #[cfg(feature = "semantic")]
    diagnostics.rs : Diagnostic, DiagnosticMessage, Severity, Help; JSON serialization
                      (keep as-is)
    catalog.rs     : CatalogLayer (pub(crate)), Catalog with named layers, all resolution
                      logic, DDL extraction helpers, ColumnResolution, FunctionCheckResult,
                      TableFunctionSet
    fuzzy.rs       : levenshtein_distance, best_suggestion (promoted from legacy, unchanged)
    walker.rs      : Walker<A>, WalkContext (not Copy), full AST traversal, inlined check logic
    analyzer.rs    : SemanticAnalyzer, single-pass analysis loop
    model.rs       : SemanticModel, StoredToken, StoredComment, SemanticToken,
                      CompletionInfo, CompletionContext
    render.rs      : DiagnosticRenderer writing to impl Write

legacy/            : entire directory deleted after promotion is complete
```

### What is deleted

| Item | Reason |
|------|--------|
| `semantic/schema.rs` | `RelationDef`, `ColumnDef`, `FunctionDef` only served the deleted catalog wrappers |
| `DatabaseCatalog` (in `catalog.rs`) | Replaced by `Catalog::add_table` / `add_view` / `add_function` |
| `DocumentCatalog` (in `catalog.rs`) | Replaced by `Catalog::document` layer + `accumulate_ddl` |
| `StaticCatalog` (in `catalog.rs`) | Replaced by `Catalog::dialect` layer built at construction |
| `CatalogStack` (in `catalog.rs`) | Replaced by `Catalog` itself: no per-lookup rebuild |
| `legacy/scope.rs` | `ScopeStack` replaced by `Catalog` query layer push/pop |
| `legacy/checks.rs` | `check_table_ref`, `check_column_ref`, `make_diagnostic` inlined into `walker.rs` |
| `legacy/` directory | Deleted after all files are promoted |

---

## Implementation Steps

Each step should leave the codebase in a compilable state.

### Step 1: Restructure `catalog.rs`

- Add `Option<Vec<String>>` to `RelationEntry`
- Add `TableFunctionSet` and restore `table_functions` field to `CatalogLayer`
- Replace `DatabaseCatalog` / `DocumentCatalog` / `StaticCatalog` / `CatalogStack` with named
  slots on `Catalog`: `dialect`, `database`, `document`, `query: Vec<CatalogLayer>`
- Implement `push_query_scope` / `pop_query_scope` / `add_query_table`
- Move `ColumnResolution` and `FunctionCheckResult` into `catalog.rs`
- Implement `resolve_relation`, `resolve_table_function`, `resolve_column`, `check_function`,
  `columns_for_table_source`, `all_*` enumeration methods on `Catalog` directly (no
  CatalogStack indirection)
- Implement public `add_table` / `add_view` / `add_function` / `add_table_function` /
  `clear_database` / `from_json` / `from_ddl` on `Catalog`
- Rewire DDL extraction helpers to write into `Catalog::document` directly; update
  `extract_columns` to accept `&database: &CatalogLayer` and `&document: &CatalogLayer` and
  return `Option<Vec<String>>`
- Change feature gate on `catalog.rs` from the old `fmt`/`embedded`/`lsp` combination to
  `#[cfg(feature = "semantic")]`
- Keep `CatalogLayer` `pub(crate)`

### Step 2: Delete `schema.rs`

Update all imports. Inline any necessary type definitions into `catalog.rs` (none should be needed
once the catalog types are rewritten).

### Step 3: Promote `fuzzy.rs`

Copy `legacy/fuzzy.rs` to `semantic/fuzzy.rs`. Declare `pub(crate) mod fuzzy` in `mod.rs`. No
content changes.

### Step 4: Promote and rewrite `walker.rs`

- Copy `legacy/walker.rs` to `semantic/walker.rs`
- Update `WalkContext` to hold `&'a mut Catalog` instead of `&'a CatalogStack<'a>`; remove
  `#[derive(Clone, Copy)]` (mutable reference makes it non-Copy)
- Replace all `scope.push()` / `scope.pop()` / `scope.add_table()` calls with
  `catalog.push_query_scope()` / `pop_query_scope()` / `add_query_table()`
- Update `check_and_add_table_ref` to check both `resolve_relation` and `resolve_table_function`,
  include table function names in suggestion candidates, and use `columns_for_table_source`
- Inline `check_table_ref`, `check_column_ref`, `make_diagnostic` from `checks.rs`
- Fix `Walker::run` signature: 3 arguments (`stmt`, `root`, `ctx`): `ctx` carries both catalog
  and config

### Step 5: Promote `model.rs`

- Copy `legacy/model.rs` to `semantic/model.rs`
- Remove the prepare/diagnostics split: `SemanticModel` stores both tokens and diagnostics
- Add `diagnostics()` accessor
- Keep `semantic_tokens()` and `completion_info()` as methods on `SemanticModel`

### Step 6: Promote and rewrite `analyzer.rs`

- Copy `legacy/analyzer.rs` to `semantic/analyzer.rs`
- Replace `StaticCatalog` / `DocumentCatalog` fields with `Catalog`
- Implement single-pass loop (tokens + DDL accumulation + AST walk in one session iteration)
- Expose `analyze(&str, &Catalog, &ValidationConfig) -> SemanticModel` as the primary method
- Remove the `prepare()` / `diagnostics_prepared()` split

### Step 7: Promote `render.rs`

- Copy `legacy/render.rs` to `semantic/render.rs`
- Replace every `eprintln!(...)` call with `write!(out, ...)` / `writeln!(out, ...)`
- Add `out: &mut impl Write` parameter to `render_diagnostic` and `render_diagnostics`
- Add `use std::io::{self, Write}` import

### Step 8: Update `mod.rs` and delete `legacy/`

- Declare `pub(crate) mod` for each new file under `#[cfg(feature = "semantic")]`
- Update public re-exports: add `Catalog`, keep `Diagnostic` / `Severity` / `Help` /
  `DiagnosticMessage` / `ValidationConfig`
- Remove old `#[cfg(all(feature = "fmt", any(feature = "embedded", feature = "lsp")))]` gates
- Delete `semantic/legacy/` directory

---

## Open Questions

- **`analyze()` ownership of `user_catalog`.** Currently the plan clones the user's database layer
  into the internal catalog at the start of each pass (`set_database_layer`). An alternative is to
  pass `&Catalog` and merge lazily at resolution time, avoiding the clone. The simple clone-on-call
  approach is easier to reason about for now.

- **`SemanticModel` and `completion_info`.** The completion logic uses a `TypedParser` incremental
  parse to replay tokens up to the cursor. This is a typed-AST concern and needs `SqliteAstMarker`
  access. Whether this belongs on `SemanticModel` as a method or back in `SemanticAnalyzer` as a
  method that takes a `SemanticModel` is an open call.

- **Feature gate cleanup.** `catalog.rs` and `schema.rs` are currently gated on
  `#[cfg(all(feature = "fmt", any(feature = "embedded", feature = "lsp")))]`. After the refactor,
  the entire `semantic/` module (catalog, walker, analyzer, model, render) is gated on
  `#[cfg(feature = "semantic")]`. All per-file `cfg` annotations are updated in Step 1 (catalog)
  and Step 8 (mod.rs). No open question: `semantic` is the gate.
