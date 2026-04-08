// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use crate::util::rust_writer::RustWriter;

// ── External dialect lib.rs ───────────────────────────────────────────────

const LIB_MODULE_DECLS: &str = r"
mod ffi;
/// Typed AST nodes for this dialect.
///
/// Each SQL statement type (e.g. `SELECT`, `INSERT`) has a corresponding struct
/// with typed accessors for its fields. The top-level enum is [`ast::Stmt`],
/// returned by [`StatementCursor::next_statement`].
pub mod ast;
pub mod tokens;
";

fn emit_section(w: &mut RustWriter, section: &str) {
    w.lines(section);
    w.newline();
}

/// Generate a self-contained grammar accessor module.
///
/// Used both for external dialect crates (as part of `lib.rs`) and for the
/// internal `SQLite` dialect (as `sqlite/grammar.rs`).
///
/// - `dialect_fn`: the `extern "C"` symbol name, e.g. `syntaqlite_sqlite_grammar`
/// - `grammar_struct`: the generated grammar struct name, e.g. `Grammar`
/// - `root_node`: the root AST node type name, e.g. `Stmt`
/// - `token_type`: the token enum type name, e.g. `TokenType`
/// - `syntax_crate`: crate providing `AnyGrammar` and `TypedGrammar`,
///   e.g. `crate` (internal) or `syntaqlite_syntax` (external)
pub(crate) fn generate_grammar_module(
    dialect_fn: &str,
    grammar_struct: &str,
    root_node: &str,
    token_type: &str,
    syntax_crate: &str,
) -> String {
    let mut w = RustWriter::new();
    w.file_header();
    emit_grammar_module(
        &mut w,
        dialect_fn,
        grammar_struct,
        root_node,
        token_type,
        syntax_crate,
    );
    w.finish()
}

fn emit_grammar_module(
    w: &mut RustWriter,
    dialect_fn: &str,
    grammar_struct: &str,
    root_node: &str,
    token_type: &str,
    syntax_crate: &str,
) {
    w.lines(&format!(
        r#"
use {syntax_crate}::any::AnyGrammar;
use {syntax_crate}::typed::TypedGrammar;
use {syntax_crate}::util::{{SqliteSyntaxFlags, SqliteVersion}};

/// The dialect grammar handle.
///
/// Wraps a [`AnyGrammar`] and implements [`TypedGrammar`]. Obtain via [`grammar()`];
/// configure with [`with_version`](Self::with_version) and [`with_cflags`](Self::with_cflags).
#[derive(Clone)]
pub struct {grammar_struct} {{
    raw: AnyGrammar,
}}

impl {grammar_struct} {{
    /// Return the underlying [`AnyGrammar`] by value.
    pub fn into_raw(self) -> AnyGrammar {{
        self.raw
    }}

    /// Construct from a raw [`AnyGrammar`].
    ///
    /// Used by dialect loading infrastructure to build a typed grammar handle
    /// from the grammar embedded in a `CDialectTemplate`.
    pub fn from_raw(raw: AnyGrammar) -> Self {{
        Self {{ raw }}
    }}

    /// Set the target `SQLite` version.
    #[must_use]
    pub fn with_version(mut self, version: SqliteVersion) -> Self {{
        self.raw = self.raw.with_version(version);
        self
    }}

    /// Replace the entire cflags bitfield.
    #[must_use]
    pub fn with_cflags(mut self, cflags: SqliteSyntaxFlags) -> Self {{
        self.raw = self.raw.with_cflags(cflags);
        self
    }}
}}

impl From<{grammar_struct}> for AnyGrammar {{
    fn from(g: {grammar_struct}) -> AnyGrammar {{
        g.raw
    }}
}}

impl TypedGrammar for {grammar_struct} {{
    type Node<'a> = super::ast::{root_node}<'a>;
    type NodeId = super::ast::NodeId;
    type Token = super::tokens::{token_type};
}}

/// Returns the dialect grammar handle.
pub fn grammar() -> {grammar_struct} {{
    // SAFETY: {dialect_fn}() returns a valid static C grammar.
    let raw = unsafe {{ AnyGrammar::new(ffi::{dialect_fn}()) }};
    {grammar_struct} {{ raw }}
}}

// ── ffi ───────────────────────────────────────────────────────────────────────

mod ffi {{
    unsafe extern "C" {{
        pub(super) fn {dialect_fn}() -> {syntax_crate}::typed::CGrammar;
    }}
}}
"#
    ));
}

/// Generate a self-contained dialect accessor module.
///
/// Used both for external dialect crates (as part of `lib.rs`) and for the
/// internal `SQLite` dialect (as `sqlite/dialect.rs`).
///
/// - `dialect_fn`: the `extern "C"` symbol name, e.g. `syntaqlite_sqlite_dialect`
/// - `grammar_struct`: the typed grammar struct name, e.g. `Grammar`
/// - `dialect_crate`: crate providing `AnyDialect`, `TypedDialect`, `CDialectTemplate`,
///   e.g. `crate` (internal) or `syntaqlite` (external)
pub(crate) fn generate_dialect_module(
    dialect_fn: &str,
    grammar_struct: &str,
    dialect_crate: &str,
) -> String {
    let mut w = RustWriter::new();
    w.file_header();
    emit_dialect_module(&mut w, dialect_fn, grammar_struct, dialect_crate);
    w.finish()
}

fn emit_dialect_module(
    w: &mut RustWriter,
    dialect_fn: &str,
    grammar_struct: &str,
    dialect_crate: &str,
) {
    w.lines(&format!(
        r#"
use std::sync::LazyLock;

use syntaqlite_syntax::typed::{grammar_struct};
use syntaqlite_syntax::util::SqliteVersion;

use {dialect_crate}::dialect::{{AnyDialect, TypedDialect}};
use {dialect_crate}::util::SqliteFlags;

/// The typed dialect handle.
///
/// Wraps an [`AnyDialect`] and implements [`TypedDialect`]. Obtain via [`dialect()`];
/// configure with [`with_version`](Self::with_version) and [`with_cflags`](Self::with_cflags).
#[derive(Clone)]
pub struct Dialect {{
    raw: AnyDialect,
}}

impl Dialect {{
    /// Returns a new default dialect handle.
    pub fn new() -> Self {{
        dialect()
    }}

    /// Erase to an [`AnyDialect`].
    pub fn erase(self) -> AnyDialect {{
        self.raw
    }}

    /// Return the typed grammar handle for this dialect.
    pub fn grammar(&self) -> {grammar_struct} {{
        {grammar_struct}::from_raw(self.raw.grammar().clone())
    }}

    /// Return a copy targeting a specific `SQLite` version.
    #[must_use]
    pub fn with_version(self, version: SqliteVersion) -> Self {{
        Dialect {{ raw: self.raw.with_version(version) }}
    }}

    /// Return a copy with the given compile-time flags.
    #[must_use]
    pub fn with_cflags(self, flags: SqliteFlags) -> Self {{
        Dialect {{ raw: self.raw.with_cflags(flags) }}
    }}
}}

impl Default for Dialect {{
    fn default() -> Self {{
        Self::new()
    }}
}}

impl TypedDialect for Dialect {{}}

impl From<Dialect> for AnyDialect {{
    fn from(d: Dialect) -> AnyDialect {{
        d.raw
    }}
}}

impl std::ops::Deref for Dialect {{
    type Target = AnyDialect;
    fn deref(&self) -> &AnyDialect {{
        &self.raw
    }}
}}

impl std::ops::DerefMut for Dialect {{
    fn deref_mut(&mut self) -> &mut AnyDialect {{
        &mut self.raw
    }}
}}

static DIALECT: LazyLock<AnyDialect> = LazyLock::new(|| {{
    // SAFETY: {dialect_fn}() returns a pointer to a valid static SyntaqliteDialectTemplate
    // struct compiled into the binary. The data lives for the entire program lifetime.
    unsafe {{ AnyDialect::from_data(ffi::{dialect_fn}()) }}
}});

/// Returns the type-erased dialect handle (cached).
pub(crate) fn any_dialect() -> AnyDialect {{
    DIALECT.clone()
}}

/// Returns the typed dialect handle.
pub(crate) fn dialect() -> Dialect {{
    Dialect {{ raw: any_dialect() }}
}}

// ── ffi ───────────────────────────────────────────────────────────────────────

mod ffi {{
    use {dialect_crate}::dialect::CDialectTemplate;

    unsafe extern "C" {{
        pub(super) fn {dialect_fn}() -> *const CDialectTemplate;
    }}
}}
"#
    ));
}

pub(crate) fn generate_rust_lib(
    dialect_fn: &str,
    grammar_struct: &str,
    root_node: &str,
    token_type: &str,
) -> String {
    let mut w = RustWriter::new();
    w.file_header();
    emit_section(&mut w, LIB_MODULE_DECLS);
    emit_grammar_module(
        &mut w,
        dialect_fn,
        grammar_struct,
        root_node,
        token_type,
        "syntaqlite_syntax",
    );
    w.newline();
    w.finish()
}

/// Generate `build.rs` for a dialect crate.
///
/// The generated build script compiles the dialect's C sources via `cc`
/// and handles version/cflag pinning by passing `-D` flags to the C compiler.
pub(crate) fn generate_rust_build_rs(dialect_name: &str) -> String {
    let mut w = RustWriter::new();
    w.file_header();
    w.lines(&format!(
        r#"
use std::env;
use std::path::PathBuf;

fn main() {{
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap());
    let csrc = manifest_dir.join("csrc");
    let runtime_include = manifest_dir.join("../syntaqlite-sys/include");
    let target_os = env::var("CARGO_CFG_TARGET_OS").unwrap_or_default();

    // TypedDialectEnv sources — Lemon parser, tokenizer, keyword lookup, and dialect glue.
    // Grammar-agnostic engine C is built by the syntaqlite crate.
    let mut build = cc::Build::new();
    build
        .file(csrc.join("dialect.c"))
        .file(csrc.join("{dialect_name}_parse.c"))
        .file(csrc.join("{dialect_name}_tokenize.c"))
        .file(csrc.join("{dialect_name}_keyword.c"))
        .include(&manifest_dir) // for dialect csrc/ headers
        .include(manifest_dir.join("include")) // for dialect include/ headers
        .include(&runtime_include) // for shared syntaqlite/*.h and syntaqlite_dialect/*.h
        .flag("-Wno-int-conversion")
        .flag("-Wno-void-pointer-to-int-cast")
        .flag("-Wno-unused-variable")
        .flag("-Wno-unused-parameter")
        .flag("-Wno-comment");
    if target_os == "emscripten" {{
        build.flag("-fPIC");
    }}

    // ── Version pinning ─────────────────────────────────────────────────
    //
    // With --features pin-version, reads SYNTAQLITE_SQLITE_VERSION env var
    // and passes -DSYNTAQLITE_SQLITE_VERSION=<value> to cc.
    if env::var("CARGO_FEATURE_PIN_VERSION").is_ok() {{
        let ver_str = env::var("SYNTAQLITE_SQLITE_VERSION").unwrap_or_else(|_| {{
            panic!(
                "pin-version feature requires SYNTAQLITE_SQLITE_VERSION env var \
                 (e.g. SYNTAQLITE_SQLITE_VERSION=3035000)"
            )
        }});
        let _: i32 = ver_str.parse().unwrap_or_else(|_| {{
            panic!("SYNTAQLITE_SQLITE_VERSION must be an integer (e.g. 3035000), got: {{ver_str}}")
        }});
        build.define("SYNTAQLITE_SQLITE_VERSION", ver_str.as_str());
    }}

    // ── Cflag pinning ───────────────────────────────────────────────────
    //
    // With --features pin-cflags, scans for SYNTAQLITE_CFLAG_* env vars
    // and passes the same -D flags to cc.
    if env::var("CARGO_FEATURE_PIN_CFLAGS").is_ok() {{
        let all_entries = syntaqlite::dialect::cflag_table();

        // Pass the master switch.
        build.define("SYNTAQLITE_SQLITE_CFLAGS", None);

        // Scan env vars for SYNTAQLITE_CFLAG_* and pass matching -D flags.
        for entry in all_entries {{
            let env_key = format!("SYNTAQLITE_CFLAG_{{}}", entry.suffix);
            if env::var(&env_key).is_ok() {{
                build.define(&env_key, None);
                println!("cargo:rerun-if-env-changed={{env_key}}");
            }}
        }}
    }}

    build.compile("syntaqlite_dialect");

    println!("cargo:rerun-if-changed=build.rs");
    println!("cargo:rerun-if-changed=csrc");
    println!("cargo:rerun-if-changed=include");
    // TypedDialectEnv C files #include syntaqlite headers.
    println!("cargo:rerun-if-changed=../syntaqlite-sys/include");
    // Re-run when pinning env vars change.
    println!("cargo:rerun-if-env-changed=SYNTAQLITE_SQLITE_VERSION");
}}
"#
    ));
    w.finish()
}

/// Generate `Cargo.toml` for a dialect crate.
///
/// `crate_name` is the published crate name (e.g. `"syntaqlite"` for the
/// base `SQLite` dialect, `"syntaqlite-libsql"` for an extension dialect).
pub(crate) fn generate_cargo_toml(crate_name: &str) -> String {
    format!(
        r#"# @generated by syntaqlite-buildtools — DO NOT EDIT

[package]
name = "{crate_name}"
version = "0.1.0"
edition = "2024"

[features]
default = ["fmt"]
fmt = ["syntaqlite/fmt"]

# Pin version/cflags at compile time for dead-code elimination.
# Values come from env vars, using the same names as the C defines:
#
#   SYNTAQLITE_SQLITE_VERSION=3035000 cargo build --features pin-version
#
#   SYNTAQLITE_CFLAG_SQLITE_OMIT_WINDOWFUNC=1 \
#   SYNTAQLITE_CFLAG_SQLITE_ENABLE_FTS5=1 \
#   cargo build --features pin-cflags
#
pin-version = []   # Pin SQLite version via SYNTAQLITE_SQLITE_VERSION env var
pin-cflags = []    # Pin compile-time flags via SYNTAQLITE_CFLAG_* env vars

[build-dependencies]
cc = "1.0"
syntaqlite = {{ path = "../syntaqlite", default-features = false }}

[dependencies]
syntaqlite = {{ path = "../syntaqlite", default-features = false }}
syntaqlite-parser = {{ path = "../syntaqlite-parser" }}
"#
    )
}
