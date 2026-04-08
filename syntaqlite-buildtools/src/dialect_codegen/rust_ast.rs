// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use std::collections::HashSet;
use std::fmt::Write as _;

use crate::util::pascal_case;
use crate::util::rust_writer::RustWriter;
use crate::util::synq_parser::{Field, Storage};

use super::{AstModel, NodeLikeRef};

// ── Rust codegen ────────────────────────────────────────────────────────

/// Map a field to its Rust type name (for FFI structs in ffi.rs).
fn rust_ffi_field_type(
    field: &Field,
    enum_names: &HashSet<&str>,
    flags_names: &HashSet<&str>,
) -> String {
    match field.storage {
        Storage::Index => "AnyNodeId".into(),
        Storage::Inline => {
            let t = &field.type_name;
            if t == "Bool" {
                "Bool".into()
            } else if enum_names.contains(t.as_str()) || flags_names.contains(t.as_str()) {
                format!("super::ast::{t}")
            } else if t == "SyntaqliteSourceSpan" {
                "SourceSpan".into()
            } else {
                t.clone()
            }
        }
    }
}

/// Map a field to its ergonomic return type for view struct accessors.
fn rust_view_return_type(
    field: &Field,
    _enum_names: &HashSet<&str>,
    _flags_names: &HashSet<&str>,
    _node_names: &HashSet<&str>,
    _list_names: &HashSet<&str>,
) -> String {
    match field.storage {
        Storage::Index => {
            let t = field.type_name.as_str();
            format!("Option<{t}<'a>>")
        }
        Storage::Inline => {
            let t = &field.type_name;
            if t == "Bool" {
                "bool".into()
            } else if t == "SyntaqliteSourceSpan" {
                "&'a str".into()
            } else {
                t.clone()
            }
        }
    }
}

/// Generate the accessor body for a view struct field.
fn rust_view_accessor_body(field: &Field, ffi_path: &str) -> String {
    let fname = rust_field_name(&field.name);
    match field.storage {
        Storage::Index => {
            format!("GrammarNodeType::from_result(self.stmt_result, self.raw.{fname})")
        }
        Storage::Inline => {
            let t = &field.type_name;
            if t == "Bool" {
                format!("self.raw.{fname} == {ffi_path}::Bool::True")
            } else if t == "SyntaqliteSourceSpan" {
                format!("self.raw.{fname}.as_str(self.stmt_result.source())")
            } else {
                format!("self.raw.{fname}")
            }
        }
    }
}

/// Check if a name is a Rust keyword that needs raw-identifier syntax.
fn is_rust_keyword(name: &str) -> bool {
    matches!(
        name,
        "as" | "break"
            | "const"
            | "continue"
            | "crate"
            | "else"
            | "enum"
            | "extern"
            | "false"
            | "fn"
            | "for"
            | "if"
            | "impl"
            | "in"
            | "let"
            | "loop"
            | "match"
            | "mod"
            | "move"
            | "mut"
            | "pub"
            | "ref"
            | "return"
            | "self"
            | "Self"
            | "static"
            | "struct"
            | "super"
            | "trait"
            | "true"
            | "type"
            | "unsafe"
            | "use"
            | "where"
            | "while"
            | "async"
            | "await"
            | "dyn"
            | "abstract"
            | "become"
            | "box"
            | "do"
            | "final"
            | "macro"
            | "override"
            | "priv"
            | "typeof"
            | "unsized"
            | "virtual"
            | "yield"
            | "try"
    )
}

/// Escape a name if it's a Rust keyword.
fn rust_field_name(name: &str) -> String {
    if is_rust_keyword(name) {
        format!("r#{name}")
    } else {
        name.to_string()
    }
}

fn emit_rust_value_enum(w: &mut RustWriter, name: &str, variants: &[String]) {
    w.line("#[derive(Debug, Clone, Copy, PartialEq, Eq)]");
    w.line("#[repr(u32)]");
    let _ = writeln!(w, "pub enum {name} {{");
    w.indent();
    for (i, v) in variants.iter().enumerate() {
        let variant_name = pascal_case(v);
        let _ = writeln!(w, "{variant_name} = {i},");
    }
    w.close_block("}");
    w.newline();

    let _ = writeln!(w, "impl {name} {{");
    w.indent();
    w.open_block("pub fn as_str(&self) -> &'static str {");
    w.open_block("match self {");
    for v in variants {
        let variant_name = pascal_case(v);
        let _ = writeln!(w, "{name}::{variant_name} => \"{v}\",");
    }
    w.close_block("}");
    w.close_block("}");
    w.close_block("}");
    w.newline();
}

fn emit_rust_flags_type(w: &mut RustWriter, name: &str, flags: &[(String, u32)]) {
    w.line("#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]");
    w.line("#[repr(transparent)]");
    let _ = writeln!(w, "pub struct {name}(u8);");
    w.newline();

    let _ = writeln!(w, "impl {name} {{");
    w.indent();
    w.open_block("pub fn bits(self) -> u8 {");
    w.line("self.0");
    w.close_block("}");
    let mut sorted: Vec<_> = flags.iter().collect();
    sorted.sort_by_key(|(_, v)| *v);
    for (flag_name, bit) in &sorted {
        let method = flag_name.to_lowercase();
        let _ = writeln!(w, "pub fn {method}(&self) -> bool {{");
        w.indent();
        let _ = writeln!(w, "self.0 & {bit} != 0");
        w.close_block("}");
    }
    w.newline();
    w.close_block("}");
    w.newline();
}

fn emit_rust_node_tag_type(w: &mut RustWriter, model: &AstModel<'_>) {
    let mut tag_names: Vec<(String, u32)> = Vec::new();
    let mut list_tags: Vec<String> = Vec::new();
    for item in model.node_like_items() {
        match item {
            NodeLikeRef::Node(node) => {
                tag_names.push((node.name.to_string(), model.tag_for(node.name)));
            }
            NodeLikeRef::List(list) => {
                tag_names.push((list.name.to_string(), model.tag_for(list.name)));
                list_tags.push(list.name.to_string());
            }
        }
    }

    w.line("#[derive(Debug, Clone, Copy, PartialEq, Eq)]");
    w.line("#[repr(u32)]");
    w.open_block("pub enum NodeTag {");
    w.line("Null = 0,");
    for (name, tag) in &tag_names {
        let _ = writeln!(w, "{name} = {tag},");
    }
    w.close_block("}");
    w.newline();

    w.open_block("impl From<NodeTag> for crate::any::AnyNodeTag {");
    w.open_block("fn from(t: NodeTag) -> crate::any::AnyNodeTag {");
    w.line("crate::any::AnyNodeTag(t as u32)");
    w.close_block("}");
    w.close_block("}");
    w.newline();

    w.open_block("impl NodeTag {");
    w.open_block("pub(crate) fn from_raw(raw: u32) -> Option<NodeTag> {");
    w.open_block("match raw {");
    w.line("0 => Some(NodeTag::Null),");
    for (name, tag) in &tag_names {
        let _ = writeln!(w, "{tag} => Some(NodeTag::{name}),");
    }
    w.line("_ => None,");
    w.close_block("}");
    w.close_block("}");
    w.close_block("}");
    w.newline();
}

fn emit_rust_node_structs(
    w: &mut RustWriter,
    model: &AstModel<'_>,
    enum_names: &HashSet<&str>,
    flags_names: &HashSet<&str>,
    struct_visibility: &str,
    field_visibility: &str,
    field_type: fn(&Field, &HashSet<&str>, &HashSet<&str>) -> String,
) {
    for node in model.nodes() {
        let name = node.name;
        let fields = node.fields;
        w.line("#[derive(Debug, Clone)]");
        w.line("#[repr(C)]");
        let _ = writeln!(w, "{struct_visibility} struct {name} {{");
        w.indent();
        let _ = writeln!(w, "{field_visibility} tag: u32,");
        for field in fields {
            let ty = field_type(field, enum_names, flags_names);
            let fname = rust_field_name(&field.name);
            let _ = writeln!(w, "{field_visibility} {fname}: {ty},");
        }
        w.close_block("}");
        w.newline();
    }
}

fn emit_rust_node_tag_accessor(
    w: &mut RustWriter,
    node_like_items: &[NodeLikeRef<'_>],
    open_for_extension: bool,
) {
    w.doc_comment("The node's tag.");
    w.open_block("pub fn tag(&self) -> NodeTag {");
    w.open_block("match self {");
    for item in node_like_items {
        let name = item.name();
        let _ = writeln!(w, "Node::{name}(..) => NodeTag::{name},");
    }
    if open_for_extension {
        w.line("Node::Other { .. } => NodeTag::Null,");
    } else {
        w.line("Node::__Phantom(_) => unreachable!(),");
    }
    w.close_block("}");
    w.close_block("}");
    w.newline();
}

/// Generate Rust source for token type enum.
pub(crate) fn generate_rust_tokens(tokens: &[(String, u32)], type_name: &str) -> String {
    let mut w = RustWriter::new();
    w.file_header();
    // Token names come from SQLite's C grammar (all-uppercase); allow the lint.
    w.line("#![allow(clippy::upper_case_acronyms, missing_docs)]");
    w.newline();

    // TokenType enum
    w.line("#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]");
    w.line("#[repr(u32)]");
    let _ = writeln!(w, "pub enum {type_name} {{");
    w.indent();
    for (name, value) in tokens {
        let variant = pascal_case(name);
        let _ = writeln!(w, "{variant} = {value},");
    }
    w.close_block("}");
    w.newline();

    let _ = writeln!(w, "impl From<{type_name}> for u32 {{");
    w.indent();
    let _ = writeln!(w, "fn from(t: {type_name}) -> u32 {{");
    w.indent();
    w.line("t as u32");
    w.close_block("}");
    w.close_block("}");
    w.newline();

    let _ = writeln!(w, "impl From<{type_name}> for crate::any::AnyTokenType {{");
    w.indent();
    let _ = writeln!(w, "fn from(t: {type_name}) -> crate::any::AnyTokenType {{");
    w.indent();
    w.line("crate::any::AnyTokenType::from_raw(t as u32)");
    w.close_block("}");
    w.close_block("}");
    w.newline();

    let _ = writeln!(w, "impl crate::ast::GrammarTokenType for {type_name} {{");
    w.indent();
    w.line("#[expect(clippy::too_many_lines)]");
    w.open_block("fn from_token_type(raw: crate::any::AnyTokenType) -> Option<Self> {");
    w.open_block("match raw.0 {");
    for (name, value) in tokens {
        let variant = pascal_case(name);
        let _ = writeln!(w, "{value} => Some({type_name}::{variant}),");
    }
    w.line("_ => None,");
    w.close_block("}");
    w.close_block("}");
    w.close_block("}");

    w.finish()
}

/// Module paths used by the Rust AST/FFI codegen.
pub(crate) struct RustAstPaths<'a> {
    /// Import prefix for the dialect crate, e.g. `"crate"` (internal) or
    /// `"syntaqlite"` (external).
    pub crate_prefix: &'a str,
    /// Path to FFI node structs, e.g. `"crate::ffi"`.
    pub ffi_path: &'a str,
    /// Path to `NodeId`/`SourceSpan`/`NodeList`, e.g. `"syntaqlite_parser::nodes"`.
    pub nodes_path: &'a str,
    /// Fully-qualified path to the dialect grammar struct, e.g.
    /// `"super::grammar::SqliteGrammar"`. Used as the `G` parameter in
    /// `TypedNodeList<'a, G, T>` type aliases emitted into `ast.rs`.
    pub grammar_type: &'a str,
}

/// Generate Rust source for the FFI layer (`ffi.rs`).
///
/// Emits `#[repr(C)]` node structs, the `Bool` enum, and `ArenaNode` impls
/// that declare each struct's tag constant.
///
/// `crate_prefix` controls import paths: `"crate"` for the internal syntaqlite
/// crate, `"syntaqlite"` for external dialect crates.
impl AstModel<'_> {
    pub(crate) fn generate_rust_ffi_nodes(&self, paths: &RustAstPaths<'_>) -> String {
        let nodes_path = paths.nodes_path;
        let enum_names = self.enum_names();
        let flags_names = self.flags_names();

        let mut w = RustWriter::new();
        w.file_header();
        w.line("#![allow(clippy::struct_field_names)]");
        w.newline();
        w.lines(&format!(
            "
        use {nodes_path}::{{ArenaNode, AnyNodeId, SourceSpan}};
    ",
        ));
        w.newline();

        // Bool enum — variants populated from C via FFI, not constructed in Rust.
        w.line("#[derive(Debug, Clone, Copy, PartialEq, Eq)]");
        w.line("#[repr(u32)]");
        w.open_block("pub(crate) enum Bool {");
        w.line("#[expect(dead_code)] // Populated from C FFI");
        w.line("False = 0,");
        w.line("True = 1,");
        w.close_block("}");
        w.newline();

        // Node structs with ArenaNode impls
        emit_rust_node_structs(
            &mut w,
            self,
            enum_names,
            flags_names,
            "pub(crate)",
            "pub(crate)",
            rust_ffi_field_type,
        );

        // ArenaNode impls — declare tag constant for each node struct
        for node in self.nodes() {
            let name = node.name;
            let tag = self.tag_for(name);
            w.line("// SAFETY: TAG matches the value the C parser writes into the `tag` field.");
            let _ = writeln!(
                w,
                "unsafe impl ArenaNode for {name} {{ const TAG: u32 = {tag}; }}"
            );
            w.newline();
        }

        w.finish()
    }

    /// Generate Rust source for the public AST layer (`ast.rs`).
    ///
    /// Emits enums, flags, `NodeTag`, view structs with ergonomic accessors,
    /// and the `Node<'a>` enum that wraps them.
    ///
    /// - `crate_prefix`: `"syntaqlite_parser"` for the internal `SQLite` dialect,
    ///   `"syntaqlite"` for external dialect crates.
    /// - `ffi_path`: module path to the dialect FFI structs (`crate::ffi` for both cases).
    #[expect(clippy::too_many_lines)]
    pub(crate) fn generate_rust_ast(
        &self,
        paths: &RustAstPaths<'_>,
        open_for_extension: bool,
    ) -> String {
        let crate_prefix = paths.crate_prefix;
        let ffi_path = paths.ffi_path;
        let grammar_type = paths.grammar_type;
        let enum_names = self.enum_names();
        let flags_names = self.flags_names();
        let node_names = self.node_names();
        let list_names = self.list_names();
        let abstract_items = self.abstract_items();

        let mut w = RustWriter::new();
        w.file_header();
        w.line(
            "#![allow(missing_docs, clippy::upper_case_acronyms, clippy::elidable_lifetime_names)]",
        );
        w.newline();
        if !open_for_extension {
            w.line("use std::marker::PhantomData;");
        }
        w.lines(&format!(
            "
        use {crate_prefix}::ast::{{TypedNodeId, AnyNodeId, AnyNode, GrammarNodeType, TypedNodeList}};
        use {crate_prefix}::parser::AnyParsedStatement;
        "
        ));
        w.newline();

        // NodeTag enum
        emit_rust_node_tag_type(&mut w, self);

        // Concrete value enums and flags — one per dialect, implementing the Like traits.
        for item in self.enums() {
            if item.name == "Bool" {
                continue;
            }
            emit_rust_value_enum(&mut w, item.name, item.variants);
        }
        for item in self.flags() {
            emit_rust_flags_type(&mut w, item.name, item.flags);
        }

        // Abstract type enums (Expr, Stmt, etc.)
        for &(abs_name, members) in abstract_items {
            let _ = writeln!(
                w,
                "/// Abstract `{abs_name}` — pattern-match to access the concrete type."
            );
            w.line("#[derive(Debug, Clone, Copy)]");
            let _ = writeln!(w, "pub enum {abs_name}<'a> {{");
            w.indent();
            for member in members {
                if node_names.contains(member.as_str()) || list_names.contains(member.as_str()) {
                    let _ = writeln!(w, "{member}({member}<'a>),");
                }
            }
            w.close_block("}");
            w.newline();

            // node_id() method
            let _ = writeln!(w, "impl<'a> {abs_name}<'a> {{");
            w.indent();
            w.doc_comment("The typed node ID of this node.");
            let _ = writeln!(w, "pub fn node_id(&self) -> {abs_name}Id {{");
            w.indent();
            w.open_block("match self {");
            for member in members {
                if node_names.contains(member.as_str()) || list_names.contains(member.as_str()) {
                    let _ = writeln!(
                        w,
                        "{abs_name}::{member}(n) => {abs_name}Id(n.node_id().into()),"
                    );
                }
            }
            w.close_block("}");
            w.close_block("}");
            w.close_block("}");
            w.newline();

            // FromArena impl
            let _ = writeln!(w, "impl<'a> GrammarNodeType<'a> for {abs_name}<'a> {{");
            w.indent();
            w.line(
                "fn from_result(stmt_result: &'a AnyParsedStatement<'a>, id: AnyNodeId) -> Option<Self> {",
            );
            w.indent();
            w.line("let node = Node::resolve(stmt_result, id)?;");
            w.open_block("match node {");
            for member in members {
                if node_names.contains(member.as_str()) || list_names.contains(member.as_str()) {
                    let _ = writeln!(w, "Node::{member}(n) => Some({abs_name}::{member}(n)),");
                }
            }
            w.line("_ => None,");
            w.close_block("}");
            w.close_block("}");
            w.close_block("}");
            w.newline();

            // XxxId newtype for this abstract enum
            w.line("#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]");
            let _ = writeln!(w, "pub struct {abs_name}Id(AnyNodeId);");
            w.newline();
            let _ = writeln!(w, "impl {abs_name}Id {{");
            w.indent();
            w.line("pub fn into_inner(self) -> AnyNodeId { self.0 }");
            w.close_block("}");
            w.newline();
            let _ = writeln!(w, "impl<'a> From<{abs_name}<'a>> for {abs_name}Id {{");
            w.indent();
            let _ = writeln!(w, "fn from(n: {abs_name}<'a>) -> Self {{ n.node_id() }}");
            w.close_block("}");
            w.newline();
            let _ = writeln!(w, "impl From<{abs_name}Id> for AnyNodeId {{");
            w.indent();
            let _ = writeln!(w, "fn from(id: {abs_name}Id) -> AnyNodeId {{ id.0 }}");
            w.close_block("}");
            w.newline();
            let _ = writeln!(w, "impl TypedNodeId for {abs_name}Id {{");
            w.indent();
            let _ = writeln!(w, "type Node<'a> = {abs_name}<'a>;");
            w.close_block("}");
            w.newline();
        }

        // View structs — ergonomic wrappers around FFI structs
        for node in self.nodes() {
            let name = node.name;
            let fields = node.fields;

            // Struct definition
            w.line("#[derive(Clone, Copy)]");
            let _ = writeln!(w, "pub struct {name}<'a> {{");
            w.indent();
            let _ = writeln!(w, "raw: &'a {ffi_path}::{name},");
            w.line("stmt_result: &'a AnyParsedStatement<'a>,");
            w.line("id: AnyNodeId,");
            w.close_block("}");
            w.newline();

            // Debug impl — delegate to raw FFI struct
            let _ = writeln!(w, "impl std::fmt::Debug for {name}<'_> {{");
            w.indent();
            w.open_block("fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {");
            w.line("self.raw.fmt(f)");
            w.close_block("}");
            w.close_block("}");
            w.newline();

            // Display impl — delegate to AnyNode which exposes a public Display
            let _ = writeln!(w, "impl std::fmt::Display for {name}<'_> {{");
            w.indent();
            w.open_block("fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {");
            w.line("AnyNode { id: self.id, stmt_result: self.stmt_result }.fmt(f)");
            w.close_block("}");
            w.close_block("}");
            w.newline();

            // Accessor methods
            let _ = writeln!(w, "impl<'a> {name}<'a> {{");
            w.indent();
            w.doc_comment("The typed node ID of this node.");
            let _ = writeln!(
                w,
                "pub fn node_id(&self) -> {name}Id {{ {name}Id(self.id) }}"
            );
            for field in fields {
                let fname = rust_field_name(&field.name);
                let return_type =
                    rust_view_return_type(field, enum_names, flags_names, node_names, list_names);
                let body = rust_view_accessor_body(field, ffi_path);
                let _ = writeln!(w, "pub fn {fname}(&self) -> {return_type} {{");
                w.indent();
                w.line(&body);
                w.close_block("}");
            }
            w.close_block("}");
            w.newline();

            // FromArena impl — resolve from arena by NodeId (tag-checked, no unsafe)
            let _ = writeln!(w, "impl<'a> GrammarNodeType<'a> for {name}<'a> {{");
            w.indent();
            w.line(
                "fn from_result(stmt_result: &'a AnyParsedStatement<'a>, id: AnyNodeId) -> Option<Self> {",
            );
            w.indent();
            let _ = writeln!(
                w,
                "let raw = stmt_result.resolve_as::<{ffi_path}::{name}>(id)?;"
            );
            let _ = writeln!(w, "Some({name} {{ raw, stmt_result, id }})");
            w.close_block("}");
            w.close_block("}");
            w.newline();

            // XxxId newtype for this view struct
            w.line("#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]");
            let _ = writeln!(w, "pub struct {name}Id(AnyNodeId);");
            w.newline();
            let _ = writeln!(w, "impl {name}Id {{");
            w.indent();
            w.line("pub fn into_inner(self) -> AnyNodeId { self.0 }");
            w.close_block("}");
            w.newline();
            let _ = writeln!(w, "impl<'a> From<{name}<'a>> for {name}Id {{");
            w.indent();
            let _ = writeln!(w, "fn from(n: {name}<'a>) -> Self {{ n.node_id() }}");
            w.close_block("}");
            w.newline();
            let _ = writeln!(w, "impl From<{name}Id> for AnyNodeId {{");
            w.indent();
            let _ = writeln!(w, "fn from(id: {name}Id) -> AnyNodeId {{ id.0 }}");
            w.close_block("}");
            w.newline();
            let _ = writeln!(w, "impl TypedNodeId for {name}Id {{");
            w.indent();
            let _ = writeln!(w, "type Node<'a> = {name}<'a>;");
            w.close_block("}");
            w.newline();
        }

        let abstract_names: HashSet<&str> = abstract_items.iter().map(|(name, _)| *name).collect();

        // Typed list type aliases
        for list in self.lists() {
            let name = list.name;
            let child_type = list.child_type;
            let ct = child_type;
            let element_type = if node_names.contains(ct)
                || list_names.contains(ct)
                || abstract_names.contains(ct)
            {
                format!("{ct}<'a>")
            } else {
                "Node<'a>".into()
            };
            let _ = writeln!(w, "/// Typed list of `{child_type}`.");
            let _ = writeln!(
                w,
                "pub type {name}<'a> = TypedNodeList<'a, {grammar_type}, {element_type}>;"
            );
            w.newline();

            // XxxId newtype for this list alias
            w.line("#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]");
            let _ = writeln!(w, "pub struct {name}Id(AnyNodeId);");
            w.newline();
            let _ = writeln!(w, "impl {name}Id {{");
            w.indent();
            w.line("pub fn into_inner(self) -> AnyNodeId { self.0 }");
            w.close_block("}");
            w.newline();
            let _ = writeln!(w, "impl<'a> From<{name}<'a>> for {name}Id {{");
            w.indent();
            let _ = writeln!(
                w,
                "fn from(n: {name}<'a>) -> Self {{ {name}Id(n.node_id().into()) }}"
            );
            w.close_block("}");
            w.newline();
            let _ = writeln!(w, "impl From<{name}Id> for AnyNodeId {{");
            w.indent();
            let _ = writeln!(w, "fn from(id: {name}Id) -> AnyNodeId {{ id.0 }}");
            w.close_block("}");
            w.newline();
            let _ = writeln!(w, "impl TypedNodeId for {name}Id {{");
            w.indent();
            let _ = writeln!(w, "type Node<'a> = {name}<'a>;");
            w.close_block("}");
            w.newline();
        }

        // Node<'a> enum — wraps view structs
        w.doc_comment("A typed AST node. Pattern-match to access the concrete type.");
        w.line("#[derive(Debug, Clone, Copy)]");
        w.line("pub enum Node<'a> {");
        w.indent();
        for item in self.node_like_items() {
            match item {
                NodeLikeRef::Node(node) => {
                    let _ = writeln!(w, "{}({}<'a>),", node.name, node.name);
                }
                NodeLikeRef::List(list) => {
                    let _ = writeln!(w, "/// List of [`{}`].", list.child_type);
                    let _ = writeln!(w, "{}({}<'a>),", list.name, list.name);
                }
            }
        }
        if open_for_extension {
            w.doc_comment("A node with an unknown tag from a dialect extension.");
            w.line("Other { id: AnyNodeId, tag: crate::any::AnyNodeTag },");
        } else {
            w.doc_comment("Placeholder for PhantomData lifetime — never constructed.");
            w.line("__Phantom(PhantomData<&'a ()>),");
        }
        w.dedent();
        w.line("}");
        w.newline();

        w.line("impl<'a> Node<'a> {");
        w.indent();

        // from_raw
        w.doc_comment("Construct a typed `Node` from a raw arena pointer.");
        w.doc_comment("");
        w.doc_comment("# Safety");
        w.doc_comment("`ptr` must be non-null, well-aligned, and valid for `'a`.");
        w.doc_comment("Its first `u32` must be a valid `NodeTag` discriminant.");
        w.line("#[expect(clippy::too_many_lines, clippy::match_wildcard_for_single_variants)]");
        w.line("pub(crate) unsafe fn from_raw(ptr: *const u32, stmt_result: &'a AnyParsedStatement<'a>, id: AnyNodeId) -> Node<'a> {");
        w.indent();
        w.line("// SAFETY: caller guarantees ptr is valid for 'a with a valid tag.");
        w.line("unsafe {");
        w.line("let tag = NodeTag::from_raw(*ptr).unwrap_or(NodeTag::Null);");
        w.line("match tag {");
        w.indent();
        for item in self.node_like_items() {
            match item {
                NodeLikeRef::Node(node) => {
                    let name = node.name;
                    let _ = writeln!(
                        w,
                        "NodeTag::{name} => Node::{name}({name} {{ raw: &*ptr.cast::<{ffi_path}::{name}>(), stmt_result, id }}),"
                    );
                }
                NodeLikeRef::List(list) => {
                    let name = list.name;
                    let _ = writeln!(
                        w,
                        "NodeTag::{name} => Node::{name}(TypedNodeList::from_result(stmt_result, id).expect(\"list tag invariant\")),"
                    );
                }
            }
        }
        if open_for_extension {
            w.line("_ => Node::Other { id, tag: crate::any::AnyNodeTag(*ptr) },");
        } else {
            w.line("_ => unreachable!(\"unknown node tag\"),");
        }
        w.dedent();
        w.line("}");
        w.line("} // unsafe");
        w.dedent();
        w.line("}");
        w.newline();

        // resolve
        w.doc_comment("Resolve a `NodeId` into a typed `Node`, or `None` if null/invalid.");
        w.lines(
            "
        #[expect(clippy::cast_ptr_alignment)]
        pub(crate) fn resolve(stmt_result: &'a AnyParsedStatement<'a>, id: AnyNodeId) -> Option<Node<'a>> {
            let (ptr, _tag) = stmt_result.node_ptr(id)?;
            // SAFETY: node_ptr returns a valid arena pointer aligned to u32;
            // ptr is valid for 'a and its first u32 is a valid NodeTag.
            Some(unsafe { Node::from_raw(ptr.cast::<u32>(), stmt_result, id) })
        }
    ",
        );
        w.newline();

        // tag()
        emit_rust_node_tag_accessor(&mut w, self.node_like_items(), open_for_extension);

        // node_id() on Node<'a>
        w.doc_comment("The typed node ID of this node.");
        w.open_block("pub fn node_id(&self) -> NodeId {");
        w.open_block("match self {");
        for item in self.node_like_items() {
            let name = item.name();
            let _ = writeln!(w, "Node::{name}(n) => NodeId(n.node_id().into()),");
        }
        if open_for_extension {
            w.line("Node::Other { id, .. } => NodeId(*id),");
        } else {
            w.line("Node::__Phantom(_) => unreachable!(),");
        }
        w.close_block("}");
        w.close_block("}");
        w.newline();

        w.dedent();
        w.line("}");
        w.newline();

        // FromArena impl for Node
        w.lines(
            "
        impl<'a> GrammarNodeType<'a> for Node<'a> {
            fn from_result(stmt_result: &'a AnyParsedStatement<'a>, id: AnyNodeId) -> Option<Self> {
                Node::resolve(stmt_result, id)
            }
        }
    ",
        );
        w.newline();

        // NodeId — typed ID for Node<'a>
        w.line("#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]");
        w.line("pub struct NodeId(AnyNodeId);");
        w.newline();
        w.open_block("impl NodeId {");
        w.line("pub fn into_inner(self) -> AnyNodeId { self.0 }");
        w.close_block("}");
        w.newline();
        w.open_block("impl<'a> From<Node<'a>> for NodeId {");
        w.line("fn from(n: Node<'a>) -> Self { n.node_id() }");
        w.close_block("}");
        w.newline();
        w.open_block("impl From<AnyNodeId> for NodeId {");
        w.line("fn from(id: AnyNodeId) -> Self { NodeId(id) }");
        w.close_block("}");
        w.newline();
        w.open_block("impl From<NodeId> for AnyNodeId {");
        w.line("fn from(id: NodeId) -> AnyNodeId { id.0 }");
        w.close_block("}");
        w.newline();
        w.open_block("impl TypedNodeId for NodeId {");
        w.line("type Node<'a> = Node<'a>;");
        w.close_block("}");
        w.newline();

        w.finish()
    }
}
