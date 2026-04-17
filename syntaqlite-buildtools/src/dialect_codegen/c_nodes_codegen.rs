// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use std::collections::HashSet;

use crate::util::c_writer::CWriter;
use crate::util::synq_parser::{Field, Storage};
use crate::util::{pascal_to_snake, upper_snake};

use super::{AstModel, NodeLikeRef, c_type_name};

impl AstModel<'_> {
    #[expect(clippy::too_many_lines)]
    pub(crate) fn generate_ast_nodes_header(&self, dialect: &str) -> String {
        let enum_names = self.enum_names();
        let flags_names = self.flags_names();

        let mut w = CWriter::new();

        w.file_header();
        let guard = format!("SYNTAQLITE_{}_NODE_H", dialect.to_uppercase());
        w.header_guard_start(&guard);
        w.include_system("stddef.h");
        w.include_system("stdint.h");
        w.newline();
        w.include_local("syntaqlite/compiler.h");
        w.include_local("syntaqlite/types.h");
        w.newline();
        // C++ doesn't have flexible array members; use [1] as a common workaround.
        w.line("#ifdef __cplusplus");
        w.line("#define SYNTAQLITE_FLEXIBLE_ARRAY 1");
        w.line("#else");
        w.line("#define SYNTAQLITE_FLEXIBLE_ARRAY");
        w.line("#endif");
        w.newline();
        w.extern_c_start();

        // Enums
        let mut any_enum = false;
        for entry in self.enums() {
            let name = entry.name;
            let variants = entry.variants;
            if !any_enum {
                w.section("Value Enums");
                any_enum = true;
            }
            let prefix = format!("SYNTAQLITE_{}", upper_snake(name));
            let owned: Vec<_> = variants
                .iter()
                .enumerate()
                .map(|(i, v)| {
                    (
                        format!("{prefix}_{v}"),
                        Some(i32::try_from(i).expect("enum variant index fits in i32")),
                    )
                })
                .collect();
            w.typedef_enum(&c_type_name(name), &refs_i32(&owned));
            w.newline();
        }

        // Flags
        let mut any_flags = false;
        for entry in self.flags() {
            let name = entry.name;
            let flags = entry.flags;
            if !any_flags {
                w.section("Flags Types");
                any_flags = true;
            }
            let mut sorted: Vec<_> = flags.iter().collect();
            sorted.sort_by_key(|(_, v)| *v);
            let bits: Vec<_> = sorted.iter().map(|(n, v)| (n.to_lowercase(), *v)).collect();
            let bit_refs: Vec<_> = bits.iter().map(|(n, v)| (n.as_str(), *v)).collect();
            w.typedef_flags_union(&c_type_name(name), &bit_refs);
            w.newline();
        }

        // Node tags
        w.section("Node Tags");
        let mut tag_variants: Vec<(String, Option<i32>)> =
            vec![("SYNTAQLITE_NODE_NULL".into(), Some(0))];
        for item in self.node_like_items() {
            let name = item.name();
            tag_variants.push((tag_name(name), Some(self.tag_for(name).cast_signed())));
        }
        tag_variants.push(("SYNTAQLITE_NODE_COUNT".into(), None));
        w.typedef_enum("SyntaqliteNodeTag", &refs_i32(&tag_variants));
        w.line("SYNQ_STATIC_ASSERT(sizeof(SyntaqliteNodeTag) == sizeof(uint32_t),");
        w.line("                   \"SyntaqliteNodeTag must be 32 bits for FFI compatibility\");");
        w.newline();

        // Node structs
        w.section("Node Structs");
        for item in self.node_like_items() {
            match item {
                NodeLikeRef::Node(node) => {
                    let sname = c_type_name(node.name);
                    let mut f = vec![("SyntaqliteNodeTag".to_string(), "tag".to_string())];
                    for field in node.fields {
                        f.push((
                            field_c_type(field, enum_names, flags_names),
                            field.name.clone(),
                        ));
                    }
                    let refs: Vec<_> = f.iter().map(|(t, n)| (t.as_str(), n.as_str())).collect();
                    w.typedef_struct(&sname, &refs);
                    w.newline();
                }
                NodeLikeRef::List(list) => {
                    w.comment(&format!("List of {}", list.child_type));
                    w.typedef_list_struct(&c_type_name(list.name));
                    w.newline();
                }
            }
        }

        // Node union
        w.section("Node Union");
        let mut union_members = vec![("SyntaqliteNodeTag".to_string(), "tag".to_string())];
        for item in self.node_like_items() {
            let name = item.name();
            union_members.push((c_type_name(name), pascal_to_snake(name)));
        }
        let union_refs: Vec<_> = union_members
            .iter()
            .map(|(t, n)| (t.as_str(), n.as_str()))
            .collect();
        w.typedef_union("SyntaqliteNode", &union_refs);
        w.newline();

        // Abstract type unions and accessors
        for &(name, members) in self.abstract_items() {
            w.section(&format!("Abstract Type: {name}"));

            let c_abs_name = c_type_name(name);
            let mut abs_union_members = vec![("SyntaqliteNodeTag".to_string(), "tag".to_string())];
            for member in members {
                abs_union_members.push((c_type_name(member), pascal_to_snake(member)));
            }
            let abs_refs: Vec<_> = abs_union_members
                .iter()
                .map(|(t, n)| (t.as_str(), n.as_str()))
                .collect();
            w.typedef_union(&c_abs_name, &abs_refs);
            w.newline();

            let check_fn = format!("syntaqlite_is_{}", pascal_to_snake(name));
            w.line(&format!(
                "static inline int {check_fn}(SyntaqliteNodeTag tag) {{"
            ));
            w.indent();
            w.line("switch (tag) {");
            w.indent();
            for member in members {
                w.line(&format!("case {}: return 1;", tag_name(member)));
            }
            w.line("default: return 0;");
            w.dedent();
            w.line("}");
            w.dedent();
            w.line("}");
            w.newline();

            for member in members {
                let accessor_fn = format!(
                    "syntaqlite_{}_as_{}",
                    pascal_to_snake(name),
                    pascal_to_snake(member)
                );
                let member_type = c_type_name(member);
                w.line(&format!(
                    "static inline const {member_type}* {accessor_fn}(const {c_abs_name}* node) {{"
                ));
                w.indent();
                w.line(&format!(
                    "return node->tag == {} ? &node->{} : NULL;",
                    tag_name(member),
                    pascal_to_snake(member)
                ));
                w.dedent();
                w.line("}");
                w.newline();
            }
        }

        w.extern_c_end();
        w.newline();

        // C++ NodeTag specializations (tag-checked NodeCast support)
        w.line("#if defined(__cplusplus) && __cplusplus >= 201703L");
        w.line("#include \"syntaqlite/parser.h\"");
        w.newline();
        w.line("namespace syntaqlite {");
        w.newline();
        w.line("template <typename T> struct NodeTag { static constexpr bool kHasTag = false; };");
        w.newline();
        for item in self.node_like_items() {
            let name = item.name();
            let cname = c_type_name(name);
            let tag = tag_name(name);
            w.line(&format!("template <> struct NodeTag<{cname}> {{"));
            w.line("  static constexpr bool kHasTag = true;");
            w.line(&format!("  static constexpr uint32_t kValue = {tag};"));
            w.line("};");
        }
        w.newline();
        w.line("}  // namespace syntaqlite");
        w.line("#endif");
        w.newline();
        w.header_guard_end(&guard);

        w.finish()
    }

    pub(crate) fn generate_ast_builder_header(&self, dialect: &str) -> String {
        let enum_names = self.enum_names();
        let flags_names = self.flags_names();

        let mut w = CWriter::new();

        let guard = format!("SYNTAQLITE_{}_DIALECT_BUILDER_H", dialect.to_uppercase());
        w.file_header();
        w.header_guard_start(&guard);
        w.include_local("syntaqlite_dialect/ast_builder.h");
        w.include_local("syntaqlite_dialect/dialect_types.h");
        w.include_local(&format!("syntaqlite_{dialect}/{dialect}_node.h"));
        w.newline();
        w.extern_c_start();

        w.section("Builder Functions");

        for node in self.nodes() {
            emit_node_builder_inline(&mut w, node.name, node.fields, enum_names, flags_names);
        }
        for list in self.lists() {
            emit_list_builder_inline(&mut w, list.name, list.prepend);
        }

        w.extern_c_end();
        w.newline();
        w.header_guard_end(&guard);

        w.finish()
    }
}

fn emit_node_builder_inline(
    w: &mut CWriter,
    name: &str,
    fields: &[Field],
    enum_names: &HashSet<&str>,
    flags_names: &HashSet<&str>,
) {
    let sn = c_type_name(name);
    let tag = tag_name(name);
    let func = builder_name(name);

    let mut param_strs = vec!["SynqParseCtx *ctx".to_string()];
    for field in fields {
        param_strs.push(format!(
            "{} {}",
            field_c_type(field, enum_names, flags_names),
            field.name
        ));
    }
    let params: Vec<&str> = param_strs.iter().map(String::as_str).collect();
    w.func_signature("static inline ", "uint32_t", &func, &params, " {");

    let mut init_parts = vec![format!(".tag = {}", tag)];
    for field in fields {
        init_parts.push(format!(".{} = {}", field.name, field.name));
    }

    let literal = format!("&({}){{{}}}", sn, init_parts.join(", "));
    let call = format!("return synq_parse_build(ctx, {literal}, (uint32_t)sizeof({sn}));");

    w.indent();
    if call.len() <= 80 {
        w.line(&call);
    } else {
        w.line("return synq_parse_build(ctx,");
        w.indent();
        w.line(&format!("&({sn}){{"));
        w.indent();
        for (i, part) in init_parts.iter().enumerate() {
            let comma = if i < init_parts.len() - 1 { "," } else { "" };
            w.line(&format!("{part}{comma}"));
        }
        w.dedent();
        w.line(&format!("}}, (uint32_t)sizeof({sn}));"));
        w.dedent();
    }
    w.dedent();
    w.line("}");
    w.newline();
}

fn emit_list_builder_inline(w: &mut CWriter, name: &str, prepend: bool) {
    let func = builder_name(name);
    let tag = tag_name(name);
    let builder_fn = if prepend {
        "synq_parse_list_prepend"
    } else {
        "synq_parse_list_append"
    };

    w.func_signature(
        "static inline ",
        "uint32_t",
        &func,
        &["SynqParseCtx *ctx", "uint32_t list_id", "uint32_t child"],
        " {",
    );
    w.indent();
    w.line(&format!("return {builder_fn}(ctx, {tag}, list_id, child);"));
    w.dedent();
    w.line("}");
    w.newline();
}

fn tag_name(name: &str) -> String {
    format!("SYNTAQLITE_NODE_{}", upper_snake(name))
}

fn builder_name(name: &str) -> String {
    format!("synq_parse_{}", pascal_to_snake(name))
}

fn field_c_type(field: &Field, enum_names: &HashSet<&str>, flags_names: &HashSet<&str>) -> String {
    match field.storage {
        Storage::Index => "uint32_t".into(),
        Storage::Inline => {
            let t = &field.type_name;
            if enum_names.contains(t.as_str()) || flags_names.contains(t.as_str()) {
                c_type_name(t)
            } else {
                t.clone()
            }
        }
    }
}

fn refs_i32(owned: &[(String, Option<i32>)]) -> Vec<(&str, Option<i32>)> {
    owned.iter().map(|(s, v)| (s.as_str(), *v)).collect()
}

#[cfg(test)]
mod tests {
    use super::super::AstModel;
    use crate::util::synq_parser::parse_synq_file;

    #[test]
    fn node_tag_size_is_statically_asserted() {
        let items = parse_synq_file("node Foo { x: inline Bool }").expect("parse");
        let model = AstModel::new(&items);
        let header = model.generate_ast_nodes_header("sqlite");
        assert!(
            header.contains("SYNQ_STATIC_ASSERT(sizeof(SyntaqliteNodeTag) == sizeof(uint32_t),"),
            "expected node tag size assertion, got:\n{header}"
        );
    }
}
