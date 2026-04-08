// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use std::collections::HashSet;
use std::path::{Path, PathBuf};

use crate::codegen_api::{CodegenArtifacts, DialectNaming};
use crate::dialect_codegen::c_dialect::DialectCIncludes;

/// A C header artifact: where to write it and what `#include "..."` string references it.
pub struct CHeader {
    /// Path relative to [`OutputLayout::root`] where the file is written. `None` = skip.
    pub write: Option<String>,
    /// Verbatim path used in `#include "..."` directives in other generated C files.
    pub include: String,
}

/// Describes where to write every generated artifact and what include paths to use.
///
/// All write paths are `Option<String>` relative to [`OutputLayout::root`]; `None` means skip.
/// For C headers, the [`CHeader`] struct bundles both the write path and the include path,
/// making the mapping between files-on-disk and `#include` directives explicit.
pub struct OutputLayout {
    /// Root directory. All write paths are relative to this.
    pub root: PathBuf,

    // ── C internal headers (csrc/) ──────────────────────────────────────────
    /// AST builder header (`dialect_builder.h`).
    pub ast_builder_h: CHeader,
    /// Dialect field metadata header (`dialect_meta.h`).
    pub dialect_meta_h: CHeader,
    /// Dialect formatter tables header (`dialect_fmt.h`).
    pub dialect_fmt_h: CHeader,
    /// Dialect token categories header (`dialect_tokens.h`).
    pub dialect_tokens_h: CHeader,
    /// Dialect dispatch header (`*_dialect_dispatch.h`).
    pub dialect_dispatch_h: CHeader,
    /// Parser API header (`sqlite_parse.h`).
    pub parse_api_h: CHeader,
    /// Tokenizer header (`sqlite_tokenize.h`).
    pub tokenize_h: CHeader,
    /// Keyword header (`sqlite_keyword.h`).
    pub keyword_h: CHeader,

    // ── C sources (csrc/) ───────────────────────────────────────────────────
    /// Dialect glue C source (`dialect.c`).
    pub dialect_c: Option<String>,
    /// Parser C source (`sqlite_parse.c`).
    pub parse_c: Option<String>,
    /// Tokenizer C source (`sqlite_tokenize.c`).
    pub tokenize_c: Option<String>,
    /// Keyword hash C source (`sqlite_keyword.c`).
    pub keyword_c: Option<String>,

    // ── C public headers (include/<dialect>/) ───────────────────────────────
    /// AST node definitions header (`*_node.h`).
    pub ast_nodes_h: CHeader,
    /// Dialect public header (`*.h`).
    pub dialect_h: CHeader,
    /// `TypedDialectEnv` tokens header: all `SYNTAQLITE_TK_*` defines (from lemon parse.h).
    pub tokens_h: CHeader,
    /// Runtime tokens header: minimal subset of tokens needed by `token_wrapped.c`.
    pub runtime_tokens_h: CHeader,
    /// `SQLite` cflag index constants header (`cflags.h`).
    pub cflags_h: CHeader,
    /// Semantic role byte array header (`dialect_roles.h`).
    pub dialect_roles_h: CHeader,

    // ── Rust sources (src/) ─────────────────────────────────────────────────
    /// Rust token constants (`tokens.rs`).
    pub tokens_rs: Option<String>,
    /// Rust FFI node definitions (`ffi.rs`).
    pub ffi_rs: Option<String>,
    /// Rust AST node types (`ast.rs`).
    pub ast_rs: Option<String>,
    /// Grammar module (`grammar.rs`).
    pub grammar_rs: Option<String>,
    /// Dialect accessor module (`dialect.rs`).
    pub dialect_rs: Option<String>,
    /// Crate root module (`lib.rs`).
    pub lib_rs: Option<String>,
    /// Functions catalog (`functions_catalog.rs`).
    pub functions_catalog_rs: Option<String>,

    // ── Crate root ───────────────────────────────────────────────────────────
    /// Build script (`build.rs`).
    pub build_rs: Option<String>,
    /// Cargo manifest (`Cargo.toml`).
    pub cargo_toml: Option<String>,

    // ── Python ────────────────────────────────────────────────────────────
    /// Python enum types (`enums.py`).
    pub enums_py: Option<String>,
    /// C header for Python extension (`_py_ast_wrap.h`).
    pub ast_wrap_h: Option<String>,
    /// Python typed AST node classes (`nodes.py`).
    pub nodes_py: Option<String>,
}

impl OutputLayout {
    /// Return a `DialectCIncludes` borrowing the include strings from this layout.
    #[must_use]
    pub fn c_includes(&self) -> DialectCIncludes<'_> {
        DialectCIncludes {
            ast_builder_h: &self.ast_builder_h.include,
            dialect_meta_h: &self.dialect_meta_h.include,
            dialect_fmt_h: &self.dialect_fmt_h.include,
            dialect_tokens_h: &self.dialect_tokens_h.include,
            parse_api_h: &self.parse_api_h.include,
            tokenize_h: &self.tokenize_h.include,
            keyword_h: &self.keyword_h.include,
            tokens_header: &self.tokens_h.include,
            dialect_roles_h: &self.dialect_roles_h.include,
        }
    }

    /// Layout for the internal `SQLite` dialect spread across two crates in the workspace.
    ///
    /// - `root`: workspace root (e.g. `Path::new(".")`).
    /// - `dialect_crate`: crate directory name, e.g. `"syntaqlite-parser-sqlite"`.
    /// - `shared_crate`: shared crate directory name, e.g. `"syntaqlite-parser"`.
    /// - `dialect_name`: e.g. `"sqlite"`.
    /// - `include_dir_name`: subdirectory under `include/`, e.g. `"syntaqlite_sqlite"`.
    #[must_use]
    pub fn for_sqlite(
        root: &Path,
        dialect_crate: &str,
        shared_crate: &str,
        dialect_name: &str,
        include_dir_name: &str,
    ) -> Self {
        let dc = dialect_crate;
        let sc = shared_crate;
        let dn = dialect_name;
        let id = include_dir_name;
        let csrc = format!("{dc}/csrc/sqlite");
        // Include paths for SQLite are relative to the dialect crate root,
        // matching the -I flag in syntaqlite-parser-sqlite/build.rs (.include(&manifest_dir)).
        let ip = "csrc/sqlite/"; // internal include prefix relative to crate root
        Self {
            root: root.to_path_buf(),
            // C internal headers
            ast_builder_h: CHeader {
                write: Some(format!("{csrc}/dialect_builder.h")),
                include: format!("{ip}dialect_builder.h"),
            },
            dialect_meta_h: CHeader {
                write: Some(format!("{csrc}/dialect_meta.h")),
                include: format!("{ip}dialect_meta.h"),
            },
            dialect_fmt_h: CHeader {
                write: Some(format!("{csrc}/dialect_fmt.h")),
                include: format!("{ip}dialect_fmt.h"),
            },
            dialect_tokens_h: CHeader {
                write: Some(format!("{csrc}/dialect_tokens.h")),
                include: format!("{ip}dialect_tokens.h"),
            },
            dialect_dispatch_h: CHeader {
                write: Some(format!("{csrc}/{dn}_dialect_dispatch.h")),
                include: format!("{ip}{dn}_dialect_dispatch.h"),
            },
            parse_api_h: CHeader {
                write: Some(format!("{csrc}/sqlite_parse.h")),
                include: format!("{ip}sqlite_parse.h"),
            },
            tokenize_h: CHeader {
                write: Some(format!("{csrc}/sqlite_tokenize.h")),
                include: format!("{ip}sqlite_tokenize.h"),
            },
            keyword_h: CHeader {
                write: Some(format!("{csrc}/sqlite_keyword.h")),
                include: format!("{ip}sqlite_keyword.h"),
            },
            // C sources
            dialect_c: Some(format!("{csrc}/dialect.c")),
            parse_c: Some(format!("{csrc}/sqlite_parse.c")),
            tokenize_c: Some(format!("{csrc}/sqlite_tokenize.c")),
            keyword_c: Some(format!("{csrc}/sqlite_keyword.c")),
            // C public headers
            ast_nodes_h: CHeader {
                write: Some(format!("{dc}/include/{id}/{dn}_node.h")),
                include: format!("syntaqlite/{dn}_node.h"),
            },
            dialect_h: CHeader {
                write: Some(format!("{dc}/include/{id}/{dn}.h")),
                include: format!("syntaqlite/{dn}.h"),
            },
            tokens_h: CHeader {
                write: Some(format!("{dc}/include/{id}/{dn}_tokens.h")),
                include: format!("{id}/{dn}_tokens.h"),
            },
            runtime_tokens_h: CHeader {
                write: Some(format!("{sc}/csrc/tokens.h")),
                include: "csrc/tokens.h".to_string(),
            },
            cflags_h: CHeader {
                write: Some(format!("{sc}/include/syntaqlite/cflags.h")),
                include: "syntaqlite/cflags.h".to_string(),
            },
            dialect_roles_h: CHeader {
                write: Some(format!("{csrc}/dialect_roles.h")),
                include: format!("{ip}dialect_roles.h"),
            },
            // Rust: all in dialect_crate/src/sqlite/ subdirectory
            tokens_rs: Some(format!("{dc}/src/sqlite/tokens.rs")),
            ffi_rs: Some(format!("{dc}/src/sqlite/ffi.rs")),
            ast_rs: Some(format!("{dc}/src/sqlite/ast.rs")),
            grammar_rs: Some(format!("{dc}/src/sqlite/grammar.rs")),
            dialect_rs: Some(format!("{dc}/src/sqlite/dialect.rs")),
            lib_rs: None, // hand-maintained
            functions_catalog_rs: None,
            // Crate root: hand-maintained for the internal crate
            build_rs: None,
            cargo_toml: None,
            // Python
            enums_py: Some("python/syntaqlite/enums.py".to_string()),
            ast_wrap_h: Some("python/csrc/_py_ast_wrap.h".to_string()),
            nodes_py: Some("python/syntaqlite/nodes.py".to_string()),
        }
    }

    /// Layout for an external dialect crate (all output in one directory).
    ///
    /// - `root`: the dialect crate root.
    /// - `dialect_name`: e.g. `"perfetto"`.
    /// - `include_dir_name`: subdirectory under `include/`, e.g. `"syntaqlite_perfetto"`.
    #[must_use]
    pub fn for_external(root: &Path, dialect_name: &str, include_dir_name: &str) -> Self {
        let dn = dialect_name;
        let id = include_dir_name;
        // External dialect crates compile with -I csrc/, so internal headers
        // are included by filename only (no directory prefix).
        Self {
            root: root.to_path_buf(),
            ast_builder_h: CHeader {
                write: Some("csrc/dialect_builder.h".to_string()),
                include: "dialect_builder.h".to_string(),
            },
            dialect_meta_h: CHeader {
                write: Some("csrc/dialect_meta.h".to_string()),
                include: "dialect_meta.h".to_string(),
            },
            dialect_fmt_h: CHeader {
                write: Some("csrc/dialect_fmt.h".to_string()),
                include: "dialect_fmt.h".to_string(),
            },
            dialect_tokens_h: CHeader {
                write: Some("csrc/dialect_tokens.h".to_string()),
                include: "dialect_tokens.h".to_string(),
            },
            dialect_dispatch_h: CHeader {
                write: Some(format!("csrc/{dn}_dialect_dispatch.h")),
                include: format!("{dn}_dialect_dispatch.h"),
            },
            parse_api_h: CHeader {
                write: Some("csrc/sqlite_parse.h".to_string()),
                include: "sqlite_parse.h".to_string(),
            },
            tokenize_h: CHeader {
                write: Some("csrc/sqlite_tokenize.h".to_string()),
                include: "sqlite_tokenize.h".to_string(),
            },
            keyword_h: CHeader {
                write: Some("csrc/sqlite_keyword.h".to_string()),
                include: "sqlite_keyword.h".to_string(),
            },
            dialect_c: Some("csrc/dialect.c".to_string()),
            parse_c: Some("csrc/sqlite_parse.c".to_string()),
            tokenize_c: Some("csrc/sqlite_tokenize.c".to_string()),
            keyword_c: Some("csrc/sqlite_keyword.c".to_string()),
            ast_nodes_h: CHeader {
                write: Some(format!("include/{id}/{dn}_node.h")),
                include: format!("{id}/{dn}_node.h"),
            },
            dialect_h: CHeader {
                write: Some(format!("include/{id}/{dn}.h")),
                include: format!("{id}/{dn}.h"),
            },
            tokens_h: CHeader {
                write: Some(format!("include/{id}/{dn}_tokens.h")),
                include: format!("{id}/{dn}_tokens.h"),
            },
            runtime_tokens_h: CHeader {
                write: Some("csrc/tokens.h".to_string()),
                include: "csrc/tokens.h".to_string(),
            },
            cflags_h: CHeader {
                write: None,
                include: "syntaqlite/cflags.h".to_string(),
            },
            dialect_roles_h: CHeader {
                write: Some("csrc/dialect_roles.h".to_string()),
                include: "dialect_roles.h".to_string(),
            },
            tokens_rs: Some("src/tokens.rs".to_string()),
            ffi_rs: Some("src/ffi.rs".to_string()),
            ast_rs: Some("src/ast.rs".to_string()),
            grammar_rs: None, // grammar accessor lives in lib.rs for external dialects
            dialect_rs: None, // TODO: generate dialect.rs for external crates (needs "syntaqlite" crate path)
            lib_rs: Some("src/lib.rs".to_string()),
            functions_catalog_rs: None,
            build_rs: Some("build.rs".to_string()),
            cargo_toml: Some("Cargo.toml".to_string()),
            enums_py: None,
            ast_wrap_h: None,
            nodes_py: None,
        }
    }

    /// Layout for a temporary directory used by the amalgamation code path.
    /// Only C files are written; all Rust fields are `None`.
    ///
    /// - `root`: the temp directory.
    /// - `dialect_name`: e.g. `"sqlite"`.
    /// - `include_dir_name`: subdirectory under `include/`.
    #[must_use]
    pub fn for_amalg_temp(root: &Path, dialect_name: &str, include_dir_name: &str) -> Self {
        let dn = dialect_name;
        let id = include_dir_name;
        // Amalg temp uses "csrc/" prefix because the amalgamator resolves
        // headers relative to the temp root, and internal headers are in csrc/.
        let ip = "csrc/";
        Self {
            root: root.to_path_buf(),
            ast_builder_h: CHeader {
                write: Some("csrc/dialect_builder.h".to_string()),
                include: format!("{ip}dialect_builder.h"),
            },
            dialect_meta_h: CHeader {
                write: Some("csrc/dialect_meta.h".to_string()),
                include: format!("{ip}dialect_meta.h"),
            },
            dialect_fmt_h: CHeader {
                write: Some("csrc/dialect_fmt.h".to_string()),
                include: format!("{ip}dialect_fmt.h"),
            },
            dialect_tokens_h: CHeader {
                write: Some("csrc/dialect_tokens.h".to_string()),
                include: format!("{ip}dialect_tokens.h"),
            },
            dialect_dispatch_h: CHeader {
                write: Some(format!("csrc/{dn}_dialect_dispatch.h")),
                include: format!("{ip}{dn}_dialect_dispatch.h"),
            },
            parse_api_h: CHeader {
                write: Some("csrc/sqlite_parse.h".to_string()),
                include: format!("{ip}sqlite_parse.h"),
            },
            tokenize_h: CHeader {
                write: Some("csrc/sqlite_tokenize.h".to_string()),
                include: format!("{ip}sqlite_tokenize.h"),
            },
            keyword_h: CHeader {
                write: Some("csrc/sqlite_keyword.h".to_string()),
                include: format!("{ip}sqlite_keyword.h"),
            },
            dialect_c: Some("csrc/dialect.c".to_string()),
            parse_c: Some("csrc/sqlite_parse.c".to_string()),
            tokenize_c: Some("csrc/sqlite_tokenize.c".to_string()),
            keyword_c: Some("csrc/sqlite_keyword.c".to_string()),
            ast_nodes_h: CHeader {
                write: Some(format!("include/{id}/{dn}_node.h")),
                include: format!("{id}/{dn}_node.h"),
            },
            dialect_h: CHeader {
                write: Some(format!("include/{id}/{dn}.h")),
                include: format!("{id}/{dn}.h"),
            },
            tokens_h: CHeader {
                write: Some(format!("include/{id}/{dn}_tokens.h")),
                include: format!("{id}/{dn}_tokens.h"),
            },
            runtime_tokens_h: CHeader {
                write: Some("csrc/tokens.h".to_string()),
                include: "csrc/tokens.h".to_string(),
            },
            cflags_h: CHeader {
                write: None,
                include: "syntaqlite/cflags.h".to_string(),
            },
            dialect_roles_h: CHeader {
                write: Some("csrc/dialect_roles.h".to_string()),
                include: format!("{ip}dialect_roles.h"),
            },
            tokens_rs: None,
            ffi_rs: None,
            ast_rs: None,
            grammar_rs: None,
            dialect_rs: None,
            lib_rs: None,
            functions_catalog_rs: None,
            build_rs: None,
            cargo_toml: None,
            enums_py: None,
            ast_wrap_h: None,
            nodes_py: None,
        }
    }

    /// Write all generated artifacts to the filesystem using this layout.
    ///
    /// # Errors
    ///
    /// Returns an error if directory creation or file writing fails.
    pub(crate) fn write_codegen_artifacts(
        &self,
        dialect: &DialectNaming,
        artifacts: CodegenArtifacts,
        ensure_dir_fn: &impl Fn(&Path) -> Result<(), String>,
        write_file_fn: &impl Fn(&Path, &str) -> Result<(), String>,
    ) -> Result<(), String> {
        let mut seen_dirs: HashSet<PathBuf> = HashSet::new();

        let mut write = |path: &Option<String>, content: &str| -> Result<(), String> {
            let Some(rel) = path else {
                return Ok(());
            };
            let dest = self.root.join(rel);
            if let Some(dir) = dest.parent()
                && seen_dirs.insert(dir.to_path_buf())
            {
                ensure_dir_fn(dir)?;
            }
            write_file_fn(&dest, content)
        };

        // C internal headers
        write(&self.ast_builder_h.write, &artifacts.ast_builder_h)?;
        write(&self.dialect_meta_h.write, &artifacts.dialect_meta_h)?;
        write(&self.dialect_fmt_h.write, &artifacts.dialect_fmt_h)?;
        write(&self.dialect_tokens_h.write, &artifacts.dialect_tokens_h)?;
        write(
            &self.dialect_dispatch_h.write,
            &artifacts.dialect_dispatch_h,
        )?;
        write(&self.parse_api_h.write, &artifacts.parse_api_h)?;
        write(&self.tokenize_h.write, &artifacts.tokenize_h)?;
        write(&self.keyword_h.write, &artifacts.keyword_h)?;

        // C sources
        write(&self.dialect_c, &artifacts.dialect_c)?;
        write(&self.parse_c, &artifacts.parse_c)?;
        write(&self.tokenize_c, &artifacts.tokenize_c)?;
        write(&self.keyword_c, &artifacts.keyword_c)?;

        // C public headers
        write(&self.ast_nodes_h.write, &artifacts.ast_nodes_h)?;
        write(&self.dialect_h.write, &artifacts.dialect_h)?;
        if self.tokens_h.write.is_some() {
            let guarded = dialect.guarded_tokens_header(&artifacts.parse_h);
            write(&self.tokens_h.write, &guarded)?;
        }
        write(&self.runtime_tokens_h.write, &artifacts.runtime_tokens_h)?;
        write(&self.cflags_h.write, &artifacts.cflags_h)?;
        write(&self.dialect_roles_h.write, &artifacts.dialect_roles_h)?;

        // Rust
        if let Some(rust) = artifacts.rust {
            write(&self.tokens_rs, &rust.tokens_rs)?;
            write(&self.ffi_rs, &rust.ffi_rs)?;
            write(&self.ast_rs, &rust.ast_rs)?;
            if let Some(ref content) = rust.grammar_rs {
                write(&self.grammar_rs, content)?;
            }
            if let Some(ref content) = rust.dialect_rs {
                write(&self.dialect_rs, content)?;
            }
            write(&self.lib_rs, &rust.lib_rs)?;
            if let Some(ref content) = rust.functions_catalog_rs {
                write(&self.functions_catalog_rs, content)?;
            }
            write(&self.build_rs, &rust.build_rs)?;
            write(&self.cargo_toml, &rust.cargo_toml)?;
        }

        // Python
        if let Some(python) = artifacts.python {
            write(&self.enums_py, &python.enums)?;
            write(&self.ast_wrap_h, &python.ast_wrap_h)?;
            write(&self.nodes_py, &python.nodes)?;
        }

        Ok(())
    }
}
