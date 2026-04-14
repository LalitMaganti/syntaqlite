// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// ── Public API ───────────────────────────────────────────────────────────────

use crate::any::{AnyNodeTag, AnyTokenType};
use crate::util::{SqliteSyntaxFlags, SqliteVersion};

/// Runtime field-value shape used when reflecting over AST nodes.
///
/// This powers dialect-agnostic tooling that inspects nodes without generated
/// Rust types.
#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FieldKind {
    /// A child node identifier.
    NodeId = 0,
    /// A source span (byte offset + length).
    Span = 1,
    /// A boolean flag.
    Bool = 2,
    /// A compact bitfield of flags.
    Flags = 3,
    /// A discriminant for an enum variant.
    Enum = 4,
}

impl FieldKind {
    fn from_u8(v: u8) -> Self {
        match v {
            1 => FieldKind::Span,
            2 => FieldKind::Bool,
            3 => FieldKind::Flags,
            4 => FieldKind::Enum,
            _ => FieldKind::NodeId,
        }
    }
}

/// High-level semantic class of a token.
///
/// Commonly used for syntax highlighting, token styling, and lightweight
/// heuristics before full semantic analysis.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TokenCategory {
    /// SQL keyword (SELECT, FROM, WHERE, …)
    Keyword,
    /// Bind parameter or session variable (`:name`, `@var`, `?`)
    Parameter,
    /// String literal or blob literal
    String,
    /// Numeric literal
    Number,
    /// Operator or comparison symbol (`+`, `=`, `||`, …)
    Operator,
    /// Comment (`-- …` or `/* … */`)
    Comment,
    /// Punctuation (`,`, `(`, `)`, `;`, …)
    Punctuation,
    /// Quoted or unquoted identifier
    Identifier,
    /// Built-in or user-defined function name
    Function,
    /// Type name (in CAST, column definitions, …)
    Type,
    /// Anything that doesn't fall into the above categories
    Other,
}

impl From<ffi::CTokenCategory> for TokenCategory {
    fn from(c: ffi::CTokenCategory) -> Self {
        match c {
            ffi::CTokenCategory::Keyword => Self::Keyword,
            ffi::CTokenCategory::Identifier => Self::Identifier,
            ffi::CTokenCategory::String => Self::String,
            ffi::CTokenCategory::Number => Self::Number,
            ffi::CTokenCategory::Operator => Self::Operator,
            ffi::CTokenCategory::Punctuation => Self::Punctuation,
            ffi::CTokenCategory::Comment => Self::Comment,
            ffi::CTokenCategory::Variable => Self::Parameter,
            ffi::CTokenCategory::Function => Self::Function,
            ffi::CTokenCategory::Type => Self::Type,
            ffi::CTokenCategory::Other => Self::Other,
        }
    }
}

/// Metadata for one AST field of one node type.
///
/// Use this to build generic inspectors, serializers, or debug UIs that can
/// walk arbitrary dialects.
pub struct FieldMeta<'a>(pub(crate) &'a ffi::CFieldMeta);

impl FieldMeta<'_> {
    /// Byte offset of this field within its parent AST node struct.
    pub fn offset(&self) -> u16 {
        self.0.offset
    }

    /// Semantic kind of this field.
    pub fn kind(&self) -> FieldKind {
        FieldKind::from_u8(self.0.kind)
    }

    /// The field name as a `&str`.
    ///
    /// # Panics
    /// Panics if the dialect table contains invalid UTF-8 in the field name
    /// (which would indicate a codegen bug).
    pub fn name(&self) -> &'static str {
        // SAFETY: `FieldMeta` is only constructed from static dialect tables
        // where `name` is always a valid, NUL-terminated UTF-8 C string.
        unsafe {
            let cstr = std::ffi::CStr::from_ptr(self.0.name);
            cstr.to_str().expect("invalid UTF-8 in field name")
        }
    }

    /// The `idx`-th display name for enum variants, if present.
    ///
    /// # Panics
    /// Panics if the dialect table contains invalid UTF-8 in a display name
    /// (which would indicate a codegen bug).
    pub fn display_name(&self, idx: usize) -> Option<&'static str> {
        if self.0.display.is_null() || idx >= self.0.display_count as usize {
            return None;
        }
        // SAFETY: `FieldMeta` is only constructed from static dialect tables;
        // `display` and its entries are valid static C strings.
        unsafe {
            let ptr = *self.0.display.add(idx);
            if ptr.is_null() {
                return None;
            }
            let cstr = std::ffi::CStr::from_ptr(ptr);
            Some(cstr.to_str().expect("invalid UTF-8 in display name"))
        }
    }

    /// Number of display names for this field.
    pub fn display_count(&self) -> usize {
        self.0.display_count as usize
    }
}

/// Parser-inferred semantic usage for an individual token occurrence.
///
/// This complements lexical token kind and helps distinguish ambiguous tokens
/// (for example keyword text used as an identifier).
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct ParserTokenFlags(u8);

impl ParserTokenFlags {
    /// Construct from a raw C flag bitfield (`SyntaqliteParserTokenFlags = uint32_t`).
    pub(crate) fn from_raw(v: u32) -> Self {
        let bits = u8::try_from(v).expect("parser token flags out of range for u8");
        ParserTokenFlags(bits)
    }

    // Bit positions — mirror C SYNQ_TOKEN_FLAG_* in syntaqlite/parser.h.
    const AS_ID: u8 = 1;
    const AS_FUNCTION: u8 = 2;
    const AS_TYPE: u8 = 4;

    /// Returns the underlying flag bits.
    pub fn bits(self) -> u8 {
        self.0
    }

    /// True if the token was used as an identifier (`SYNQ_TOKEN_FLAG_AS_ID`).
    pub fn used_as_identifier(self) -> bool {
        self.0 & Self::AS_ID != 0
    }

    /// True if the token was used as a function name (`SYNQ_TOKEN_FLAG_AS_FUNCTION`).
    pub fn used_as_function(self) -> bool {
        self.0 & Self::AS_FUNCTION != 0
    }

    /// True if the token was used as a type name (`SYNQ_TOKEN_FLAG_AS_TYPE`).
    pub fn used_as_type(self) -> bool {
        self.0 & Self::AS_TYPE != 0
    }
}

/// Trait implemented by generated dialect handles.
///
/// End users typically consume implementations rather than writing them.
/// The trait links a dialect to its typed node and token enums.
pub trait TypedDialect: Clone + Into<AnyDialect> {
    /// The top-level typed AST node enum for this dialect.
    type Node<'a>: crate::ast::GrammarNodeType<'a>;
    /// The dialect's typed node ID, wrapping an [`crate::ast::AnyNodeId`].
    ///
    /// Used as the return type of [`TypedNodeList::node_id`](crate::ast::TypedNodeList::node_id)
    /// so callers get a dialect-typed handle rather than a raw [`crate::ast::AnyNodeId`].
    type NodeId: Copy + From<crate::ast::AnyNodeId> + Into<crate::ast::AnyNodeId>;
    /// The typed token enum for this dialect.
    type Token: crate::ast::GrammarTokenType;
}

/// Dialect handle for runtime-configurable, dialect-agnostic workflows.
///
/// Use `AnyDialect` when dialect selection/configuration is dynamic (plugins,
/// LSP hosts, multi-dialect test harnesses). It carries version/cflag knobs
/// and introspection metadata, while remaining cheap to clone.
///
/// Built-in dialects hold `&'static` C data directly. Dynamically loaded
/// dialects transmute library-memory pointers to `&'static` and keep the
/// library alive via an [`Arc`](std::sync::Arc). Use `AnyDialect::load` to create one.
#[derive(Clone)]
pub struct AnyDialect {
    pub(crate) inner: ffi::CDialect,
    /// Keeps the shared library alive for dynamically-loaded dialects.
    /// `None` for built-in (static) dialects.
    _keep_alive: Option<std::sync::Arc<dyn Send + Sync>>,
}

// SAFETY: The dialect wraps an immutable reference to static C data.
unsafe impl Send for AnyDialect {}
// SAFETY: AnyDialect wraps a *const CDialect to a static C dialect object; it is safe to share across threads.
unsafe impl Sync for AnyDialect {}

impl AnyDialect {
    /// Construct an `AnyDialect` from a raw C dialect value.
    ///
    /// This unsafe method exists only for use by dialect implementations which are code generated.
    /// End users should never need to call this directly.
    ///
    /// # Safety
    /// The `template` pointer inside `inner` must point to valid, `'static`
    /// C dialect tables (e.g. returned by a dialect's `extern "C"` accessor
    /// such as `syntaqlite_sqlite_dialect()`).
    pub unsafe fn new(inner: ffi::CDialect) -> Self {
        AnyDialect {
            inner,
            _keep_alive: None,
        }
    }

    /// Construct an `AnyDialect` from a dialect template pointer.
    ///
    /// Uses `INT32_MAX` as the version (latest) and empty cflags.
    ///
    /// # Safety
    /// `template` must point to a valid, `'static` `SyntaqliteDialectTemplate`.
    pub unsafe fn from_dialect_template(template: *const ffi::CDialectTemplate) -> Self {
        let inner = ffi::CDialect {
            template,
            sqlite_version: i32::MAX,
            cflags: crate::util::ffi::CCflags::default(),
        };
        AnyDialect {
            inner,
            _keep_alive: None,
        }
    }

    /// Load a dialect from a shared library (`.so` / `.dylib` / `.dll`).
    ///
    /// Resolves `syntaqlite_<name>_dialect` (or `syntaqlite_dialect` when `name`
    /// is `None`) and calls it to obtain the dialect handle.
    ///
    /// # Errors
    /// Returns `Err` if the library cannot be opened or the dialect symbol is absent.
    ///
    /// # Library lifetime
    /// The loaded library is kept alive via an [`Arc`] stored inside the
    /// returned `AnyDialect`. Dropping the last clone of the dialect unloads
    /// the library. Use [`syntaqlite::Dialect::load`] for dialect-level loading.
    #[cfg(feature = "dynload")]
    pub fn load(path: &str, name: Option<&str>) -> Result<Self, String> {
        // SAFETY: We keep `lib` alive in an `Arc` below so the dialect pointer
        // lives as long as any clone of the returned AnyDialect.
        let lib = unsafe {
            libloading::Library::new(path).map_err(|e| format!("failed to load {path:?}: {e}"))?
        };

        let symbol = match name {
            Some(n) => format!("syntaqlite_{n}_dialect"),
            None => "syntaqlite_dialect".to_string(),
        };
        // SAFETY: We call the function immediately and drop `func` before `lib`
        // is moved into the Arc, so there is no lifetime overlap issue.
        let raw: ffi::CDialect = unsafe {
            let func: libloading::Symbol<'_, unsafe extern "C" fn() -> ffi::CDialect> = lib
                .get(symbol.as_bytes())
                .map_err(|e| format!("symbol {symbol:?} not found in {path:?}: {e}"))?;
            func()
        };

        let keep_alive: std::sync::Arc<dyn Send + Sync> = std::sync::Arc::new(lib);

        // SAFETY: `raw.template` points into the shared library kept alive by
        // `keep_alive`. Dropping the last AnyDialect clone unloads the library.
        Ok(AnyDialect {
            inner: raw,
            _keep_alive: Some(keep_alive),
        })
    }

    /// Pin this dialect handle to a target `SQLite` version.
    ///
    /// Useful when your product must emulate a specific engine release.
    #[must_use]
    pub fn with_version(mut self, version: SqliteVersion) -> Self {
        self.inner.sqlite_version = version.as_int();
        self
    }

    /// Replace compile-time compatibility flags on this handle.
    #[must_use]
    pub fn with_cflags(mut self, flags: SqliteSyntaxFlags) -> Self {
        self.inner.cflags = flags.0;
        self
    }

    /// Target `SQLite` version currently configured on this handle.
    pub fn version(&self) -> SqliteVersion {
        SqliteVersion::from_int(self.inner.sqlite_version)
    }

    /// Active C-parser compile-time compatibility flags.
    pub fn cflags(&self) -> SqliteSyntaxFlags {
        SqliteSyntaxFlags(self.inner.cflags)
    }

    /// Whether this dialect supports Rust-style macro invocations (`name!(args)`).
    pub fn has_macro_style(&self) -> bool {
        self.template().macro_style != 0
    }

    /// Return a reference to the underlying C dialect template.
    #[inline]
    fn template(&self) -> &'static ffi::CDialectTemplate {
        // SAFETY: `inner.template` points to a valid `SyntaqliteDialectTemplate`.
        unsafe { &*self.inner.template }
    }

    /// Return the human-readable node name for `tag`.
    ///
    /// # Panics
    /// Panics if `tag` is out of bounds for this dialect.
    pub fn node_name(&self, tag: AnyNodeTag) -> &'static str {
        let raw = self.template();
        let idx = tag.0 as usize;
        assert!(
            idx < raw.node_count as usize,
            "node tag {} out of bounds (count={})",
            idx,
            raw.node_count,
        );
        // SAFETY: idx is bounds-checked above; node_names is a static array of
        // length node_count populated by codegen, with valid NUL-terminated strings.
        unsafe {
            let cstr = std::ffi::CStr::from_ptr(*raw.node_names.add(idx));
            cstr.to_str().expect("invalid UTF-8 in node name")
        }
    }

    /// Whether `tag` identifies a list node shape.
    pub fn is_list(&self, tag: AnyNodeTag) -> bool {
        let raw = self.template();
        let idx = tag.0 as usize;
        if idx >= raw.node_count as usize {
            return false;
        }
        // SAFETY: idx is bounds-checked above; list_tags is a static array of
        // length node_count populated by codegen.
        unsafe { *raw.list_tags.add(idx) != 0 }
    }

    /// Return field metadata for nodes with tag `tag`.
    pub fn field_meta(&self, tag: AnyNodeTag) -> impl ExactSizeIterator<Item = FieldMeta<'static>> {
        let raw = self.template();
        let idx = tag.0 as usize;
        // SAFETY: idx is bounds-checked; field_meta_counts and field_meta are
        // parallel static arrays of length node_count populated by codegen.
        let slice: &'static [ffi::CFieldMeta] = unsafe {
            if idx >= raw.node_count as usize {
                &[]
            } else {
                let count = *raw.field_meta_counts.add(idx) as usize;
                let ptr = (*raw.field_meta.add(idx)).cast::<ffi::CFieldMeta>();
                if count == 0 || ptr.is_null() {
                    &[]
                } else {
                    std::slice::from_raw_parts(ptr, count)
                }
            }
        };
        slice.iter().map(FieldMeta)
    }

    /// Classify a token for presentation/analysis using parser context when available.
    pub fn classify_token(
        &self,
        token_type: AnyTokenType,
        flags: ParserTokenFlags,
    ) -> TokenCategory {
        if flags.used_as_function() {
            TokenCategory::Function
        } else if flags.used_as_type() {
            TokenCategory::Type
        } else if flags.used_as_identifier() {
            TokenCategory::Identifier
        } else {
            self.token_category(token_type)
        }
    }

    /// Return the default semantic category for a token type ordinal.
    pub fn token_category(&self, token_type: AnyTokenType) -> TokenCategory {
        let raw = self.template();
        let idx = token_type.0 as usize;
        if raw.token_categories.is_null() || idx >= raw.token_type_count as usize {
            return TokenCategory::Other;
        }
        // SAFETY: token_categories is null-checked; it is a static array of
        // length token_type_count populated by codegen.
        let byte = unsafe { *raw.token_categories.add(idx) };
        TokenCategory::from(ffi::CTokenCategory::from_u8(byte))
    }

    /// Iterate all keywords known to this dialect.
    ///
    /// Yields a [`KeywordEntry`] for each keyword, containing the token type
    /// ordinal and the keyword lexeme (e.g. `SELECT`, `WHERE`).
    ///
    /// The iterator implements [`ExactSizeIterator`], so `.len()` gives the
    /// total keyword count without consuming the iterator.
    pub fn keywords(&self) -> impl ExactSizeIterator<Item = KeywordEntry> + '_ {
        let raw = self.template();
        let count = if raw.keyword_text.is_null()
            || raw.keyword_offsets.is_null()
            || raw.keyword_lens.is_null()
            || raw.keyword_codes.is_null()
            || raw.keyword_count.is_null()
        {
            0
        } else {
            // SAFETY: keyword_count is null-checked above; points to a static u32.
            unsafe { *raw.keyword_count as usize }
        };
        KeywordIter {
            dialect: self,
            idx: 0,
            count,
        }
    }
}

impl TypedDialect for AnyDialect {
    type Node<'a> = crate::ast::AnyNode<'a>;
    type NodeId = crate::ast::AnyNodeId;
    type Token = AnyTokenType;
}

/// One dialect keyword entry.
///
/// Yielded by [`AnyDialect::keywords`] for completions, lexers, and tooling.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct KeywordEntry {
    /// The token type for this keyword.
    token_type: AnyTokenType,
    /// The keyword lexeme (e.g. `"SELECT"`, `"WHERE"`).
    keyword: &'static str,
}

impl KeywordEntry {
    /// The token type for this keyword.
    pub fn token_type(&self) -> AnyTokenType {
        self.token_type
    }
    /// The keyword lexeme (e.g. `"SELECT"`, `"WHERE"`).
    pub fn keyword(&self) -> &'static str {
        self.keyword
    }
}

struct KeywordIter<'a> {
    dialect: &'a AnyDialect,
    idx: usize,
    count: usize,
}

impl Iterator for KeywordIter<'_> {
    type Item = KeywordEntry;

    fn next(&mut self) -> Option<KeywordEntry> {
        if self.idx >= self.count {
            return None;
        }
        let raw = self.dialect.template();
        // SAFETY: all keyword pointers were null-checked in `keywords()`; arrays
        // are static, length = self.count, and self.idx < self.count.
        let entry = unsafe {
            let code = u32::from(*raw.keyword_codes.add(self.idx));
            let len = *raw.keyword_lens.add(self.idx) as usize;
            let off = *raw.keyword_offsets.add(self.idx) as usize;
            let bytes = std::slice::from_raw_parts(raw.keyword_text.cast::<u8>().add(off), len);
            KeywordEntry {
                token_type: AnyTokenType(code),
                keyword: std::str::from_utf8_unchecked(bytes),
            }
        };
        self.idx += 1;
        Some(entry)
    }

    fn size_hint(&self) -> (usize, Option<usize>) {
        let remaining = self.count - self.idx;
        (remaining, Some(remaining))
    }
}

impl ExactSizeIterator for KeywordIter<'_> {}

// ── ffi ───────────────────────────────────────────────────────────────────────

pub(crate) mod ffi {
    use crate::util::ffi::CCflags;

    /// Mirrors C `SynqTokenCategory` enum defined in
    /// `include/syntaqlite/dialect.h`.
    #[repr(u8)]
    #[derive(Debug, Clone, Copy, PartialEq, Eq)]
    pub(crate) enum CTokenCategory {
        Other = 0,
        Keyword = 1,
        Identifier = 2,
        String = 3,
        Number = 4,
        Operator = 5,
        Punctuation = 6,
        Comment = 7,
        Variable = 8,
        Function = 9,
        Type = 10,
    }

    impl CTokenCategory {
        /// Convert a raw byte from the dialect table to a `CTokenCategory`.
        /// Unknown values map to `Other`.
        pub(crate) fn from_u8(v: u8) -> Self {
            match v {
                1 => Self::Keyword,
                2 => Self::Identifier,
                3 => Self::String,
                4 => Self::Number,
                5 => Self::Operator,
                6 => Self::Punctuation,
                7 => Self::Comment,
                8 => Self::Variable,
                9 => Self::Function,
                10 => Self::Type,
                _ => Self::Other,
            }
        }
    }

    /// Mirrors the C `SyntaqliteDialectTemplate` struct defined in
    /// `include/syntaqlite/dialect.h`.
    ///
    /// All fields are always present; optional sections (fmt, validation)
    /// are zeroed when the feature is not compiled in.
    #[repr(C)]
    pub struct CDialectTemplate {
        #[doc(hidden)]
        pub name: *const std::ffi::c_char,

        // Range metadata
        #[doc(hidden)]
        pub range_meta: *const std::ffi::c_void,

        // AST metadata
        #[doc(hidden)]
        pub node_count: u32,
        #[doc(hidden)]
        pub node_names: *const *const std::ffi::c_char,
        #[doc(hidden)]
        pub field_meta: *const *const std::ffi::c_void,
        #[doc(hidden)]
        pub field_meta_counts: *const u8,
        #[doc(hidden)]
        pub list_tags: *const u8,

        // Parser lifecycle (function pointers provided by the dialect's parser)
        #[doc(hidden)]
        pub parser_alloc: *const std::ffi::c_void,
        #[doc(hidden)]
        pub parser_init: *const std::ffi::c_void,
        #[doc(hidden)]
        pub parser_finalize: *const std::ffi::c_void,
        #[doc(hidden)]
        pub parser_free: *const std::ffi::c_void,
        #[doc(hidden)]
        pub parser_feed: *const std::ffi::c_void,
        #[doc(hidden)]
        pub parser_trace: *const std::ffi::c_void,
        #[doc(hidden)]
        pub parser_expected_tokens: *const std::ffi::c_void,
        #[doc(hidden)]
        pub parser_completion_context: *const std::ffi::c_void,

        // Lemon %fallback lookup
        #[doc(hidden)]
        pub parser_fallback: *const std::ffi::c_void,

        // Tokenizer (function pointer provided by the dialect's parser)
        #[doc(hidden)]
        pub get_token: *const std::ffi::c_void,

        // Keyword table metadata
        #[doc(hidden)]
        pub keyword_text: *const std::ffi::c_char,
        #[doc(hidden)]
        pub keyword_offsets: *const u16,
        #[doc(hidden)]
        pub keyword_lens: *const u8,
        #[doc(hidden)]
        pub keyword_codes: *const u8,
        #[doc(hidden)]
        pub keyword_count: *const u32,

        // Token metadata (indexed by token type ordinal)
        #[doc(hidden)]
        pub token_categories: *const u8,
        #[doc(hidden)]
        pub token_type_count: u32,

        // Macro invocation style
        #[doc(hidden)]
        pub macro_style: u32,

        // ── fmt fields ──
        #[doc(hidden)]
        pub fmt_str_data: *const u8,
        #[doc(hidden)]
        pub fmt_str_offsets: *const u32,
        #[doc(hidden)]
        pub fmt_str_count: u32,
        #[doc(hidden)]
        pub fmt_enum_display: *const u16,
        #[doc(hidden)]
        pub fmt_enum_display_count: u32,
        #[doc(hidden)]
        pub fmt_ops: *const u8,
        #[doc(hidden)]
        pub fmt_ops_count: u32,
        #[doc(hidden)]
        pub fmt_dispatch: *const u32,
        #[doc(hidden)]
        pub fmt_dispatch_count: u32,
        #[doc(hidden)]
        pub fmt_prec_table: *const u8,
        #[doc(hidden)]
        pub fmt_prec_table_count: u32,
        #[doc(hidden)]
        pub fmt_expr_meta: *const u32,
        #[doc(hidden)]
        pub fmt_expr_meta_count: u32,

        // ── validation fields ──
        #[doc(hidden)]
        pub roles_data: *const u8,
        #[doc(hidden)]
        pub roles_count: u32,
        #[doc(hidden)]
        pub macro_defs_data: *const u8,
        #[doc(hidden)]
        pub macro_defs_count: u32,
    }

    // SAFETY: CDialectTemplate contains only pointers to immutable static C data.
    unsafe impl Send for CDialectTemplate {}
    // SAFETY: CDialectTemplate contains only pointers to immutable static C data.
    unsafe impl Sync for CDialectTemplate {}

    /// Mirrors C `SyntaqliteDialect` from `include/syntaqlite/dialect.h`.
    #[repr(C)]
    #[derive(Debug, Clone, Copy)]
    pub struct CDialect {
        #[doc(hidden)]
        pub template: *const CDialectTemplate,
        #[doc(hidden)]
        pub sqlite_version: i32,
        #[doc(hidden)]
        pub cflags: CCflags,
    }

    /// Mirrors C `SyntaqliteFieldMeta` from `include/syntaqlite_dialect/dialect_types.h`.
    #[repr(C)]
    pub(crate) struct CFieldMeta {
        pub(crate) offset: u16,
        pub(crate) kind: u8,
        pub(crate) name: *const std::ffi::c_char,
        pub(crate) display: *const *const std::ffi::c_char,
        pub(crate) display_count: u8,
    }
}
