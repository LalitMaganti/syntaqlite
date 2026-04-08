// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Layered semantic catalog.
//!
//! Resolution order: query (innermost frame first) → document → connection → database → dialect.

use std::collections::{HashMap, HashSet};

use syntaqlite_syntax::any::{AnyNodeId, AnyParsedStatement, FieldValue, NodeFields};

use crate::dialect::AnyDialect;
use crate::dialect::{
    FIELD_ABSENT, FunctionCategory as DialectFunctionCategory, SemanticRole, is_function_available,
};

/// Convert a `u8` field index with [`FIELD_ABSENT`] sentinel to `Option<u8>`.
#[inline]
fn opt_field(v: u8) -> Option<u8> {
    (v != FIELD_ABSENT).then_some(v)
}

// ── Core layer types ─────────────────────────────────────────────────────────

/// The category of a catalog function.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum FunctionCategory {
    /// A scalar function (e.g. `length`, `upper`).
    Scalar,
    /// An aggregate function (e.g. `count`, `sum`).
    Aggregate,
    /// A window function (e.g. `row_number`, `rank`).
    Window,
}

/// Describes how many arguments a function overload accepts.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum AritySpec {
    /// Accepts exactly this many arguments.
    Exact(usize),
    /// Accepts at least this many arguments (variadic).
    AtLeast(usize),
    /// Accepts any number of arguments.
    Any,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub(crate) struct FunctionOverload {
    pub category: FunctionCategory,
    pub arity: AritySpec,
}

#[derive(Debug, Clone)]
struct FunctionSet {
    name: String,
    overloads: Vec<FunctionOverload>,
}

/// Where a catalog entry was originally defined (e.g. in an external schema file).
#[derive(Debug, Clone)]
pub(crate) struct DefinitionSite {
    /// File URI (e.g. `"file:///path/to/schema.sql"`).
    pub file_uri: String,
    /// Byte offset of the start of the name in the source file.
    pub start: usize,
    /// Byte offset of the end of the name in the source file.
    pub end: usize,
}

#[derive(Debug, Clone)]
struct RelationEntry {
    name: String,
    /// `None` = table is known to exist but column list is not tracked.
    /// Column references against it are conservatively accepted.
    columns: Option<Vec<String>>,
    /// `true` for WITHOUT ROWID tables — no implicit rowid/oid/_rowid_ columns.
    without_rowid: bool,
    /// `true` if this relation is a view (not a physical table).
    is_view: bool,
    /// Where this relation was defined, if known.
    definition_site: Option<DefinitionSite>,
    /// Where each column was defined, keyed by lowercase column name.
    column_definition_sites: HashMap<String, DefinitionSite>,
}

#[derive(Debug, Clone)]
struct TableFunctionSet {
    name: String,
    overloads: Vec<FunctionOverload>,
    /// Empty = output columns unknown; suppress column errors.
    output_columns: Vec<String>,
}

// ── Resolution result types ───────────────────────────────────────────────────

#[derive(Debug, PartialEq, Eq)]
pub(crate) enum ColumnResolution {
    /// Column found (or table has unknown columns — conservatively accepted).
    Found {
        table: String,
        all_columns: Vec<String>,
    },
    /// Table is in scope but this column is not in its known list.
    TableFoundColumnMissing,
    /// The qualifier table is not in scope — table check already reported this.
    TableNotFound,
    /// Unqualified column not found in any table in scope.
    NotFound,
}

#[derive(Debug, PartialEq, Eq)]
pub(crate) enum FunctionCheckResult {
    Ok,
    Unknown,
    WrongArity { expected: Vec<usize> },
}

// ── CatalogLayerContents ──────────────────────────────────────────────────────

/// The data stored in a single catalog layer.
///
/// Callers obtain a mutable reference via [`Catalog::layer_mut`] and call
/// `insert_*` methods to populate it.
#[derive(Debug, Default, Clone)]
pub struct CatalogLayerContents {
    relations: HashMap<String, RelationEntry>,
    functions: HashMap<String, FunctionSet>,
    table_functions: HashMap<String, TableFunctionSet>,
}

impl CatalogLayerContents {
    /// Remove all entries from this layer.
    fn clear(&mut self) {
        self.relations = HashMap::default();
        self.functions = HashMap::default();
        self.table_functions = HashMap::default();
    }

    /// Merge all entries from `other` into this layer (existing keys are
    /// overwritten).
    pub(crate) fn merge_from(&mut self, other: &Self) {
        self.relations
            .extend(other.relations.iter().map(|(k, v)| (k.clone(), v.clone())));
        self.functions
            .extend(other.functions.iter().map(|(k, v)| (k.clone(), v.clone())));
        self.table_functions.extend(
            other
                .table_functions
                .iter()
                .map(|(k, v)| (k.clone(), v.clone())),
        );
    }

    /// Insert a table into this layer.
    ///
    /// Pass `columns = Some(vec![...])` when the column list is known so the
    /// analyzer can validate column references. Pass `columns = None` when the
    /// table exists but its columns are unknown — references against it are
    /// conservatively accepted without warnings.
    ///
    /// Set `without_rowid` to `true` for `WITHOUT ROWID` tables (suppresses
    /// the implicit `rowid` column during resolution).
    ///
    /// # Example
    ///
    /// ```
    /// # use syntaqlite::semantic::CatalogLayer;
    /// # use syntaqlite::Catalog;
    /// let mut catalog = Catalog::new(syntaqlite::sqlite_dialect());
    /// let db = catalog.layer_mut(CatalogLayer::Database);
    ///
    /// // Known columns — misspelled column names will produce diagnostics.
    /// db.insert_table("users", Some(vec!["id".into(), "name".into()]), false);
    ///
    /// // Unknown columns — any column reference is accepted.
    /// db.insert_table("external_data", None, false);
    /// ```
    pub fn insert_table(
        &mut self,
        name: impl Into<String>,
        columns: Option<Vec<String>>,
        without_rowid: bool,
    ) {
        let name = name.into();
        self.relations.insert(
            canonical_name(&name),
            RelationEntry {
                name,
                columns,
                without_rowid,
                is_view: false,
                definition_site: None,
                column_definition_sites: HashMap::new(),
            },
        );
    }

    /// Insert a view into this layer.
    ///
    /// Views behave like tables for resolution purposes but never expose an
    /// implicit `rowid` column. As with [`insert_table`](Self::insert_table),
    /// pass `None` for `columns` when the column list is unknown.
    ///
    /// # Example
    ///
    /// ```
    /// # use syntaqlite::semantic::CatalogLayer;
    /// # use syntaqlite::Catalog;
    /// let mut catalog = Catalog::new(syntaqlite::sqlite_dialect());
    /// catalog
    ///     .layer_mut(CatalogLayer::Database)
    ///     .insert_view("active_users", Some(vec!["id".into(), "name".into()]));
    /// ```
    pub fn insert_view(&mut self, name: impl Into<String>, columns: Option<Vec<String>>) {
        let name = name.into();
        self.relations.insert(
            canonical_name(&name),
            RelationEntry {
                name,
                columns,
                without_rowid: true, // views have no rowid
                is_view: true,
                definition_site: None,
                column_definition_sites: HashMap::new(),
            },
        );
    }

    /// Insert a single function overload into this layer.
    ///
    /// Use this to register application-defined functions so the analyzer can
    /// validate calls and arity. Call multiple times with the same name to
    /// register multiple overloads (e.g. one accepting 1 argument and another
    /// accepting 2).
    ///
    /// # Example
    ///
    /// ```
    /// # use syntaqlite::semantic::CatalogLayer;
    /// # use syntaqlite::{Catalog, SemanticAnalyzer, ValidationConfig};
    /// # use syntaqlite::semantic::{FunctionCategory, AritySpec};
    /// let mut catalog = Catalog::new(syntaqlite::sqlite_dialect());
    /// let db = catalog.layer_mut(CatalogLayer::Database);
    ///
    /// // Register a custom scalar function that takes exactly 2 arguments.
    /// db.insert_function_overload("my_concat", FunctionCategory::Scalar, AritySpec::Exact(2));
    ///
    /// // The analyzer now accepts calls to my_concat().
    /// let mut analyzer = SemanticAnalyzer::new();
    /// let config = ValidationConfig::default();
    /// let model = analyzer.analyze(
    ///     "SELECT my_concat('hello', 'world');",
    ///     &catalog,
    ///     &config,
    /// );
    /// assert!(model.diagnostics().is_empty());
    /// ```
    pub fn insert_function_overload(
        &mut self,
        name: impl Into<String>,
        category: FunctionCategory,
        arity: AritySpec,
    ) {
        let name = name.into();
        let key = canonical_name(&name);
        self.functions
            .entry(key)
            .and_modify(|set| set.overloads.push(FunctionOverload { category, arity }))
            .or_insert_with(|| FunctionSet {
                name,
                overloads: vec![FunctionOverload { category, arity }],
            });
    }

    /// Insert multiple arities for a function (dialect codegen helper).
    pub(crate) fn insert_function_arities(
        &mut self,
        name: impl Into<String>,
        category: FunctionCategory,
        arities: &[i16],
    ) {
        let name = name.into();
        if arities.is_empty() {
            self.insert_function_overload(name, category, AritySpec::Any);
            return;
        }
        for &a in arities {
            let arity = match a.cmp(&-1) {
                std::cmp::Ordering::Equal => AritySpec::Any,
                std::cmp::Ordering::Less => AritySpec::AtLeast(
                    usize::try_from(-i32::from(a) - 1).expect("negative arity encodes minimum"),
                ),
                std::cmp::Ordering::Greater => AritySpec::Exact(
                    usize::try_from(i32::from(a)).expect("fixed arity must be non-negative"),
                ),
            };
            self.insert_function_overload(name.clone(), category, arity);
        }
    }

    /// Insert a table-valued function.
    pub fn insert_table_function(
        &mut self,
        name: impl Into<String>,
        arity: AritySpec,
        output_columns: Vec<String>,
    ) {
        let name = name.into();
        let key = canonical_name(&name);
        self.table_functions
            .entry(key)
            .and_modify(|set| {
                set.overloads.push(FunctionOverload {
                    category: FunctionCategory::Scalar,
                    arity,
                });
            })
            .or_insert_with(|| TableFunctionSet {
                name,
                overloads: vec![FunctionOverload {
                    category: FunctionCategory::Scalar,
                    arity,
                }],
                output_columns,
            });
    }

    fn relation(&self, name: &str) -> Option<&RelationEntry> {
        self.relations.get(&canonical_name(name))
    }

    fn function(&self, name: &str) -> Option<&FunctionSet> {
        self.functions.get(&canonical_name(name))
    }

    fn table_function(&self, name: &str) -> Option<&TableFunctionSet> {
        self.table_functions.get(&canonical_name(name))
    }
}

// ── CatalogLayer enum ─────────────────────────────────────────────────────────

/// Identifies a fixed layer in the [`Catalog`].
///
/// Use [`Catalog::layer`] / [`Catalog::layer_mut`] to access the corresponding
/// [`CatalogLayerContents`] directly.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CatalogLayer {
    /// Dialect built-ins — populated at construction, never mutated.
    Dialect,
    /// Persistent user schema (cross-connection): tables, views, functions.
    Database,
    /// Connection-scoped schema (attached databases, session overrides).
    Connection,
    /// DDL accumulated from the current source document — cleared each pass.
    Document,
}

impl CatalogLayer {
    fn index(self) -> usize {
        match self {
            Self::Dialect => LAYER_DIALECT,
            Self::Database => LAYER_DATABASE,
            Self::Connection => LAYER_CONNECTION,
            Self::Document => LAYER_DOCUMENT,
        }
    }
}

// ── Layer index constants ─────────────────────────────────────────────────────

const LAYER_DIALECT: usize = 0;
const LAYER_DATABASE: usize = 1;
const LAYER_CONNECTION: usize = 2;
const LAYER_DOCUMENT: usize = 3;
/// Number of fixed layers that are always present.
const FIXED_LAYER_COUNT: usize = 4;

// ── Public Catalog ────────────────────────────────────────────────────────────

/// Layered semantic catalog describing a database schema.
///
/// Use this to tell [`SemanticAnalyzer`](super::analyzer::SemanticAnalyzer)
/// which tables, views, and functions exist so it can validate column
/// references, function calls, and arity.
///
/// Layers are stored in a single `Vec` indexed by priority (lowest first):
///
/// ```text
/// index 0  CatalogLayer::Dialect    — dialect built-ins (never mutated)
/// index 1  CatalogLayer::Database   — persistent user schema
/// index 2  CatalogLayer::Connection — connection-scoped schema
/// index 3  CatalogLayer::Document   — DDL from the current source
/// index 4+ query scopes             — pushed/popped during AST traversal
/// ```
///
/// Resolution iterates layers from highest index to lowest, so the priority
/// order is: innermost query scope > document > connection > database > dialect.
///
/// # Populating layers
///
/// Obtain a mutable reference to any fixed layer via [`layer_mut`](Self::layer_mut)
/// and call `insert_*` methods on the returned [`CatalogLayerContents`]:
///
/// ```
/// # use syntaqlite::semantic::CatalogLayer;
/// # use syntaqlite::Catalog;
/// let mut catalog = Catalog::new(syntaqlite::sqlite_dialect());
///
/// // Register a table with known columns.
/// catalog
///     .layer_mut(CatalogLayer::Database)
///     .insert_table("users", Some(vec!["id".into(), "name".into()]), false);
///
/// // Register a table whose columns are unknown — column references
/// // against it are conservatively accepted without warnings.
/// catalog
///     .layer_mut(CatalogLayer::Database)
///     .insert_table("logs", None, false);
/// ```
pub struct Catalog {
    layers: Vec<CatalogLayerContents>,
}

impl Catalog {
    /// Create a catalog for `dialect`.
    ///
    /// The dialect's built-in functions (e.g. `length`, `count`, `substr` for
    /// `SQLite`) are loaded immediately into the dialect layer. After
    /// construction, use [`layer_mut`](Self::layer_mut) to populate the
    /// database layer with your application's tables and views.
    ///
    /// # Example
    ///
    /// ```
    /// # use syntaqlite::semantic::CatalogLayer;
    /// # use syntaqlite::Catalog;
    /// let mut catalog = Catalog::new(syntaqlite::sqlite_dialect());
    ///
    /// catalog
    ///     .layer_mut(CatalogLayer::Database)
    ///     .insert_table("orders", Some(vec!["id".into(), "total".into()]), false);
    /// ```
    pub fn new(dialect: impl Into<AnyDialect>) -> Self {
        let dialect = dialect.into();
        let mut layers = vec![CatalogLayerContents::default(); FIXED_LAYER_COUNT];
        build_dialect_layer(&mut layers[LAYER_DIALECT], &dialect);
        Self { layers }
    }

    // ── Direct layer access ───────────────────────────────────────────────────

    /// Borrow a fixed layer immutably.
    pub fn layer(&self, which: CatalogLayer) -> &CatalogLayerContents {
        &self.layers[which.index()]
    }

    /// Borrow a fixed layer mutably.
    ///
    /// Use the returned `CatalogLayerContents` to insert relations, functions,
    /// or table-valued functions into the chosen layer.
    pub fn layer_mut(&mut self, which: CatalogLayer) -> &mut CatalogLayerContents {
        &mut self.layers[which.index()]
    }

    // ── Lifecycle convenience methods ─────────────────────────────────────────

    /// Switch to a new database.
    ///
    /// Clears the Database, Connection, and Document layers and discards all
    /// query scopes. Use this when the connected database changes entirely.
    pub fn new_database(&mut self) {
        self.layers.truncate(FIXED_LAYER_COUNT);
        for i in LAYER_DATABASE..FIXED_LAYER_COUNT {
            self.layers[i].clear();
        }
    }

    /// Switch to a new connection on the same database.
    ///
    /// Resets the Connection and Document layers and discards all query scopes.
    pub fn new_connection(&mut self) {
        self.layers.truncate(FIXED_LAYER_COUNT);
        for i in LAYER_CONNECTION..FIXED_LAYER_COUNT {
            self.layers[i].clear();
        }
    }

    /// Start a new document analysis pass.
    ///
    /// Resets the Document layer and discards all query scopes.
    /// Call this at the start of each analysis pass before accumulating DDL.
    pub fn new_document(&mut self) {
        self.layers.truncate(FIXED_LAYER_COUNT);
        self.layers[LAYER_DOCUMENT].clear();
    }

    // ── Convenience constructors ──────────────────────────────────────────────

    /// Parse DDL statements from one or more sources and populate the database
    /// layer.
    ///
    /// Each entry in `sources` is a `(sql_text, optional_file_uri)` pair.
    /// All sources are accumulated into a single catalog so that tables
    /// defined in earlier sources are visible to later ones.
    ///
    /// Returns `(catalog, errors)`. `errors` contains the human-readable
    /// message for each statement that failed to parse. Partial results from
    /// successfully parsed statements are always accumulated.
    #[cfg(feature = "sqlite")]
    pub fn from_ddl(
        dialect: impl Into<AnyDialect>,
        sources: &[(&str, Option<&str>)],
    ) -> (Self, Vec<String>) {
        use syntaqlite_syntax::ParseOutcome;
        let dialect = dialect.into();
        let mut catalog = Catalog::new(dialect.clone());
        let mut errors: Vec<String> = Vec::new();
        let parser = syntaqlite_syntax::Parser::new();
        for &(source, file_uri) in sources {
            let mut session = parser.parse(source);
            loop {
                let stmt = match session.next() {
                    ParseOutcome::Ok(stmt) => stmt,
                    ParseOutcome::Done => break,
                    ParseOutcome::Err(e) => {
                        errors.push(e.message().to_string());
                        continue;
                    }
                };
                let Some(root) = stmt.root() else { continue };
                let root_id: AnyNodeId = root.node_id().into();
                let erased = stmt.erase();
                catalog.accumulate_ddl(CatalogLayer::Database, &erased, root_id, &dialect);
                catalog.record_ddl_definition_site(&erased, root_id, &dialect, file_uri);
            }
        }
        (catalog, errors)
    }

    /// Parse a JSON schema description into the database layer.
    ///
    /// Expected format:
    /// ```json
    /// {
    ///   "tables":    [{ "name": "users",        "columns": ["id", "name"] }],
    ///   "views":     [{ "name": "active_users", "columns": ["id"] }],
    ///   "functions": [{ "name": "my_func",      "args": 2 }]
    /// }
    /// ```
    #[cfg(feature = "serde-json")]
    pub(crate) fn from_json(dialect: impl Into<AnyDialect>, s: &str) -> Result<Self, String> {
        #[derive(serde::Deserialize)]
        struct Root {
            #[serde(default)]
            tables: Vec<TableInput>,
            #[serde(default)]
            views: Vec<TableInput>,
            #[serde(default)]
            functions: Vec<FunctionInput>,
        }
        #[derive(serde::Deserialize)]
        struct TableInput {
            name: String,
            #[serde(default)]
            columns: Vec<String>,
        }
        #[derive(serde::Deserialize)]
        struct FunctionInput {
            name: String,
            args: Option<usize>,
        }

        let dialect = dialect.into();
        let root: Root =
            serde_json::from_str(s).map_err(|e| format!("invalid catalog JSON: {e}"))?;

        let mut catalog = Catalog::new(dialect);
        let db = catalog.layer_mut(CatalogLayer::Database);
        for t in root.tables {
            // Empty column list means "unknown columns — accept any ref conservatively".
            // Only use Some(cols) when columns are explicitly specified.
            let cols = if t.columns.is_empty() {
                None
            } else {
                Some(t.columns.iter().map(|c| c.to_ascii_lowercase()).collect())
            };
            db.insert_table(t.name, cols, false);
        }
        for v in root.views {
            let cols = if v.columns.is_empty() {
                None
            } else {
                Some(v.columns.iter().map(|c| c.to_ascii_lowercase()).collect())
            };
            db.insert_view(v.name, cols);
        }
        for f in root.functions {
            let arity = match f.args {
                Some(n) => AritySpec::Exact(n),
                None => AritySpec::Any,
            };
            db.insert_function_overload(f.name, FunctionCategory::Scalar, arity);
        }
        Ok(catalog)
    }

    // ── DDL accumulation ──────────────────────────────────────────────────────

    /// Extract DDL contributions from a parsed statement and insert them into
    /// `target`. Temporary objects are always routed to the Connection layer.
    ///
    /// Called statement-by-statement during analysis so that later statements
    /// can reference earlier DDL. Pass `CatalogLayer::Document` for inline DDL
    /// and `CatalogLayer::Database` when pre-populating a schema.
    pub(crate) fn accumulate_ddl(
        &mut self,
        target: CatalogLayer,
        stmt: &AnyParsedStatement<'_>,
        root: AnyNodeId,
        dialect: &AnyDialect,
    ) {
        let Some((tag, fields)) = stmt.extract_fields(root) else {
            return;
        };
        let idx = u32::from(tag) as usize;
        let Some(&role) = dialect.roles().get(idx) else {
            return;
        };

        match role {
            SemanticRole::DefineTable {
                name,
                columns,
                select,
                without_rowid,
            } => {
                let name_val = match fields[name as usize] {
                    FieldValue::Span { text: s, .. } if !s.is_empty() => s.to_string(),
                    _ => return,
                };
                let cols = extract_columns(
                    stmt,
                    &fields,
                    opt_field(columns),
                    opt_field(select),
                    dialect.roles(),
                );
                let is_without_rowid = without_rowid.field != FIELD_ABSENT
                    && matches!(
                        fields[without_rowid.field as usize],
                        FieldValue::Flags(f) if without_rowid.is_set(f)
                    );
                self.layers[target.index()].insert_table(name_val, cols, is_without_rowid);
            }
            SemanticRole::DefineView {
                name,
                columns,
                select,
            } => {
                let name_val = match fields[name as usize] {
                    FieldValue::Span { text: s, .. } if !s.is_empty() => s.to_string(),
                    _ => return,
                };
                let cols = extract_columns(
                    stmt,
                    &fields,
                    opt_field(columns),
                    Some(select),
                    dialect.roles(),
                );
                self.layers[target.index()].insert_view(name_val, cols);
            }
            SemanticRole::DefineFunction {
                name,
                args,
                return_type,
                ..
            } => {
                let name_val = match fields[name as usize] {
                    FieldValue::Span { text: s, .. } if !s.is_empty() => s.to_string(),
                    _ => return,
                };
                let arity = extract_function_arity(stmt, &fields, opt_field(args));
                let layer = &mut self.layers[target.index()];
                layer.insert_function_overload(name_val.clone(), FunctionCategory::Scalar, arity);
                if is_table_returning(stmt, &fields, opt_field(return_type), dialect.roles()) {
                    layer.insert_table_function(name_val, AritySpec::Any, Vec::new());
                }
            }
            // Non-DDL roles are irrelevant to catalog accumulation.
            _ => {}
        }
    }

    // ── Query scope management ────────────────────────────────────────────────

    /// Push a new empty scope frame. Called on subquery / CTE entry.
    pub(crate) fn push_query_scope(&mut self) {
        self.layers.push(CatalogLayerContents::default());
    }

    /// Pop the innermost scope frame. Called on subquery / CTE exit.
    pub(crate) fn pop_query_scope(&mut self) {
        if self.layers.len() > FIXED_LAYER_COUNT {
            self.layers.pop();
        }
    }

    /// Register a table or alias in the current (innermost) query scope.
    /// `columns = None` means the table exists but its column list is unknown —
    /// column references against it are conservatively accepted.
    pub(crate) fn add_query_table(&mut self, name: &str, columns: Option<Vec<String>>) {
        if let Some(frame) = self.layers[FIXED_LAYER_COUNT..].last_mut() {
            frame.insert_table(name, columns, false);
        }
    }

    // ── Schema sync (used by SemanticAnalyzer) ────────────────────────────────

    /// Copy the Database and Connection layers from `src` into this catalog.
    ///
    /// Called at the start of each Document-mode analysis pass.
    pub(crate) fn copy_schema_layers_from(&mut self, src: &Catalog) {
        self.layers[LAYER_DATABASE] = src.layers[LAYER_DATABASE].clone();
        self.layers[LAYER_CONNECTION] = src.layers[LAYER_CONNECTION].clone();
    }

    /// Copy only the Database layer from `src`, preserving this catalog's
    /// Connection layer.
    ///
    /// Called at the start of each Execute-mode analysis pass — the Connection
    /// layer accumulates executed DDL and must not be overwritten.
    pub(crate) fn copy_database_from(&mut self, src: &Catalog) {
        self.layers[LAYER_DATABASE] = src.layers[LAYER_DATABASE].clone();
    }

    /// Merge DDL discovered in the Document layer into the Connection layer.
    ///
    /// Called after each Execute-mode analysis pass so that DDL persists across
    /// subsequent `analyze()` calls.
    pub(crate) fn promote_document_to_connection(&mut self) {
        // Clone first to satisfy the borrow checker.
        let doc = self.layers[LAYER_DOCUMENT].clone();
        self.layers[LAYER_CONNECTION].merge_from(&doc);
    }

    // ── Resolution ────────────────────────────────────────────────────────────

    /// Returns `true` if `name` is a known relation in any layer.
    pub(crate) fn resolve_relation(&self, name: &str) -> bool {
        self.all_layers_ordered()
            .any(|layer| layer.relation(name).is_some())
    }

    /// Returns `true` if `name` is a view in any layer.
    pub(crate) fn is_view(&self, name: &str) -> bool {
        self.all_layers_ordered()
            .any(|layer| layer.relation(name).is_some_and(|r| r.is_view))
    }

    /// Returns `true` if `name` is a known table-valued function in any layer.
    pub(crate) fn resolve_table_function(&self, name: &str) -> bool {
        self.all_layers_ordered()
            .any(|layer| layer.table_function(name).is_some())
    }

    pub(crate) fn check_function(&self, name: &str, arg_count: usize) -> FunctionCheckResult {
        let set = self
            .all_layers_ordered()
            .find_map(|layer| layer.function(name));
        let Some(set) = set else {
            return FunctionCheckResult::Unknown;
        };
        if set
            .overloads
            .iter()
            .copied()
            .any(|ov| overload_accepts(ov, arg_count))
        {
            return FunctionCheckResult::Ok;
        }
        FunctionCheckResult::WrongArity {
            expected: expected_fixed_arities(set),
        }
    }

    /// Return the column list and WITHOUT ROWID flag for a table source.
    ///
    /// Returns `(columns, without_rowid)`:
    /// - `columns = Some(cols)` — known column list.
    /// - `columns = None` — not found, or columns unknown (accept any ref).
    /// - `without_rowid = true` — no implicit rowid/oid/_rowid_ column.
    pub(crate) fn table_source_info(&self, name: &str) -> (Option<Vec<String>>, bool) {
        for layer in self.all_layers_ordered() {
            if let Some(rel) = layer.relation(name) {
                return (rel.columns.clone(), rel.without_rowid);
            }
            if let Some(tf) = layer.table_function(name) {
                let cols = if tf.output_columns.is_empty() {
                    None
                } else {
                    Some(tf.output_columns.clone())
                };
                return (cols, false);
            }
        }
        (None, false)
    }

    /// Record definition sites for a DDL statement (table + columns), if `file_uri` is provided.
    fn record_ddl_definition_site(
        &mut self,
        stmt: &AnyParsedStatement<'_>,
        root: AnyNodeId,
        dialect: &AnyDialect,
        file_uri: Option<&str>,
    ) {
        let Some(uri) = file_uri else { return };
        let Some((name, start, end)) = ddl_name_span(stmt, root, dialect.roles()) else {
            return;
        };
        let key = canonical_name(&name);
        let Some(entry) = self.layers[CatalogLayer::Database.index()]
            .relations
            .get_mut(&key)
        else {
            return;
        };
        entry.definition_site = Some(DefinitionSite {
            file_uri: uri.to_string(),
            start,
            end,
        });

        // Also record column definition sites.
        let col_sites = ddl_column_spans(stmt, root, dialect.roles());
        for (col_name, col_start, col_end) in col_sites {
            entry.column_definition_sites.insert(
                col_name,
                DefinitionSite {
                    file_uri: uri.to_string(),
                    start: col_start,
                    end: col_end,
                },
            );
        }
    }

    /// Return the definition site for a column in a relation, if one was recorded.
    pub(crate) fn column_definition_site(
        &self,
        table: &str,
        column: &str,
    ) -> Option<&DefinitionSite> {
        let col_key = canonical_name(column);
        for layer in self.all_layers_ordered() {
            if let Some(rel) = layer.relation(table) {
                return rel.column_definition_sites.get(&col_key);
            }
        }
        None
    }

    /// Return the definition site for a relation, if one was recorded.
    pub(crate) fn relation_definition_site(&self, name: &str) -> Option<&DefinitionSite> {
        for layer in self.all_layers_ordered() {
            if let Some(rel) = layer.relation(name) {
                return rel.definition_site.as_ref();
            }
        }
        None
    }

    // ── Enumeration (for fuzzy suggestions and completions) ───────────────────

    pub(crate) fn all_relation_names(&self) -> Vec<String> {
        let mut seen = HashSet::new();
        let mut out = Vec::new();
        for layer in self.all_layers_ordered() {
            for rel in layer.relations.values() {
                push_unique(&mut seen, &mut out, &rel.name);
            }
        }
        out.sort_unstable_by_key(|n| canonical_name(n));
        out
    }

    pub(crate) fn all_column_names(&self, table: Option<&str>) -> Vec<String> {
        let mut names = Vec::new();
        for layer in self.all_layers_ordered() {
            for rel in layer.relations.values() {
                if table.is_none_or(|t| rel.name.eq_ignore_ascii_case(t))
                    && let Some(cols) = &rel.columns
                {
                    names.extend(cols.iter().map(|c| c.to_ascii_lowercase()));
                }
            }
        }
        names.sort_unstable();
        names.dedup();
        names
    }

    /// Look up function metadata by name: returns (category, arities) if found.
    pub(crate) fn function_signature(
        &self,
        name: &str,
    ) -> Option<(FunctionCategory, Vec<AritySpec>)> {
        let set = self
            .all_layers_ordered()
            .find_map(|layer| layer.function(name))?;
        let category = set
            .overloads
            .first()
            .map_or(FunctionCategory::Scalar, |ov| ov.category);
        let arities: Vec<AritySpec> = set.overloads.iter().map(|ov| ov.arity).collect();
        Some((category, arities))
    }

    pub(crate) fn all_function_names(&self) -> Vec<String> {
        let mut seen = HashSet::new();
        let mut out = Vec::new();
        for layer in self.all_layers_ordered() {
            for f in layer.functions.values() {
                push_unique(&mut seen, &mut out, &f.name);
            }
        }
        out.sort_unstable_by_key(|n| canonical_name(n));
        out
    }

    pub(crate) fn all_table_function_names(&self) -> Vec<String> {
        let mut seen = HashSet::new();
        let mut out = Vec::new();
        for layer in self.all_layers_ordered() {
            for tf in layer.table_functions.values() {
                push_unique(&mut seen, &mut out, &tf.name);
            }
        }
        out.sort_unstable_by_key(|n| canonical_name(n));
        out
    }

    // ── Private helpers ───────────────────────────────────────────────────────

    /// Iterator over all layers in resolution priority order:
    /// query (innermost first) → document → connection → database → dialect.
    fn all_layers_ordered(&self) -> impl Iterator<Item = &CatalogLayerContents> {
        self.layers.iter().rev()
    }
}

// ── DDL extraction helpers ────────────────────────────────────────────────────

/// Extract the definition name and byte-offset range from a DDL statement.
///
/// Returns `(lowercase_name, start, end)` for CREATE TABLE / CREATE VIEW,
/// or `None` for non-DDL statements.
fn ddl_name_span(
    stmt: &AnyParsedStatement<'_>,
    root: AnyNodeId,
    roles: &[SemanticRole],
) -> Option<(String, usize, usize)> {
    let (tag, fields) = stmt.extract_fields(root)?;
    let role = roles.get(u32::from(tag) as usize)?;
    let name_idx = match role {
        SemanticRole::DefineTable { name, .. } | SemanticRole::DefineView { name, .. } => *name,
        _ => return None,
    };
    let FieldValue::Span { text: s, .. } = fields[name_idx as usize] else {
        return None;
    };
    if s.is_empty() {
        return None;
    }
    let off = s.as_ptr() as usize - stmt.source().as_ptr() as usize;
    Some((s.to_ascii_lowercase(), off, off + s.len()))
}

/// Extract column name spans from a `CREATE TABLE` statement.
///
/// Returns `[(lowercase_name, start, end)]` for each `ColumnDef` child.
pub(crate) fn ddl_column_spans(
    stmt: &AnyParsedStatement<'_>,
    root: AnyNodeId,
    roles: &[SemanticRole],
) -> Vec<(String, usize, usize)> {
    let mut out = Vec::new();
    let Some((tag, fields)) = stmt.extract_fields(root) else {
        return out;
    };
    let Some(&SemanticRole::DefineTable { columns, .. }) = roles.get(u32::from(tag) as usize)
    else {
        return out;
    };
    if columns == FIELD_ABSENT {
        return out;
    }
    let FieldValue::NodeId(col_list_id) = fields[columns as usize] else {
        return out;
    };
    if col_list_id.is_null() {
        return out;
    }
    let Some(children) = stmt.list_children(col_list_id) else {
        return out;
    };
    let source_start = stmt.source().as_ptr() as usize;

    for &child_id in children {
        if child_id.is_null() {
            continue;
        }
        if let Some((name, start, end)) = column_def_name_span(stmt, child_id, roles, source_start)
        {
            out.push((name.to_ascii_lowercase(), start, end));
        }
    }
    out
}

/// Extract the name and byte offset of a single `ColumnDef` node.
fn column_def_name_span<'a>(
    stmt: &AnyParsedStatement<'a>,
    node_id: AnyNodeId,
    roles: &[SemanticRole],
    source_start: usize,
) -> Option<(&'a str, usize, usize)> {
    let (tag, fields) = stmt.extract_fields(node_id)?;
    let SemanticRole::ColumnDef { name: name_idx, .. } = roles.get(u32::from(tag) as usize)? else {
        return None;
    };
    let FieldValue::NodeId(name_id) = fields[*name_idx as usize] else {
        return None;
    };
    if name_id.is_null() {
        return None;
    }
    let (_, name_fields) = stmt.extract_fields(name_id)?;
    for j in 0..name_fields.len() {
        if let FieldValue::Span { text: s, .. } = name_fields[j]
            && !s.is_empty()
        {
            let off = s.as_ptr() as usize - source_start;
            return Some((s, off, off + s.len()));
        }
    }
    None
}

/// Extract columns for a table/view DDL contribution.
///
/// Tries the explicit column list first; if absent, infers column names from
/// the result columns of the AS-SELECT body. Returns `None` only when
/// inference is impossible (e.g. `SELECT *`), which tells the catalog to
/// accept any column reference conservatively.
fn extract_columns<'a>(
    stmt: &AnyParsedStatement<'a>,
    fields: &NodeFields<'a>,
    columns_field: Option<u8>,
    select_field: Option<u8>,
    roles: &'static [SemanticRole],
) -> Option<Vec<String>> {
    // Explicit column list takes priority.
    if let Some(col_idx) = columns_field
        && let FieldValue::NodeId(col_list_id) = fields[col_idx as usize]
        && !col_list_id.is_null()
    {
        let mut columns = Vec::new();
        columns_from_column_list(stmt, col_list_id, roles, &mut columns);
        if !columns.is_empty() {
            return Some(columns);
        }
    }

    // Fall back to inferring names from SELECT result columns.
    if let Some(sel_idx) = select_field
        && let FieldValue::NodeId(select_id) = fields[sel_idx as usize]
        && !select_id.is_null()
    {
        return columns_from_select(stmt, select_id, roles);
    }

    None
}

/// Check whether a DDL function returns a table.
fn is_table_returning<'a>(
    stmt: &AnyParsedStatement<'a>,
    fields: &NodeFields<'a>,
    return_type_field: Option<u8>,
    roles: &'static [SemanticRole],
) -> bool {
    let Some(rt_idx) = return_type_field else {
        return false;
    };
    let FieldValue::NodeId(rt_id) = fields[rt_idx as usize] else {
        return false;
    };
    if rt_id.is_null() {
        return false;
    }
    let Some((rt_tag, rt_fields)) = stmt.extract_fields(rt_id) else {
        return false;
    };
    let tag_idx = u32::from(rt_tag) as usize;
    let Some(&SemanticRole::ReturnSpec { columns }) = roles.get(tag_idx) else {
        return false;
    };
    if columns == FIELD_ABSENT {
        return false;
    }
    matches!(rt_fields[columns as usize], FieldValue::NodeId(id) if !id.is_null())
}

/// Extract argument count for a function DDL contribution.
fn extract_function_arity<'a>(
    stmt: &AnyParsedStatement<'a>,
    fields: &NodeFields<'a>,
    args_field: Option<u8>,
) -> AritySpec {
    let Some(args_idx) = args_field else {
        return AritySpec::Any;
    };
    let FieldValue::NodeId(args_id) = fields[args_idx as usize] else {
        return AritySpec::Any;
    };
    if args_id.is_null() {
        return AritySpec::Any;
    }
    let Some(children) = stmt.list_children(args_id) else {
        return AritySpec::Any;
    };
    AritySpec::Exact(children.len())
}

fn columns_from_column_list(
    stmt: &AnyParsedStatement<'_>,
    list_id: AnyNodeId,
    roles: &'static [SemanticRole],
    out: &mut Vec<String>,
) {
    let Some(children) = stmt.list_children(list_id) else {
        return;
    };

    for &child_id in children {
        if child_id.is_null() {
            continue;
        }
        let Some((child_tag, child_fields)) = stmt.extract_fields(child_id) else {
            continue;
        };

        // Use the ColumnDef role to find the name field precisely.
        let tag_idx = u32::from(child_tag) as usize;
        let Some(&SemanticRole::ColumnDef { name: name_idx, .. }) = roles.get(tag_idx) else {
            continue;
        };

        let FieldValue::NodeId(name_id) = child_fields[name_idx as usize] else {
            continue;
        };
        if name_id.is_null() {
            continue;
        }
        let Some((_, name_fields)) = stmt.extract_fields(name_id) else {
            continue;
        };
        // The first non-empty Span inside the name node is the identifier text.
        for j in 0..name_fields.len() {
            if let FieldValue::Span { text: s, .. } = name_fields[j]
                && !s.is_empty()
            {
                out.push(s.to_ascii_lowercase());
                break;
            }
        }
    }
}

/// Infer column names from a SELECT statement's result column list.
///
/// Returns `Some(names)` if every result column has an inferable name:
/// - An explicit alias is used as-is.
/// - A bare `ColumnRef` with no alias uses the column name.
///
/// Returns `None` if any result column uses `*` (STAR) or has an expression
/// that cannot be named (e.g. a literal or function call without an alias).
/// A `None` return causes the caller to register the table with unknown
/// columns, conservatively accepting all column references.
pub(super) fn columns_from_select(
    stmt: &AnyParsedStatement<'_>,
    select_id: AnyNodeId,
    roles: &[SemanticRole],
) -> Option<Vec<String>> {
    let (select_tag, select_fields) = stmt.extract_fields(select_id)?;
    let select_role = roles.get(u32::from(select_tag) as usize).copied()?;

    let SemanticRole::Query {
        columns: cols_idx, ..
    } = select_role
    else {
        return None;
    };

    let FieldValue::NodeId(list_id) = select_fields[cols_idx as usize] else {
        return None;
    };
    if list_id.is_null() {
        return None;
    }

    let children = stmt.list_children(list_id)?;
    let mut out = Vec::new();

    for &child_id in children {
        if child_id.is_null() {
            continue;
        }
        let (child_tag, child_fields) = stmt.extract_fields(child_id)?;
        let child_role = roles
            .get(u32::from(child_tag) as usize)
            .copied()
            .unwrap_or(SemanticRole::Transparent);

        let SemanticRole::ResultColumn {
            flags: flags_idx,
            alias: alias_idx,
            expr: expr_idx,
        } = child_role
        else {
            continue;
        };

        // STAR flag (bit 0) → wildcard: can't enumerate columns.
        if let FieldValue::Flags(f) = child_fields[flags_idx as usize]
            && f & 1 != 0
        {
            return None;
        }

        match infer_result_col_name(stmt, &child_fields, alias_idx, expr_idx, roles) {
            Some(name) => out.push(name),
            None => return None,
        }
    }

    if out.is_empty() { None } else { Some(out) }
}

/// Infer the output column name for a single result column node.
///
/// Mirrors `SQLite`'s `sqlite3ExprListSetName` / `sqlite3ExprListSetSpan` logic:
/// 1. Explicit alias → use alias text (already stripped of quotes by the grammar).
/// 2. Bare `ColumnRef` with no alias → use the column-name span.
/// 3. Any other expression with no alias → use the raw source text of the
///    expression node (`SQLite` calls this `ENAME_SPAN`, stored by
///    `sqlite3ExprListSetSpan`). For `SELECT 1` this gives `"1"`;
///    for `SELECT 1+2` it gives `"1+2"`; etc.
fn infer_result_col_name<'a>(
    stmt: &AnyParsedStatement<'a>,
    child_fields: &NodeFields<'a>,
    alias_idx: u8,
    expr_idx: u8,
    roles: &[SemanticRole],
) -> Option<String> {
    // Try explicit alias.
    if let FieldValue::NodeId(alias_id) = child_fields[alias_idx as usize]
        && !alias_id.is_null()
        && let Some((_, alias_fields)) = stmt.extract_fields(alias_id)
    {
        for j in 0..alias_fields.len() {
            if let FieldValue::Span { text: s, .. } = alias_fields[j]
                && !s.is_empty()
            {
                return Some(s.to_ascii_lowercase());
            }
        }
    }

    let FieldValue::NodeId(expr_id) = child_fields[expr_idx as usize] else {
        return None;
    };
    if expr_id.is_null() {
        return None;
    }
    let (expr_tag, expr_fields) = stmt.extract_fields(expr_id)?;

    // Try bare ColumnRef (no alias) → use column name.
    let expr_role = roles
        .get(u32::from(expr_tag) as usize)
        .copied()
        .unwrap_or(SemanticRole::Transparent);
    if let SemanticRole::ColumnRef {
        column: col_idx, ..
    } = expr_role
        && let FieldValue::Span { text: col_span, .. } = expr_fields[col_idx as usize]
        && !col_span.is_empty()
    {
        return Some(col_span.to_ascii_lowercase());
    }

    // Fallback: use the raw source text spanned by the expression node
    // (SQLite's ENAME_SPAN — set by sqlite3ExprListSetSpan).
    // Collect all Span values in the subtree; the byte range [min, max)
    // gives the source text of the expression, including any operators or
    // parentheses that sit between leaf spans.
    expr_source_text(stmt, expr_id).map(str::to_ascii_lowercase)
}

/// Extract the source text of an expression node by recursively collecting
/// all `Span` field values in its subtree and taking the enclosing byte range.
pub(super) fn expr_source_text<'a>(
    stmt: &AnyParsedStatement<'a>,
    id: AnyNodeId,
) -> Option<&'a str> {
    let source = stmt.source();
    let base = source.as_ptr() as usize;
    let mut min = usize::MAX;
    let mut max = 0usize;
    collect_spans(stmt, id, base, &mut min, &mut max);
    if min < max {
        Some(&source[min..max])
    } else {
        None
    }
}

/// Walk `id` and all its descendants, updating `[min, max)` with every `Span`.
fn collect_spans(
    stmt: &AnyParsedStatement<'_>,
    id: AnyNodeId,
    base: usize,
    min: &mut usize,
    max: &mut usize,
) {
    if id.is_null() {
        return;
    }
    if let Some((_, fields)) = stmt.extract_fields(id) {
        for i in 0..fields.len() {
            match fields[i] {
                FieldValue::Span { text: s, .. } if !s.is_empty() => {
                    let start = s.as_ptr() as usize - base;
                    let end = start + s.len();
                    if start < *min {
                        *min = start;
                    }
                    if end > *max {
                        *max = end;
                    }
                }
                FieldValue::NodeId(child) if !child.is_null() => {
                    collect_spans(stmt, child, base, min, max);
                }
                _ => {}
            }
        }
    }
    // Also descend into list children (e.g. ExprList inside a FunctionCall).
    if let Some(children) = stmt.list_children(id) {
        for &child in children {
            collect_spans(stmt, child, base, min, max);
        }
    }
}

// ── Dialect layer builder ─────────────────────────────────────────────────────

fn build_dialect_layer(layer: &mut CatalogLayerContents, dialect: &AnyDialect) {
    #[cfg(feature = "sqlite")]
    for entry in crate::sqlite::functions_catalog::SQLITE_FUNCTIONS {
        if !is_function_available(entry, dialect) {
            continue;
        }
        if entry.info.category == DialectFunctionCategory::TableValued {
            layer.insert_table_function(entry.info.name.to_string(), AritySpec::Any, Vec::new());
        } else {
            layer.insert_function_arities(
                entry.info.name.to_string(),
                map_function_category(entry.info.category),
                entry.info.arities,
            );
        }
    }
}

fn map_function_category(category: DialectFunctionCategory) -> FunctionCategory {
    match category {
        DialectFunctionCategory::Scalar | DialectFunctionCategory::TableValued => {
            FunctionCategory::Scalar // TableValued is unreachable via this path
        }
        DialectFunctionCategory::Aggregate => FunctionCategory::Aggregate,
        DialectFunctionCategory::Window => FunctionCategory::Window,
    }
}

// ── Utilities ─────────────────────────────────────────────────────────────────

fn canonical_name(name: &str) -> String {
    name.to_ascii_lowercase()
}

fn overload_accepts(overload: FunctionOverload, arg_count: usize) -> bool {
    match overload.arity {
        AritySpec::Exact(n) => n == arg_count,
        AritySpec::AtLeast(min) => arg_count >= min,
        AritySpec::Any => true,
    }
}

fn expected_fixed_arities(set: &FunctionSet) -> Vec<usize> {
    let mut arities: Vec<usize> = set
        .overloads
        .iter()
        .filter_map(|ov| match ov.arity {
            AritySpec::Exact(n) => Some(n),
            AritySpec::AtLeast(_) | AritySpec::Any => None,
        })
        .collect();
    arities.sort_unstable();
    arities.dedup();
    arities
}

fn push_unique(seen: &mut HashSet<String>, out: &mut Vec<String>, name: &str) {
    let lower = canonical_name(name);
    if seen.insert(lower) {
        out.push(name.to_string());
    }
}
