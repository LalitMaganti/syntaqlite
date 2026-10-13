// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Layered semantic catalog.
//!
//! Resolution order: query (innermost frame first) → document → connection → database → dialect.

use std::borrow::Cow;
use std::collections::{HashMap, HashSet};

use syntaqlite_syntax::any::{AnyNodeId, AnyParsedStatement, FieldValue, NodeFields};

use super::name_key::NameKey;
use super::result_columns::ColumnResolver;
use super::stmt_reader::StmtReader;
use crate::dialect::AnyDialect;
use crate::dialect::{FIELD_ABSENT, FunctionCategory as DialectFunctionCategory, SemanticRole};

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

impl FunctionOverload {
    fn accepts(self, arg_count: usize) -> bool {
        match self.arity {
            AritySpec::Exact(n) => n == arg_count,
            AritySpec::AtLeast(min) => arg_count >= min,
            AritySpec::Any => true,
        }
    }
}

#[derive(Debug, Clone)]
struct FunctionSet {
    name: String,
    overloads: Vec<FunctionOverload>,
}

impl FunctionSet {
    /// All exact arities accepted by this function, sorted and deduped.
    /// Excludes variadic overloads.
    fn fixed_arities(&self) -> Vec<usize> {
        let mut arities: Vec<usize> = self
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
}

impl From<DialectFunctionCategory> for FunctionCategory {
    fn from(category: DialectFunctionCategory) -> Self {
        match category {
            DialectFunctionCategory::Scalar | DialectFunctionCategory::TableValued => {
                Self::Scalar // TableValued is unreachable via this path
            }
            DialectFunctionCategory::Aggregate => Self::Aggregate,
            DialectFunctionCategory::Window => Self::Window,
        }
    }
}

/// How a table column takes part in `SELECT *` and in an `INSERT` without a
/// column list. Every column can be referenced by name.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
#[cfg_attr(
    feature = "serde",
    derive(serde::Deserialize),
    serde(rename_all = "lowercase")
)]
pub enum ColumnKind {
    /// An ordinary column.
    #[default]
    Normal,
    /// A generated column: included in `SELECT *`, but not filled by an
    /// `INSERT` without a column list.
    Generated,
    /// A hidden virtual-table column: in neither `SELECT *` nor an `INSERT`
    /// without a column list.
    Hidden,
}

/// A column of a [`TableDef`] or [`ViewDef`].
///
/// Converts from a name for an ordinary column.
///
/// ```
/// # use syntaqlite::analysis::{Column, ColumnKind};
/// let plain: Column = "id".into();
/// assert_eq!(plain.kind(), ColumnKind::Normal);
/// assert_eq!(Column::generated("total").kind(), ColumnKind::Generated);
/// ```
#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Column {
    name: String,
    kind: ColumnKind,
}

impl Column {
    /// A column of the given kind.
    pub fn new(name: impl Into<String>, kind: ColumnKind) -> Self {
        Column {
            name: name.into(),
            kind,
        }
    }

    /// A generated column.
    pub fn generated(name: impl Into<String>) -> Self {
        Self::new(name, ColumnKind::Generated)
    }

    /// A hidden virtual-table column.
    pub fn hidden(name: impl Into<String>) -> Self {
        Self::new(name, ColumnKind::Hidden)
    }

    /// The column name.
    pub fn name(&self) -> &str {
        &self.name
    }

    /// How the column takes part in `SELECT *` and `INSERT`.
    pub fn kind(&self) -> ColumnKind {
        self.kind
    }
}

impl From<&str> for Column {
    fn from(name: &str) -> Self {
        Self::new(name, ColumnKind::Normal)
    }
}

impl From<String> for Column {
    fn from(name: String) -> Self {
        Self::new(name, ColumnKind::Normal)
    }
}

/// Deserializes from a name, or from `{"name": ..., "kind": ...}` where
/// `kind` is `"normal"` (the default), `"generated"` or `"hidden"`.
#[cfg(feature = "serde")]
impl<'de> serde::Deserialize<'de> for Column {
    fn deserialize<D: serde::Deserializer<'de>>(deserializer: D) -> Result<Self, D::Error> {
        #[derive(serde::Deserialize)]
        #[serde(untagged, expecting = "a column name or {\"name\", \"kind\"}")]
        enum Repr {
            Name(String),
            Def(Def),
        }
        #[derive(serde::Deserialize)]
        #[serde(deny_unknown_fields)]
        struct Def {
            name: String,
            #[serde(default)]
            kind: ColumnKind,
        }
        Ok(match Repr::deserialize(deserializer)? {
            Repr::Name(name) => name.into(),
            Repr::Def(def) => Column::new(def.name, def.kind),
        })
    }
}

/// A table for [`CatalogLayerContents::insert_table`].
///
/// Its columns are unknown until [`columns`](Self::columns) is called, and
/// any column reference against it is accepted.
///
/// Deserializes from `{"name": ..., "columns": [...]}`, where a missing or
/// `null` column list means the columns are unknown.
#[derive(Debug, Clone, PartialEq, Eq)]
#[cfg_attr(feature = "serde", derive(serde::Deserialize))]
pub struct TableDef {
    name: String,
    #[cfg_attr(feature = "serde", serde(default))]
    columns: Option<Vec<Column>>,
    #[cfg_attr(feature = "serde", serde(skip))]
    without_rowid: bool,
}

impl TableDef {
    /// A table whose columns are unknown.
    pub fn new(name: impl Into<String>) -> Self {
        Self {
            name: name.into(),
            columns: None,
            without_rowid: false,
        }
    }

    /// Set the table's columns, in order.
    #[must_use]
    pub fn columns(mut self, columns: impl IntoIterator<Item = impl Into<Column>>) -> Self {
        self.columns = Some(columns.into_iter().map(Into::into).collect());
        self
    }

    /// Mark the table `WITHOUT ROWID`, so it has no implicit `rowid` column.
    #[must_use]
    pub fn without_rowid(mut self) -> Self {
        self.without_rowid = true;
        self
    }
}

/// A view for [`CatalogLayerContents::insert_view`].
///
/// Its columns are unknown until [`columns`](Self::columns) is called, and
/// any column reference against it is accepted.
///
/// Deserializes like [`TableDef`].
#[derive(Debug, Clone, PartialEq, Eq)]
#[cfg_attr(feature = "serde", derive(serde::Deserialize))]
pub struct ViewDef {
    name: String,
    #[cfg_attr(feature = "serde", serde(default))]
    columns: Option<Vec<Column>>,
}

impl ViewDef {
    /// A view whose columns are unknown.
    pub fn new(name: impl Into<String>) -> Self {
        Self {
            name: name.into(),
            columns: None,
        }
    }

    /// Set the view's columns, in order.
    #[must_use]
    pub fn columns(mut self, columns: impl IntoIterator<Item = impl Into<Column>>) -> Self {
        self.columns = Some(columns.into_iter().map(Into::into).collect());
        self
    }
}

#[derive(Debug, Clone)]
struct RelationEntry {
    name: String,
    /// `None` = table is known to exist but column list is not tracked.
    /// Column references against it are conservatively accepted.
    columns: Option<Vec<Column>>,
    /// `true` for WITHOUT ROWID tables — no implicit rowid/oid/_rowid_ columns.
    without_rowid: bool,
    /// `true` if this relation is a view (not a physical table).
    is_view: bool,
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
    /// The qualifier names no source anywhere in the statement so far.
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
    relations: HashMap<NameKey, RelationEntry>,
    functions: HashMap<NameKey, FunctionSet>,
    table_functions: HashMap<NameKey, TableFunctionSet>,
    /// Modules whose DDL has been merged into this layer. Used to dedupe
    /// `INCLUDE PERFETTO MODULE` imports — once imported, re-imports no-op.
    imports: HashSet<String>,
    /// Relation names this layer masks from outer layers — a tombstone for
    /// `ALTER TABLE ... RENAME TO` / `DROP TABLE`. A name found here during
    /// innermost-first resolution reads as absent even if an outer layer
    /// still defines it.
    removed_relations: HashSet<NameKey>,
}

impl CatalogLayerContents {
    /// Remove all entries from this layer.
    fn clear(&mut self) {
        self.relations = HashMap::default();
        self.functions = HashMap::default();
        self.table_functions = HashMap::default();
        self.imports = HashSet::default();
        self.removed_relations = HashSet::default();
    }

    /// Merge all entries from `other` into this layer (existing keys are
    /// overwritten).
    ///
    /// `other`'s tombstones are applied as concrete removals: a name `other`
    /// masked is deleted from this layer's relations, so promoting a Document
    /// layer that renamed/dropped a relation actually retires it here.
    pub(crate) fn merge_from(&mut self, other: &Self) {
        for name in &other.removed_relations {
            self.relations.remove(name);
        }
        for (k, v) in &other.relations {
            self.relations.insert(k.clone(), v.clone());
        }
        self.functions
            .extend(other.functions.iter().map(|(k, v)| (k.clone(), v.clone())));
        self.table_functions.extend(
            other
                .table_functions
                .iter()
                .map(|(k, v)| (k.clone(), v.clone())),
        );
        self.imports.extend(other.imports.iter().cloned());
    }

    /// Record that a module's DDL has been merged into this layer.
    pub(crate) fn mark_imported(&mut self, module: impl Into<String>) {
        self.imports.insert(module.into());
    }

    /// Whether this layer already contains the given module's DDL.
    pub(crate) fn contains_import(&self, module: &str) -> bool {
        self.imports.contains(module)
    }

    /// Iterate over all relation (table/view) names in this layer.
    pub fn relation_names(&self) -> impl Iterator<Item = &str> {
        self.relations.keys().map(NameKey::as_str)
    }

    /// Insert a table into this layer.
    ///
    /// # Example
    ///
    /// ```
    /// # use syntaqlite::analysis::{CatalogLayer, Column, TableDef};
    /// # use syntaqlite::Catalog;
    /// let mut catalog = Catalog::new(syntaqlite::sqlite_dialect());
    /// let db = catalog.layer_mut(CatalogLayer::Database);
    ///
    /// // Known columns — misspelled column names will produce diagnostics.
    /// db.insert_table(TableDef::new("users").columns(["id", "name"]));
    ///
    /// // Generated and hidden columns are declared by kind.
    /// db.insert_table(TableDef::new("orders").columns([
    ///     Column::from("price"),
    ///     Column::from("qty"),
    ///     Column::generated("total"),
    /// ]));
    ///
    /// // Unknown columns — any column reference is accepted.
    /// db.insert_table(TableDef::new("external_data"));
    /// ```
    pub fn insert_table(&mut self, table: TableDef) {
        let TableDef {
            name,
            columns,
            without_rowid,
        } = table;
        self.insert_entry(
            &name.clone(),
            RelationEntry {
                name,
                columns,
                without_rowid,
                is_view: false,
            },
        );
    }

    /// Insert a view into this layer.
    ///
    /// Views behave like tables for resolution purposes but never expose an
    /// implicit `rowid` column.
    ///
    /// # Example
    ///
    /// ```
    /// # use syntaqlite::analysis::{CatalogLayer, ViewDef};
    /// # use syntaqlite::Catalog;
    /// let mut catalog = Catalog::new(syntaqlite::sqlite_dialect());
    /// catalog
    ///     .layer_mut(CatalogLayer::Database)
    ///     .insert_view(ViewDef::new("active_users").columns(["id", "name"]));
    /// ```
    pub fn insert_view(&mut self, view: ViewDef) {
        let ViewDef { name, columns } = view;
        self.insert_entry(
            &name.clone(),
            RelationEntry {
                name,
                columns,
                without_rowid: true,
                is_view: true,
            },
        );
    }

    /// Tombstone `name` in this layer so it reads as absent even when an outer
    /// layer still defines it. Also drops any same-layer definition.
    fn remove_relation(&mut self, name: &str) {
        let key = NameKey::new(name);
        self.relations.remove(&key);
        self.removed_relations.insert(key);
    }

    /// Insert a pre-built relation entry under `name`, clearing any tombstone.
    fn insert_entry(&mut self, name: &str, entry: RelationEntry) {
        let key = NameKey::new(name);
        self.removed_relations.remove(&key);
        self.relations.insert(key, entry);
    }

    /// Insert a single function overload into this layer.
    ///
    /// Use this to register application-defined functions so the analyzer can
    /// analyze calls and arity. Call multiple times with the same name to
    /// register multiple overloads (e.g. one accepting 1 argument and another
    /// accepting 2).
    ///
    /// # Example
    ///
    /// ```
    /// # use syntaqlite::analysis::CatalogLayer;
    /// # use syntaqlite::{AnalysisContext, Catalog, Analyzer};
    /// # use syntaqlite::analysis::{FunctionCategory, AritySpec};
    /// let mut catalog = Catalog::new(syntaqlite::sqlite_dialect());
    /// let db = catalog.layer_mut(CatalogLayer::Database);
    ///
    /// // Register a custom scalar function that takes exactly 2 arguments.
    /// db.insert_function_overload("my_concat", FunctionCategory::Scalar, AritySpec::Exact(2));
    ///
    /// // The analyzer now accepts calls to my_concat().
    /// let mut analyzer = Analyzer::new();
    /// let mut ctx = AnalysisContext::new(&mut catalog);
    /// let model = analyzer.analyze("SELECT my_concat('hello', 'world');", &mut ctx);
    /// assert!(!model.has_diagnostics());
    /// ```
    pub fn insert_function_overload(
        &mut self,
        name: impl Into<String>,
        category: FunctionCategory,
        arity: AritySpec,
    ) {
        let name = name.into();
        let key = NameKey::new(&name);
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
        let key = NameKey::new(&name);
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

    fn relation(&self, key: &NameKey) -> Option<&RelationEntry> {
        self.relations.get(key)
    }

    fn function(&self, key: &NameKey) -> Option<&FunctionSet> {
        self.functions.get(key)
    }

    fn table_function(&self, key: &NameKey) -> Option<&TableFunctionSet> {
        self.table_functions.get(key)
    }

    /// Populate this layer with the dialect's built-in functions and relations.
    fn populate_dialect_builtins(&mut self, dialect: &AnyDialect) {
        dialect_builtins::populate(self, dialect);
    }
}

#[cfg(feature = "sqlite")]
mod dialect_builtins {
    use super::{AritySpec, CatalogLayerContents, TableDef};
    use crate::dialect::{AnyDialect, FunctionCategory as DialectFunctionCategory, is_available};

    pub(super) fn populate(layer: &mut CatalogLayerContents, dialect: &AnyDialect) {
        for entry in crate::sqlite::functions_catalog::SQLITE_FUNCTIONS {
            if !is_available(entry.availability, dialect) {
                continue;
            }
            if entry.info.category == DialectFunctionCategory::TableValued {
                layer.insert_table_function(
                    entry.info.name.to_string(),
                    AritySpec::Any,
                    Vec::new(),
                );
            } else {
                layer.insert_function_arities(
                    entry.info.name.to_string(),
                    entry.info.category.into(),
                    entry.info.arities,
                );
            }
        }

        for entry in crate::sqlite::relations_catalog::SQLITE_RELATIONS {
            if !is_available(entry.availability, dialect) {
                continue;
            }
            let table = TableDef::new(entry.info.name).columns(entry.info.columns.iter().copied());
            layer.insert_table(if entry.info.without_rowid {
                table.without_rowid()
            } else {
                table
            });
        }
    }
}

#[cfg(not(feature = "sqlite"))]
mod dialect_builtins {
    use super::CatalogLayerContents;
    use crate::dialect::AnyDialect;

    pub(super) fn populate(_layer: &mut CatalogLayerContents, _dialect: &AnyDialect) {}
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
/// Use this to tell [`Analyzer`](super::engine::Analyzer)
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
/// # use syntaqlite::analysis::{CatalogLayer, TableDef};
/// # use syntaqlite::Catalog;
/// let mut catalog = Catalog::new(syntaqlite::sqlite_dialect());
///
/// // Register a table with known columns.
/// catalog
///     .layer_mut(CatalogLayer::Database)
///     .insert_table(TableDef::new("users").columns(["id", "name"]));
///
/// // Register a table whose columns are unknown — column references
/// // against it are conservatively accepted without warnings.
/// catalog
///     .layer_mut(CatalogLayer::Database)
///     .insert_table(TableDef::new("logs"));
/// ```
#[derive(Clone)]
pub struct Catalog {
    layers: Vec<CatalogLayerContents>,
    /// Relations the most recently accumulated statement removed, so its
    /// own existence check still sees them.
    retired_by_statement: HashSet<NameKey>,
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
    /// # use syntaqlite::analysis::{CatalogLayer, TableDef};
    /// # use syntaqlite::Catalog;
    /// let mut catalog = Catalog::new(syntaqlite::sqlite_dialect());
    ///
    /// catalog
    ///     .layer_mut(CatalogLayer::Database)
    ///     .insert_table(TableDef::new("orders").columns(["id", "total"]));
    /// ```
    pub fn new(dialect: impl Into<AnyDialect>) -> Self {
        let dialect = dialect.into();
        let mut layers = vec![CatalogLayerContents::default(); FIXED_LAYER_COUNT];
        layers[LAYER_DIALECT].populate_dialect_builtins(&dialect);
        Self {
            layers,
            retired_by_statement: HashSet::default(),
        }
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

    /// Whether a module has already been imported (i.e. its DDL merged into
    /// this catalog). Import resolution stores the flag on the Database layer
    /// since imports are database-lifetime.
    pub(crate) fn is_imported(&self, module: &str) -> bool {
        self.layers[LAYER_DATABASE].contains_import(module)
    }

    /// Record that a module's DDL has been merged into this catalog. Stored on
    /// the Database layer so the import remains cached for as long as the
    /// database does.
    pub(crate) fn mark_imported(&mut self, module: impl Into<String>) {
        self.layers[LAYER_DATABASE].mark_imported(module);
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
        self.retired_by_statement.clear();
    }

    // ── Convenience constructors ──────────────────────────────────────────────

    /// Parse DDL statements from one or more sources and populate the database
    /// layer.
    ///
    /// All sources are accumulated into a single catalog so that tables defined
    /// in earlier sources are visible to later ones.
    ///
    /// Returns `(catalog, errors)`. `errors` contains the human-readable
    /// message for each statement that failed to parse. Partial results from
    /// successfully parsed statements are always accumulated.
    #[cfg(feature = "sqlite")]
    pub fn from_ddl(dialect: impl Into<AnyDialect>, sources: &[&str]) -> (Self, Vec<String>) {
        use syntaqlite_syntax::ParseOutcome;
        let dialect = dialect.into();
        let mut catalog = Catalog::new(dialect.clone());
        let mut errors: Vec<String> = Vec::new();
        let parser = syntaqlite_syntax::Parser::new();
        for &source in sources {
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
            tables: Vec<TableDef>,
            #[serde(default)]
            views: Vec<ViewDef>,
            #[serde(default)]
            functions: Vec<FunctionInput>,
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
        for table in root.tables {
            db.insert_table(table);
        }
        for view in root.views {
            db.insert_view(view);
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
        self.retired_by_statement.clear();
        let Some((tag, fields)) = stmt.extract_fields(root) else {
            return;
        };
        let Some(&role) = dialect.roles().get(u32::from(tag) as usize) else {
            return;
        };
        let reader = StmtReader::new(stmt, dialect.roles());

        match role {
            SemanticRole::DefineTable {
                name,
                columns,
                select,
                without_rowid,
            } => {
                let Some(name_val) = stmt.span_field_ident_text(&fields, name) else {
                    return;
                };
                let normalized_name = name_val.into_owned();
                let cols = self.ddl_columns(stmt, dialect, &reader, &fields, columns, select);
                let is_without_rowid = without_rowid.field != FIELD_ABSENT
                    && matches!(
                        fields[without_rowid.field as usize],
                        FieldValue::Flags(f) if without_rowid.is_set(f)
                    );
                self.layers[target.index()].insert_table(TableDef {
                    name: normalized_name,
                    columns: cols,
                    without_rowid: is_without_rowid,
                });
            }
            SemanticRole::DefineView {
                name,
                columns,
                select,
            } => {
                let Some(name_val) = stmt.span_field_ident_text(&fields, name) else {
                    return;
                };
                let normalized_name = name_val.into_owned();
                let cols = self.ddl_columns(stmt, dialect, &reader, &fields, columns, select);
                self.layers[target.index()].insert_view(ViewDef {
                    name: normalized_name,
                    columns: cols,
                });
            }
            SemanticRole::DefineFunction {
                name,
                args,
                return_type,
                ..
            } => {
                let Some(name_val) = stmt.span_field_ident_text(&fields, name) else {
                    return;
                };
                let normalized_name = name_val.into_owned();
                let arity = reader.function_arity(&fields, args);
                let layer = &mut self.layers[target.index()];
                layer.insert_function_overload(
                    normalized_name.clone(),
                    FunctionCategory::Scalar,
                    arity,
                );
                if reader.is_table_returning(&fields, return_type) {
                    layer.insert_table_function(normalized_name, AritySpec::Any, Vec::new());
                }
            }
            SemanticRole::AlterTable {
                op,
                target: target_idx,
                new_name,
                old_name,
                column,
            } => {
                let op_name = stmt.enum_name(root, op);
                self.accumulate_alter_table(
                    target, stmt, &reader, &fields, op_name, target_idx, new_name, old_name, column,
                );
            }
            SemanticRole::DropObject {
                kind,
                target: target_idx,
                ..
            } => {
                let kind_name = stmt.enum_name(root, kind);
                self.accumulate_drop(target, stmt, &fields, kind_name, target_idx);
            }
            // Non-DDL roles are irrelevant to catalog accumulation.
            _ => {}
        }
    }

    /// The columns a CREATE TABLE or CREATE VIEW defines: its column list,
    /// or the columns of its SELECT.
    fn ddl_columns(
        &self,
        stmt: &AnyParsedStatement<'_>,
        dialect: &AnyDialect,
        reader: &StmtReader<'_, '_>,
        fields: &NodeFields,
        columns: u8,
        select: u8,
    ) -> Option<Vec<Column>> {
        reader.declared_columns(fields, columns).or_else(|| {
            let select = fields.node_id_at(select)?;
            let names = ColumnResolver::new(stmt, dialect.roles(), self).result_columns(select)?;
            Some(names.into_iter().map(Column::from).collect())
        })
    }

    /// Remove the relation a `DROP TABLE` / `DROP VIEW` names. `SQLite`
    /// rejects dropping a builtin or the wrong kind of relation, so those
    /// leave the catalog unchanged.
    fn accumulate_drop(
        &mut self,
        target: CatalogLayer,
        stmt: &AnyParsedStatement<'_>,
        fields: &NodeFields,
        kind_name: &str,
        target_idx: u8,
    ) {
        let Some(drops_view) = drop_kind_is_view(kind_name) else {
            return;
        };
        let name = alter_qualified_name(stmt, fields, target_idx);
        let key = NameKey::new(&name);
        if self.visible_relation_is_dialect_builtin(&key) {
            return;
        }
        if self
            .visible_relation(&key)
            .is_some_and(|r| r.is_view == drops_view)
        {
            self.layers[target.index()].remove_relation(&name);
            self.retired_by_statement.insert(key);
        }
    }

    /// Apply an `ALTER TABLE` role's mutation to the catalog.
    ///
    /// Copy-on-write: the currently-visible relation entry is cloned, mutated,
    /// and written into `target` so it shadows outer layers (and is retired
    /// into Connection on promotion). A no-op when the target doesn't resolve —
    /// `SQLite` rejects those, and the walker reports them as unknown tables.
    #[expect(clippy::too_many_arguments)]
    fn accumulate_alter_table(
        &mut self,
        target: CatalogLayer,
        stmt: &AnyParsedStatement<'_>,
        reader: &StmtReader<'_, '_>,
        fields: &NodeFields,
        op_name: &str,
        target_idx: u8,
        new_name: u8,
        old_name: u8,
        column: u8,
    ) {
        let table_name = alter_qualified_name(stmt, fields, target_idx);
        let table_key = NameKey::new(&table_name);
        // Dialect built-ins are immutable. Do not let an invalid ALTER mutate
        // the analyzer's built-in relation and poison later statements.
        if self.visible_relation_is_dialect_builtin(&table_key) {
            return;
        }
        let Some(mut entry) = self.visible_relation(&table_key).cloned() else {
            return;
        };
        let layer = target.index();
        match op_name {
            "ADD_COLUMN" => {
                let def = fields.node_id_at(column);
                let added = def
                    .and_then(|id| reader.column_def_name(id))
                    .or_else(|| alter_name(stmt, fields, old_name));
                if let (Some(cols), Some(col)) = (entry.columns.as_mut(), added) {
                    let kind = def.map_or(ColumnKind::Normal, |id| reader.column_def_kind(id));
                    cols.push(Column::new(col.to_ascii_lowercase(), kind));
                }
                self.layers[layer].insert_entry(&table_name, entry);
            }
            "DROP_COLUMN" => {
                if let (Some(cols), Some(col)) =
                    (entry.columns.as_mut(), alter_name(stmt, fields, old_name))
                {
                    cols.retain(|c| !c.name.eq_ignore_ascii_case(&col));
                }
                self.layers[layer].insert_entry(&table_name, entry);
            }
            "RENAME_COLUMN" => {
                if let (Some(cols), Some(from), Some(to)) = (
                    entry.columns.as_mut(),
                    alter_name(stmt, fields, old_name),
                    alter_name(stmt, fields, new_name),
                ) {
                    for c in cols.iter_mut() {
                        if c.name.eq_ignore_ascii_case(&from) {
                            c.name = to.to_ascii_lowercase();
                        }
                    }
                }
                self.layers[layer].insert_entry(&table_name, entry);
            }
            "RENAME_TABLE" => {
                let Some(new) = alter_name(stmt, fields, new_name) else {
                    return;
                };
                entry.name = new.to_string();
                self.layers[layer].insert_entry(&new, entry);
                self.layers[layer].remove_relation(&table_name);
                self.retired_by_statement.insert(table_key.clone());
            }
            // Operations a dialect adds that the catalog doesn't model.
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
            frame.insert_table(TableDef {
                name: name.to_string(),
                columns: columns.map(|cols| cols.into_iter().map(Column::from).collect()),
                without_rowid: false,
            });
        }
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

    /// Innermost-first resolution honoring tombstones: the first layer that
    /// either tombstones `key` (→ `None`) or defines it (→ its entry) wins.
    fn visible_relation(&self, key: &NameKey) -> Option<&RelationEntry> {
        for layer in self.all_layers_ordered() {
            if layer.removed_relations.contains(key) {
                return None;
            }
            if let Some(rel) = layer.relation(key) {
                return Some(rel);
            }
        }
        None
    }

    pub(crate) fn visible_relation_is_dialect_builtin(&self, key: &NameKey) -> bool {
        for (index, layer) in self.layers.iter().enumerate().rev() {
            if layer.removed_relations.contains(key) {
                return false;
            }
            if layer.relation(key).is_some() {
                return index == LAYER_DIALECT;
            }
        }
        false
    }

    /// Returns `true` if `key` is a known relation in any layer.
    pub(crate) fn resolve_relation(&self, key: &NameKey) -> bool {
        self.visible_relation(key).is_some()
    }

    /// Returns `true` if `key` is a visible relation, or one the most recent
    /// statement removed. Used by the ALTER and DROP target checks: the
    /// catalog applies a `RENAME TO` or `DROP` before the check runs, so the
    /// statement must not flag its own target. A name an earlier statement
    /// removed reads as missing.
    pub(crate) fn relation_defined(&self, key: &NameKey) -> bool {
        self.resolve_relation(key) || self.retired_by_statement.contains(key)
    }

    /// Returns `true` if `key` is a view in any layer.
    /// How many columns a relation has, of every kind. `None` when the
    /// relation or its columns are unknown.
    pub(crate) fn column_count(&self, key: &NameKey) -> Option<usize> {
        Some(self.visible_relation(key)?.columns.as_ref()?.len())
    }

    /// Values an INSERT without a column list must supply: one per normal
    /// column. `None` when the relation or its columns are unknown.
    pub(crate) fn insert_column_count(&self, key: &NameKey) -> Option<usize> {
        let columns = self.visible_relation(key)?.columns.as_ref()?;
        Some(
            columns
                .iter()
                .filter(|c| c.kind == ColumnKind::Normal)
                .count(),
        )
    }

    pub(crate) fn is_view(&self, key: &NameKey) -> bool {
        self.visible_relation(key).is_some_and(|r| r.is_view)
    }

    /// Returns `true` if `key` is a known table-valued function in any layer.
    pub(crate) fn resolve_table_function(&self, key: &NameKey) -> bool {
        self.all_layers_ordered()
            .any(|layer| layer.table_function(key).is_some())
    }

    pub(crate) fn check_function(&self, key: &NameKey, arg_count: usize) -> FunctionCheckResult {
        let set = self
            .all_layers_ordered()
            .find_map(|layer| layer.function(key));
        let Some(set) = set else {
            return FunctionCheckResult::Unknown;
        };
        if set.overloads.iter().any(|ov| ov.accepts(arg_count)) {
            return FunctionCheckResult::Ok;
        }
        FunctionCheckResult::WrongArity {
            expected: set.fixed_arities(),
        }
    }

    /// Return the column list and WITHOUT ROWID flag for a table source.
    ///
    /// Returns `(columns, without_rowid)`:
    /// - `columns = Some(cols)` — known column list.
    /// - `columns = None` — not found, or columns unknown (accept any ref).
    /// - `without_rowid = true` — no implicit rowid/oid/_rowid_ column.
    pub(crate) fn table_source_info(&self, key: &NameKey) -> (Option<Vec<String>>, bool) {
        for layer in self.all_layers_ordered() {
            if layer.removed_relations.contains(key) {
                return (None, false);
            }
            if let Some(rel) = layer.relation(key) {
                let names = rel
                    .columns
                    .as_ref()
                    .map(|cols| cols.iter().map(|c| c.name.clone()).collect());
                return (names, rel.without_rowid);
            }
            if let Some(tf) = layer.table_function(key) {
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

    /// The columns `SELECT *` expands a relation or table function to, in
    /// order: every column but the hidden ones. `None` when the relation
    /// or its columns are unknown.
    pub(crate) fn star_columns(&self, key: &NameKey) -> Option<Vec<String>> {
        for layer in self.all_layers_ordered() {
            if layer.removed_relations.contains(key) {
                return None;
            }
            if let Some(rel) = layer.relation(key) {
                let cols = rel.columns.as_ref()?;
                return Some(
                    cols.iter()
                        .filter(|c| c.kind != ColumnKind::Hidden)
                        .map(|c| c.name.clone())
                        .collect(),
                );
            }
            if let Some(tf) = layer.table_function(key) {
                return (!tf.output_columns.is_empty()).then(|| tf.output_columns.clone());
            }
        }
        None
    }

    // ── Enumeration (for fuzzy suggestions and completions) ───────────────────

    pub(crate) fn all_relation_names(&self) -> Vec<String> {
        let mut names =
            self.unique_names_across_layers(|l| l.relations.values().map(|r| r.name.as_str()));
        names.retain(|n| self.resolve_relation(&NameKey::new(n)));
        names
    }

    /// Enumerate column names across all layers, optionally filtered to a
    /// single relation. Unfiltered, dialect builtins such as `sqlite_schema`
    /// are skipped so their columns don't crowd out the user's.
    pub(crate) fn all_column_names(&self, table: Option<&str>) -> Vec<String> {
        let mut names = Vec::new();
        for (index, layer) in self.layers.iter().enumerate() {
            if table.is_none() && index == LAYER_DIALECT {
                continue;
            }
            for rel in layer.relations.values() {
                if table.is_none_or(|t| rel.name.eq_ignore_ascii_case(t))
                    && let Some(cols) = &rel.columns
                {
                    names.extend(cols.iter().map(|c| c.name.to_ascii_lowercase()));
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
        key: &NameKey,
    ) -> Option<(FunctionCategory, Vec<AritySpec>)> {
        let set = self
            .all_layers_ordered()
            .find_map(|layer| layer.function(key))?;
        let category = set
            .overloads
            .first()
            .map_or(FunctionCategory::Scalar, |ov| ov.category);
        let arities: Vec<AritySpec> = set.overloads.iter().map(|ov| ov.arity).collect();
        Some((category, arities))
    }

    pub(crate) fn all_function_names(&self) -> Vec<String> {
        self.unique_names_across_layers(|l| l.functions.values().map(|f| f.name.as_str()))
    }

    pub(crate) fn all_table_function_names(&self) -> Vec<String> {
        self.unique_names_across_layers(|l| l.table_functions.values().map(|t| t.name.as_str()))
    }

    /// Collect names from every layer using `from_layer`, lowercase-deduped
    /// and sorted by lowercase form.  Preserves the original casing of the
    /// first occurrence.
    fn unique_names_across_layers<'a, F, I>(&'a self, from_layer: F) -> Vec<String>
    where
        F: Fn(&'a CatalogLayerContents) -> I,
        I: Iterator<Item = &'a str>,
    {
        let mut seen: HashSet<String> = HashSet::new();
        let mut out: Vec<String> = Vec::new();
        for layer in self.all_layers_ordered() {
            for name in from_layer(layer) {
                if seen.insert(name.to_ascii_lowercase()) {
                    out.push(name.to_string());
                }
            }
        }
        out.sort_unstable_by_key(|n| n.to_ascii_lowercase());
        out
    }

    // ── Private helpers ───────────────────────────────────────────────────────

    /// Iterator over all layers in resolution priority order:
    /// query (innermost first) → document → connection → database → dialect.
    fn all_layers_ordered(&self) -> impl Iterator<Item = &CatalogLayerContents> {
        self.layers.iter().rev()
    }
}

// ── ALTER TABLE helpers ────────────────────────────────────────────────────────

/// For a `DROP` of a table (`Some(false)`) or view (`Some(true)`), which kind
/// it drops, from the dialect's name for the drop's object type. `None` for
/// indexes, triggers and anything else the catalog doesn't track.
pub(crate) fn drop_kind_is_view(kind_name: &str) -> Option<bool> {
    match kind_name {
        "TABLE" => Some(false),
        "VIEW" => Some(true),
        _ => None,
    }
}

/// Unqualified table name from an `ALTER` target field. The field points at a
/// `QualifiedName` node whose object name is its first child; the schema
/// qualifier (if any) is dropped, matching how catalog keys are stored.
fn alter_qualified_name(stmt: &AnyParsedStatement<'_>, fields: &NodeFields, idx: u8) -> String {
    let target_id = fields
        .node_id_at(idx)
        .expect("a schema statement has a target");
    let (_, qf) = stmt
        .extract_fields(target_id)
        .expect("the target is a qualified name");
    let obj_id = qf
        .node_id_at(0)
        .expect("a qualified name has an object name");
    stmt.name_ident_text(Some(obj_id)).0.into_owned()
}

/// Identifier value of an `ALTER` name field (a `Name` node), or `None`.
fn alter_name<'s>(
    stmt: &AnyParsedStatement<'s>,
    fields: &NodeFields,
    idx: u8,
) -> Option<Cow<'s, str>> {
    let id = fields.node_id_at(idx)?;
    let (text, _) = stmt.name_ident_text(Some(id));
    (!text.is_empty()).then_some(text)
}

// ── Dialect layer builder ─────────────────────────────────────────────────────

#[cfg(test)]
mod tests {
    use super::*;

    fn sqlite_catalog() -> Catalog {
        Catalog::new(crate::sqlite::dialect::dialect())
    }

    #[test]
    fn add_table_and_resolve() {
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Database)
            .insert_table(TableDef::new("users").columns(["id", "name"]));
        assert!(cat.resolve_relation(&NameKey::new("users")));
        assert!(cat.resolve_relation(&NameKey::new("USERS")));
        assert!(!cat.resolve_relation(&NameKey::new("orders")));
    }

    #[test]
    fn add_view_and_resolve() {
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Database)
            .insert_view(ViewDef::new("active_users").columns(["id"]));
        assert!(cat.resolve_relation(&NameKey::new("active_users")));
    }

    #[test]
    fn add_function_and_check() {
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Database)
            .insert_function_overload("my_func", FunctionCategory::Scalar, AritySpec::Exact(2));
        assert!(matches!(
            cat.check_function(&NameKey::new("my_func"), 2),
            FunctionCheckResult::Ok
        ));
        assert!(matches!(
            cat.check_function(&NameKey::new("my_func"), 1),
            FunctionCheckResult::WrongArity { .. }
        ));
    }

    #[test]
    fn add_variadic_function() {
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Database)
            .insert_function_overload("variadic_fn", FunctionCategory::Scalar, AritySpec::Any);
        assert!(matches!(
            cat.check_function(&NameKey::new("variadic_fn"), 0),
            FunctionCheckResult::Ok
        ));
        assert!(matches!(
            cat.check_function(&NameKey::new("variadic_fn"), 100),
            FunctionCheckResult::Ok
        ));
    }

    #[test]
    fn builtin_functions_resolved() {
        let cat = sqlite_catalog();
        assert!(!matches!(
            cat.check_function(&NameKey::new("abs"), 1),
            FunctionCheckResult::Unknown
        ));
        assert!(!matches!(
            cat.check_function(&NameKey::new("coalesce"), 2),
            FunctionCheckResult::Unknown
        ));
    }

    #[test]
    fn builtin_schema_tables_resolved() {
        let cat = sqlite_catalog();
        for name in [
            "sqlite_master",
            "sqlite_schema",
            "sqlite_temp_master",
            "sqlite_temp_schema",
        ] {
            assert!(cat.resolve_relation(&NameKey::new(name)), "{name}");
            assert_eq!(
                cat.table_source_info(&NameKey::new(name)).0,
                Some(vec![
                    "type".to_string(),
                    "name".to_string(),
                    "tbl_name".to_string(),
                    "rootpage".to_string(),
                    "sql".to_string(),
                ]),
                "{name}",
            );
        }
    }

    #[test]
    fn sqlite_schema_alias_respects_version() {
        let dialect = crate::Dialect::new()
            .with_version(crate::util::SqliteVersion::V3_32)
            .erase();
        let cat = Catalog::new(dialect);
        assert!(cat.resolve_relation(&NameKey::new("sqlite_master")));
        assert!(!cat.resolve_relation(&NameKey::new("sqlite_schema")));
    }

    #[test]
    fn temp_schema_aliases_respect_omit_tempdb() {
        let flags = crate::util::SqliteFlags::default().with(crate::util::SqliteFlag::OmitTempdb);
        let dialect = crate::Dialect::new().with_cflags(flags).erase();
        let cat = Catalog::new(dialect);
        assert!(cat.resolve_relation(&NameKey::new("sqlite_schema")));
        assert!(!cat.resolve_relation(&NameKey::new("sqlite_temp_schema")));
        assert!(!cat.resolve_relation(&NameKey::new("sqlite_temp_master")));
    }

    #[test]
    fn invalid_alter_does_not_mutate_schema_table() {
        let dialect = crate::sqlite::dialect::any_dialect();
        let mut cat = Catalog::new(dialect.clone());
        accumulate_into(
            &mut cat,
            &dialect,
            CatalogLayer::Document,
            "ALTER TABLE sqlite_schema DROP COLUMN sql;",
        );
        assert!(
            cat.table_source_info(&NameKey::new("sqlite_schema"))
                .0
                .is_some_and(|columns| columns.iter().any(|column| column == "sql"))
        );
    }

    #[test]
    fn from_ddl_populates_tables() {
        let dialect = crate::sqlite::dialect::dialect();
        let cat = Catalog::from_ddl(dialect, &["CREATE TABLE users (id INTEGER, name TEXT);"]).0;
        assert!(cat.resolve_relation(&NameKey::new("users")));
    }

    #[test]
    fn from_ddl_populates_virtual_tables() {
        let dialect = crate::sqlite::dialect::dialect();
        let cat = Catalog::from_ddl(dialect, &["CREATE VIRTUAL TABLE fts USING fts5(content);"]).0;
        assert!(cat.resolve_relation(&NameKey::new("fts")));
    }

    /// Parse `sql` and accumulate its DDL into `layer` of `cat`.
    fn accumulate_into(cat: &mut Catalog, dialect: &AnyDialect, layer: CatalogLayer, sql: &str) {
        use syntaqlite_syntax::ParseOutcome;
        let parser = syntaqlite_syntax::Parser::new();
        let mut session = parser.parse(sql);
        loop {
            match session.next() {
                ParseOutcome::Ok(stmt) => {
                    let Some(root) = stmt.root() else { continue };
                    let root_id: AnyNodeId = root.node_id().into();
                    let erased = stmt.erase();
                    cat.accumulate_ddl(layer, &erased, root_id, dialect);
                }
                ParseOutcome::Done => break,
                ParseOutcome::Err(_) => {}
            }
        }
    }

    /// The columns `cat` resolves `table` to, as `(name, kind)` pairs.
    fn column_kinds(cat: &Catalog, table: &str) -> Vec<(String, ColumnKind)> {
        let rel = cat
            .visible_relation(&NameKey::new(table))
            .expect("table exists");
        let cols = rel.columns.as_ref().expect("columns are known");
        cols.iter().map(|c| (c.name.clone(), c.kind)).collect()
    }

    #[test]
    fn ddl_records_generated_columns() {
        let dialect = crate::sqlite::dialect::dialect();
        let cat = Catalog::from_ddl(
            dialect,
            &["CREATE TABLE t(a, b AS (a * 2), c GENERATED ALWAYS AS (a) STORED);"],
        )
        .0;
        assert_eq!(
            column_kinds(&cat, "t"),
            [
                ("a".into(), ColumnKind::Normal),
                ("b".into(), ColumnKind::Generated),
                ("c".into(), ColumnKind::Generated),
            ]
        );
    }

    #[test]
    fn alter_keeps_column_kinds() {
        let dialect = crate::sqlite::dialect::dialect();
        let cat = Catalog::from_ddl(
            dialect,
            &["CREATE TABLE t(a, b AS (a)); \
               ALTER TABLE t ADD COLUMN c AS (a); \
               ALTER TABLE t RENAME COLUMN b TO bb; \
               ALTER TABLE t ADD COLUMN d; \
               ALTER TABLE t DROP COLUMN a;"],
        )
        .0;
        assert_eq!(
            column_kinds(&cat, "t"),
            [
                ("bb".into(), ColumnKind::Generated),
                ("c".into(), ColumnKind::Generated),
                ("d".into(), ColumnKind::Normal),
            ]
        );
    }

    #[test]
    fn json_columns_carry_kinds() {
        let dialect = crate::sqlite::dialect::dialect();
        let cat = Catalog::from_json(
            dialect,
            r#"{"tables": [{"name": "ft", "columns": [
                "body",
                {"name": "len", "kind": "generated"},
                {"name": "rank", "kind": "hidden"},
                {"name": "id"}
            ]}]}"#,
        )
        .unwrap();
        assert_eq!(
            column_kinds(&cat, "ft"),
            [
                ("body".into(), ColumnKind::Normal),
                ("len".into(), ColumnKind::Generated),
                ("rank".into(), ColumnKind::Hidden),
                ("id".into(), ColumnKind::Normal),
            ]
        );
    }

    #[test]
    fn json_rejects_malformed_columns() {
        let dialect = crate::sqlite::dialect::dialect();
        for column in [
            r#"{"name": "a", "generated": true}"#,
            r#"{"name": "a", "kind": "virtual"}"#,
        ] {
            let json = format!(r#"{{"tables": [{{"name": "t", "columns": [{column}]}}]}}"#);
            assert!(
                Catalog::from_json(dialect.clone(), &json).is_err(),
                "{column} should be rejected"
            );
        }
    }

    #[test]
    fn json_omitted_columns_are_unknown_and_empty_ones_known() {
        let dialect = crate::sqlite::dialect::dialect();
        let cat = Catalog::from_json(
            dialect,
            r#"{"tables": [{"name": "a"}, {"name": "b", "columns": null}, {"name": "c", "columns": []}],
                "views": [{"name": "v", "columns": [{"name": "x", "kind": "normal"}]}]}"#,
        )
        .unwrap();
        for unknown in ["a", "b"] {
            assert!(
                cat.visible_relation(&NameKey::new(unknown))
                    .unwrap()
                    .columns
                    .is_none()
            );
        }
        assert_eq!(column_kinds(&cat, "c"), []);
        assert_eq!(column_kinds(&cat, "v"), [("x".into(), ColumnKind::Normal)]);
    }

    #[test]
    fn alter_add_column_registers_column() {
        let dialect = crate::sqlite::dialect::dialect();
        let cat = Catalog::from_ddl(
            dialect,
            &["CREATE TABLE x1(a); ALTER TABLE x1 ADD COLUMN b DEFAULT 5;"],
        )
        .0;
        assert_eq!(
            cat.table_source_info(&NameKey::new("x1")).0,
            Some(vec!["a".to_string(), "b".to_string()]),
        );
    }

    #[test]
    fn alter_add_column_with_full_definition_registers_column() {
        let dialect = crate::sqlite::dialect::dialect();
        let cat = Catalog::from_ddl(
            dialect,
            &["CREATE TABLE x1(a); \
                 ALTER TABLE x1 ADD COLUMN b TEXT NOT NULL DEFAULT '' COLLATE NOCASE;"],
        )
        .0;
        assert_eq!(
            cat.table_source_info(&NameKey::new("x1")).0,
            Some(vec!["a".to_string(), "b".to_string()]),
        );
    }

    #[test]
    fn alter_add_column_none_columns_stays_none() {
        // A table registered with unknown columns must stay column-agnostic
        // after ADD COLUMN — no phantom Some(vec![...]) that would start
        // rejecting every other column reference.
        let dialect = crate::sqlite::dialect::dialect();
        let mut cat = Catalog::new(dialect.clone());
        cat.layer_mut(CatalogLayer::Database)
            .insert_table(TableDef::new("x1"));
        accumulate_into(
            &mut cat,
            &dialect,
            CatalogLayer::Database,
            "ALTER TABLE x1 ADD COLUMN b;",
        );
        assert_eq!(cat.table_source_info(&NameKey::new("x1")).0, None);
    }

    #[test]
    fn alter_drop_and_rename_column() {
        let dialect = crate::sqlite::dialect::dialect();
        let cat = Catalog::from_ddl(
            dialect,
            &["CREATE TABLE t(a, b, c); \
                 ALTER TABLE t DROP COLUMN b; \
                 ALTER TABLE t RENAME COLUMN c TO d;"],
        )
        .0;
        assert_eq!(
            cat.table_source_info(&NameKey::new("t")).0,
            Some(vec!["a".to_string(), "d".to_string()]),
        );
    }

    #[test]
    fn alter_of_unknown_table_is_noop() {
        let dialect = crate::sqlite::dialect::dialect();
        let cat = Catalog::from_ddl(dialect, &["ALTER TABLE nosuch DROP COLUMN z;"]).0;
        assert!(!cat.resolve_relation(&NameKey::new("nosuch")));
    }

    #[test]
    fn alter_rename_table_masks_old_and_registers_new_across_promotion() {
        let dialect = crate::sqlite::dialect::dialect();
        let mut cat = Catalog::new(dialect.clone());
        // Simulate a table established by an earlier (already promoted) pass.
        cat.layer_mut(CatalogLayer::Connection)
            .insert_table(TableDef::new("t3").columns(["a"]));
        accumulate_into(
            &mut cat,
            &dialect,
            CatalogLayer::Document,
            "ALTER TABLE t3 RENAME TO t5;",
        );

        // The new name resolves; the old name is masked for FROM-source use...
        assert!(cat.resolve_relation(&NameKey::new("t5")));
        assert!(!cat.resolve_relation(&NameKey::new("t3")));
        // ...but the rename's own existence check still sees the target
        // (tombstone-ignoring), so RENAME TO doesn't flag its own source.
        assert!(cat.relation_defined(&NameKey::new("t3")));
        assert_eq!(
            cat.table_source_info(&NameKey::new("t5")).0,
            Some(vec!["a".to_string()])
        );

        cat.promote_document_to_connection();
        // Next analysis pass starts fresh — the Document tombstone is cleared,
        // but Connection has already retired the old name for good.
        cat.new_document();
        assert!(cat.resolve_relation(&NameKey::new("t5")));
        assert!(!cat.resolve_relation(&NameKey::new("t3")));
        assert!(!cat.relation_defined(&NameKey::new("t3")));
    }

    #[test]
    fn recreating_a_renamed_table_clears_the_tombstone() {
        let dialect = crate::sqlite::dialect::dialect();
        let cat = Catalog::from_ddl(
            dialect,
            &["CREATE TABLE t3(a); ALTER TABLE t3 RENAME TO t5; CREATE TABLE t3(b);"],
        )
        .0;
        assert!(cat.resolve_relation(&NameKey::new("t3")));
        assert!(cat.resolve_relation(&NameKey::new("t5")));
        assert_eq!(
            cat.table_source_info(&NameKey::new("t3")).0,
            Some(vec!["b".to_string()])
        );
    }

    #[test]
    fn clear_database() {
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Database)
            .insert_table(TableDef::new("tmp").columns(["id"]));
        assert!(cat.resolve_relation(&NameKey::new("tmp")));
        cat.new_database();
        assert!(!cat.resolve_relation(&NameKey::new("tmp")));
    }

    #[test]
    fn clear_connection() {
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Connection)
            .insert_table(TableDef::new("conn_tbl").columns(["id"]));
        cat.new_connection();
        assert!(!cat.resolve_relation(&NameKey::new("conn_tbl")));
    }

    #[test]
    fn connection_layer_resolves() {
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Connection)
            .insert_table(TableDef::new("conn_tbl").columns(["id"]));
        assert!(cat.resolve_relation(&NameKey::new("conn_tbl")));
    }

    #[test]
    fn is_view_matches_inserted_views_only() {
        let mut cat = sqlite_catalog();
        cat.layer_mut(CatalogLayer::Database)
            .insert_table(TableDef::new("users").columns(["id"]));
        cat.layer_mut(CatalogLayer::Database)
            .insert_view(ViewDef::new("active_users").columns(["id"]));

        assert!(!cat.is_view(&NameKey::new("users")));
        assert!(cat.is_view(&NameKey::new("active_users")));
        assert!(!cat.is_view(&NameKey::new("nonexistent")));
    }

    #[test]
    fn relation_names_enumerates_tables_and_views() {
        let mut catalog = sqlite_catalog();
        catalog
            .layer_mut(CatalogLayer::Database)
            .insert_table(TableDef::new("t1").columns(["a"]));
        catalog
            .layer_mut(CatalogLayer::Database)
            .insert_view(ViewDef::new("v1").columns(["b"]));

        let names: HashSet<&str> = catalog
            .layer(CatalogLayer::Database)
            .relation_names()
            .collect();
        assert!(names.contains("t1"));
        assert!(names.contains("v1"));
    }
}
