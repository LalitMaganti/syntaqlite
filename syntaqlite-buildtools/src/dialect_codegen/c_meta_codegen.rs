// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use std::collections::HashSet;
use std::fmt::{Display, Formatter};

use crate::util::c_writer::CWriter;
use crate::util::pascal_to_snake;
use crate::util::synq_parser::{Field, Storage};

use super::{AstModel, NodeLikeRef, c_type_name};

#[derive(Debug, Clone)]
pub(crate) enum CMetaCodegenError {
    UnknownInlineType(String),
}

impl Display for CMetaCodegenError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::UnknownInlineType(t) => {
                write!(f, "unknown inline type for C field meta codegen: {t}")
            }
        }
    }
}

impl std::error::Error for CMetaCodegenError {}

#[derive(Debug, Clone)]
pub(crate) enum CFmtCodegenError {
    FmtCompile(super::fmt_compiler::FmtCompileError),
}

impl Display for CFmtCodegenError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::FmtCompile(err) => Display::fmt(err, f),
        }
    }
}

impl std::error::Error for CFmtCodegenError {}

impl AstModel<'_> {
    /// Generate a C header containing all formatter tables for `dialect`.
    ///
    /// Strings are CSR-encoded: a flat `uint8_t` byte buffer plus a `uint32_t`
    /// offsets array with N+1 entries (offsets[N] = total byte count).  All
    /// symbol names are prefixed with `{dialect}_fmt_` so that `dialect.c` can
    /// re-export them as non-static `syntaqlite_{dialect}_fmt_*()` functions.
    #[expect(clippy::too_many_lines)]
    pub(crate) fn generate_c_fmt_tables(&self, dialect: &str) -> Result<String, CFmtCodegenError> {
        let compiled =
            super::fmt_compiler::try_compile_all(self).map_err(CFmtCodegenError::FmtCompile)?;

        let upper = dialect.to_uppercase();
        let guard = format!("SYNTAQLITE_{upper}_DIALECT_FMT_H");
        let p = format!("{dialect}_fmt"); // symbol prefix

        let mut w = CWriter::new();
        w.file_header();
        w.header_guard_start(&guard);
        w.include_system("stdint.h");
        w.newline();

        // ── String table (CSR encoding) ──────────────────────────────────────
        // Build flat byte buffer and N+1 offset array.
        let mut str_data: Vec<u8> = Vec::new();
        let mut str_offsets: Vec<u32> = Vec::new();
        for s in &compiled.strings {
            str_offsets.push(u32::try_from(str_data.len()).expect("string table fits u32"));
            str_data.extend_from_slice(s.as_bytes());
        }
        str_offsets.push(u32::try_from(str_data.len()).expect("string table fits u32")); // sentinel: offsets[N] = total length
        let str_count = compiled.strings.len();

        w.line(&format!("static const uint8_t {p}_string_data[] = {{"));
        for chunk in str_data.chunks(16) {
            let vals: Vec<String> = chunk.iter().map(|b| format!("0x{b:02x}")).collect();
            w.line(&format!("    {},", vals.join(",")));
        }
        w.line("};");
        w.newline();

        w.line(&format!("static const uint32_t {p}_string_offsets[] = {{"));
        for chunk in str_offsets.chunks(16) {
            let vals: Vec<String> = chunk.iter().map(|v| format!("{v}")).collect();
            w.line(&format!("    {},", vals.join(",")));
        }
        w.line("};");
        w.newline();
        w.line(&format!(
            "static const uint32_t {p}_string_count = {str_count};"
        ));
        w.newline();

        // ── Enum display table ───────────────────────────────────────────────
        let edlen = compiled.enum_display.len();
        w.line(&format!("static const uint16_t {p}_enum_display[] = {{"));
        for chunk in compiled.enum_display.chunks(16) {
            let vals: Vec<String> = chunk.iter().map(|v| format!("{v}")).collect();
            w.line(&format!("    {},", vals.join(",")));
        }
        w.line("};");
        w.newline();
        w.line(&format!(
            "static const uint32_t {p}_enum_display_count = {edlen};"
        ));
        w.newline();

        // ── Ops pool ─────────────────────────────────────────────────────────
        let mut op_pool: Vec<syntaqlite_common::fmt::bytecode::RawOp> = Vec::new();
        let mut node_ranges: Vec<(&str, u16, u16)> = Vec::new();
        for cn in &compiled.nodes {
            let offset = u16::try_from(op_pool.len()).expect("fmt op pool offset fits in u16");
            let length = u16::try_from(cn.ops.len()).expect("fmt op count fits in u16");
            op_pool.extend_from_slice(&cn.ops);
            node_ranges.push((&cn.name, offset, length));
        }
        let opslen = op_pool.len() * 6; // each RawOp = 6 bytes

        w.line(&format!("static const uint8_t {p}_ops[] = {{"));
        for op in &op_pool {
            let b_bytes = op.b.to_le_bytes();
            let c_bytes = op.c.to_le_bytes();
            w.line(&format!(
                "    {},{},{},{},{},{},",
                op.opcode, op.a, b_bytes[0], b_bytes[1], c_bytes[0], c_bytes[1],
            ));
        }
        w.line("};");
        w.newline();
        w.line(&format!("static const uint32_t {p}_ops_count = {opslen};"));
        w.newline();

        // ── Dispatch table ────────────────────────────────────────────────────
        let mut dispatch_table: Vec<u32> = vec![0xFFFF_0000; compiled.tag_count];
        let mut ordinal_map: std::collections::HashMap<&str, usize> =
            std::collections::HashMap::new();
        let mut next_ordinal = 1usize;
        for item in self.node_like_items() {
            ordinal_map.insert(item.name(), next_ordinal);
            next_ordinal += 1;
        }
        for &(name, offset, length) in &node_ranges {
            if let Some(&ordinal) = ordinal_map.get(name) {
                dispatch_table[ordinal] = (u32::from(offset) << 16) | u32::from(length);
            }
        }
        let displen = dispatch_table.len();

        w.line(&format!("static const uint32_t {p}_dispatch[] = {{"));
        for chunk in dispatch_table.chunks(8) {
            let vals: Vec<String> = chunk.iter().map(|v| format!("0x{v:08x}")).collect();
            w.line(&format!("    {},", vals.join(",")));
        }
        w.line("};");
        w.newline();
        w.line(&format!(
            "static const uint32_t {p}_dispatch_count = {displen};"
        ));
        w.newline();

        // ── Precedence table ────────────────────────────────────────────────
        let ptlen = compiled.prec_table.len();
        if ptlen > 0 {
            w.line(&format!("static const uint8_t {p}_prec_table[] = {{"));
            for chunk in compiled.prec_table.chunks(16) {
                let vals: Vec<String> = chunk.iter().map(|b| format!("{b}")).collect();
                w.line(&format!("    {},", vals.join(",")));
            }
            w.line("};");
        } else {
            w.line(&format!("static const uint8_t {p}_prec_table[] = {{0}};"));
        }
        w.newline();
        w.line(&format!(
            "static const uint32_t {p}_prec_table_count = {ptlen};"
        ));
        w.newline();

        // ── Expr-meta per tag ───────────────────────────────────────────────
        let expr_meta_len = compiled.expr_meta.len();
        w.line(&format!("static const uint32_t {p}_expr_meta[] = {{"));
        for chunk in compiled.expr_meta.chunks(8) {
            let vals: Vec<String> = chunk.iter().map(|v| format!("0x{v:08x}")).collect();
            w.line(&format!("    {},", vals.join(",")));
        }
        w.line("};");
        w.newline();
        w.line(&format!(
            "static const uint32_t {p}_expr_meta_count = {expr_meta_len};"
        ));
        w.newline();

        w.header_guard_end(&guard);
        Ok(w.finish())
    }

    #[expect(clippy::too_many_lines)]
    pub(crate) fn generate_c_field_metadata(
        &self,
        dialect: &str,
    ) -> Result<String, CMetaCodegenError> {
        let enum_names = self.enum_names();
        let flags_names = self.flags_names();

        let mut w = CWriter::new();
        w.file_header();
        w.header_guard_start("SYNTAQLITE_DIALECT_META_H");
        w.include_system("stddef.h");
        w.include_local("syntaqlite/grammar.h");
        w.include_local("syntaqlite_dialect/dialect_types.h");
        w.include_local(&format!("syntaqlite_{dialect}/{dialect}_node.h"));
        w.newline();

        for item in self.enums() {
            let name = item.name;
            let variants = item.variants;
            let var = format!("display_{}", pascal_to_snake(name));
            w.line(&format!("static const char* const {var}[] = {{"));
            w.indent();
            for v in variants {
                w.line(&format!("\"{v}\","));
            }
            w.dedent();
            w.line("};");
            w.newline();
        }

        for item in self.flags() {
            let name = item.name;
            let flags = item.flags;
            let max_bit_pos = flags
                .iter()
                .map(|(_, v)| bit_position(*v))
                .max()
                .unwrap_or(0);
            let var = format!("display_{}", pascal_to_snake(name));
            w.line(&format!("static const char* const {var}[] = {{"));
            w.indent();
            for pos in 0..=max_bit_pos {
                let label = flags
                    .iter()
                    .find(|(_, v)| bit_position(*v) == pos)
                    .map_or("", |(n, _)| n.as_str());
                w.line(&format!("\"{label}\","));
            }
            w.dedent();
            w.line("};");
            w.newline();
        }

        for node in self.nodes() {
            let name = node.name;
            let fields = node.fields;
            if fields.is_empty() {
                continue;
            }
            let sn = c_type_name(name);
            let var = format!("field_meta_{}", pascal_to_snake(name));
            w.line(&format!("static const SyntaqliteFieldMeta {var}[] = {{"));
            w.indent();
            for field in fields {
                let kind = c_field_kind(field, enum_names, flags_names)?;
                let (display, display_count) = c_field_display(field, enum_names, flags_names);
                w.line(&format!(
                    "{{offsetof({}, {}), {}, \"{}\", {}, {}}},",
                    sn, field.name, kind, field.name, display, display_count,
                ));
            }
            w.dedent();
            w.line("};");
            w.newline();
        }

        for node in self.nodes() {
            let name = node.name;
            let span_fields: Vec<_> = node
                .fields
                .iter()
                .filter(|f| f.storage == Storage::Inline && f.type_name == "SyntaqliteSourceSpan")
                .collect();
            if span_fields.is_empty() {
                continue;
            }
            let sn = c_type_name(name);
            let var = format!("range_meta_{}", pascal_to_snake(name));
            w.line(&format!(
                "static const SyntaqliteFieldRangeMeta {var}[] = {{"
            ));
            w.indent();
            for field in &span_fields {
                w.line(&format!("{{offsetof({sn}, {}), 1}},", field.name));
            }
            w.dedent();
            w.line("};");
            w.newline();
        }

        w.section("Node Names");
        w.line("static const char* const ast_meta_node_names[] = {");
        w.indent();
        w.line("\"Null\",");
        for item in self.node_like_items() {
            w.line(&format!("\"{}\",", item.name()));
        }
        w.dedent();
        w.line("};");
        w.newline();

        // Build field_meta, field_meta_counts, list_tags, and range_meta arrays in a single pass.
        let mut field_meta_entries = vec!["NULL, /* Null */".to_string()];
        let mut field_count_entries = vec!["0, /* Null */".to_string()];
        let mut list_tag_entries = vec!["0, /* Null */".to_string()];
        let mut range_meta_entries = vec!["{NULL, 0}, /* Null */".to_string()];

        for item in self.node_like_items() {
            match item {
                NodeLikeRef::Node(node) => {
                    if node.fields.is_empty() {
                        field_meta_entries.push(format!("NULL, /* {} */", node.name));
                    } else {
                        field_meta_entries.push(format!(
                            "field_meta_{}, /* {} */",
                            pascal_to_snake(node.name),
                            node.name
                        ));
                    }
                    field_count_entries.push(format!("{}, /* {} */", node.fields.len(), node.name));
                    list_tag_entries.push(format!("0, /* {} */", node.name));
                    let span_count = node
                        .fields
                        .iter()
                        .filter(|f| {
                            f.storage == Storage::Inline && f.type_name == "SyntaqliteSourceSpan"
                        })
                        .count();
                    if span_count > 0 {
                        range_meta_entries.push(format!(
                            "{{range_meta_{}, {}}}, /* {} */",
                            pascal_to_snake(node.name),
                            span_count,
                            node.name
                        ));
                    } else {
                        range_meta_entries.push(format!("{{NULL, 0}}, /* {} */", node.name));
                    }
                }
                NodeLikeRef::List(list) => {
                    field_meta_entries.push(format!("NULL, /* {} */", list.name));
                    field_count_entries.push(format!("0, /* {} */", list.name));
                    list_tag_entries.push(format!("1, /* {} */", list.name));
                    range_meta_entries.push(format!("{{NULL, 0}}, /* {} */", list.name));
                }
            }
        }

        w.section("Field Meta Dispatch");
        w.line("static const SyntaqliteFieldMeta* const ast_meta_field_meta[] = {");
        w.indent();
        for entry in &field_meta_entries {
            w.line(entry);
        }
        w.dedent();
        w.line("};");
        w.newline();

        w.line("static const uint8_t ast_meta_field_meta_counts[] = {");
        w.indent();
        for entry in &field_count_entries {
            w.line(entry);
        }
        w.dedent();
        w.line("};");
        w.newline();

        w.section("List Tags");
        w.line("static const uint8_t ast_meta_list_tags[] = {");
        w.indent();
        for entry in &list_tag_entries {
            w.line(entry);
        }
        w.dedent();
        w.line("};");
        w.newline();

        w.section("Range Meta Dispatch");
        w.line("static const SyntaqliteRangeMetaEntry ast_meta_range_meta[] = {");
        w.indent();
        for entry in &range_meta_entries {
            w.line(entry);
        }
        w.dedent();
        w.line("};");
        w.newline();

        w.header_guard_end("SYNTAQLITE_DIALECT_META_H");
        Ok(w.finish())
    }
}

fn c_field_kind(
    field: &Field,
    enum_names: &HashSet<&str>,
    flags_names: &HashSet<&str>,
) -> Result<&'static str, CMetaCodegenError> {
    match field.storage {
        Storage::Index => Ok("SYNTAQLITE_FIELD_NODE_ID"),
        Storage::Inline => {
            let t = &field.type_name;
            if t == "SyntaqliteSourceSpan" {
                Ok("SYNTAQLITE_FIELD_SPAN")
            } else if t == "Bool" {
                Ok("SYNTAQLITE_FIELD_BOOL")
            } else if flags_names.contains(t.as_str()) {
                Ok("SYNTAQLITE_FIELD_FLAGS")
            } else if enum_names.contains(t.as_str()) {
                Ok("SYNTAQLITE_FIELD_ENUM")
            } else {
                Err(CMetaCodegenError::UnknownInlineType(t.clone()))
            }
        }
    }
}

fn c_field_display(
    field: &Field,
    enum_names: &HashSet<&str>,
    flags_names: &HashSet<&str>,
) -> (String, String) {
    match field.storage {
        Storage::Inline => {
            let t = &field.type_name;
            if enum_names.contains(t.as_str()) || flags_names.contains(t.as_str()) {
                let var = format!("display_{}", pascal_to_snake(t));
                let count = format!("sizeof({var}) / sizeof({var}[0])");
                (var, count)
            } else {
                ("NULL".into(), "0".into())
            }
        }
        Storage::Index => ("NULL".into(), "0".into()),
    }
}

const fn bit_position(value: u32) -> u32 {
    if value == 0 {
        return 0;
    }
    value.trailing_zeros()
}
