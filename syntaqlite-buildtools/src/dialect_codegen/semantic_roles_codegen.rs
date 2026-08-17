// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Codegen for the semantic role byte table.
//!
//! `generate_c_roles_h` emits a C header containing the role data as a flat
//! `uint8_t` byte array.  The layout matches `SemanticRole`'s `#[repr(C, u8)]`
//! representation exactly (discriminant in byte 0, payload bytes in bytes 1–N,
//! zero-padded to `size_of::<SemanticRole>()` = 8 bytes per entry).
//!
//! The header is included by `dialect.c`, which re-exports the data through
//! two non-static C symbols so Rust can link to them:
//!
//! ```c
//! const uint8_t *syntaqlite_<name>_roles_data(void);
//! uint32_t       syntaqlite_<name>_roles_count(void);
//! ```
//!
//! On the Rust side, `TypedDialect::roles()` casts the pointer directly to
//! `&[SemanticRole]` — zero-decode cost.

use std::mem::size_of;

use syntaqlite_common::roles::{FIELD_ABSENT, FlagSpec, MacroDef, RelationKind, SemanticRole};

use super::AstModel;
use crate::util::c_writer::CWriter;
use crate::util::synq_parser::{Field, SemanticRole as SynqRole};

const ROLE_SIZE: usize = size_of::<SemanticRole>();

/// Return the 0-based index of `field_name` in the node's field list.
fn field_index(fields: &[Field], field_name: &str) -> u8 {
    let idx = fields
        .iter()
        .position(|f| f.name == field_name)
        .unwrap_or_else(|| panic!("field '{field_name}' not found in field list"));
    u8::try_from(idx).expect("field index fits u8")
}

/// Construct the raw bytes for a `SemanticRole` in a deterministic way:
/// byte 0 is the discriminant (read from a Rust-constructed value of that
/// variant), bytes 1–N are the payload fields explicitly, and all remaining
/// bytes are zero (no undefined padding sneaking in).
/// Resolve a dotted flag reference `(field_name, bit_name)` to a `(field_index, bit_mask)` pair.
///
/// Uses the field list to find the field's type name, then looks up the bit
/// mask from the flags type definitions in the model.
fn resolve_flag_spec(
    fields: &[Field],
    flag_ref: Option<&(String, String)>,
    model: &AstModel<'_>,
) -> (u8, u8) {
    let Some((field_name, bit_name)) = flag_ref else {
        return (FIELD_ABSENT, 0);
    };
    let field_idx = field_index(fields, field_name);
    let field = &fields[field_idx as usize];
    let flags_type = model
        .flags()
        .iter()
        .find(|f| f.name == field.type_name)
        .unwrap_or_else(|| panic!("flags type '{}' not found", field.type_name));
    let mask = flags_type
        .flags
        .iter()
        .find(|(name, _)| name.eq_ignore_ascii_case(bit_name))
        .unwrap_or_else(|| panic!("flag bit '{bit_name}' not found in '{}'", field.type_name))
        .1;
    (field_idx, u8::try_from(mask).expect("flag mask fits u8"))
}

#[expect(
    clippy::too_many_lines,
    reason = "large match over all SemanticRole variants; not worth splitting"
)]
fn role_to_bytes(
    fields: &[Field],
    synq_role: Option<&SynqRole>,
    model: &AstModel<'_>,
) -> [u8; ROLE_SIZE] {
    let fi = |name: &str| field_index(fields, name);
    let opt = |name: &Option<String>| name.as_ref().map_or(FIELD_ABSENT, |n| fi(n));

    // Helper: read the discriminant from a constructed SemanticRole value.
    // With #[repr(C, u8)], byte 0 is always the discriminant tag.
    let disc = |role: SemanticRole| -> u8 {
        // SAFETY: #[repr(C, u8)] guarantees byte 0 is the discriminant.
        unsafe { *(&raw const role).cast::<u8>() }
    };

    let mut bytes = [0u8; ROLE_SIZE];

    let Some(role) = synq_role else {
        // No annotation → Transparent.
        bytes[0] = disc(SemanticRole::Transparent);
        return bytes;
    };

    match role {
        // ── Catalog roles ────────────────────────────────────────────────
        SynqRole::DefineTable {
            name,
            columns,
            select,
            without_rowid,
        } => {
            bytes[0] = disc(SemanticRole::DefineTable {
                name: 0,
                columns: 0,
                select: 0,
                without_rowid: FlagSpec::ABSENT,
            });
            bytes[1] = fi(name);
            bytes[2] = opt(columns);
            bytes[3] = opt(select);
            let (flag_field, flag_mask) = resolve_flag_spec(fields, without_rowid.as_ref(), model);
            bytes[4] = flag_field;
            bytes[5] = flag_mask;
        }
        SynqRole::DefineView {
            name,
            columns,
            select,
        } => {
            bytes[0] = disc(SemanticRole::DefineView {
                name: 0,
                columns: 0,
                select: 0,
            });
            bytes[1] = fi(name);
            bytes[2] = opt(columns);
            bytes[3] = fi(select);
        }
        SynqRole::DefineFunction {
            name,
            args,
            return_type,
            select,
        } => {
            bytes[0] = disc(SemanticRole::DefineFunction {
                name: 0,
                args: 0,
                return_type: 0,
                select: 0,
            });
            bytes[1] = fi(name);
            bytes[2] = opt(args);
            bytes[3] = opt(return_type);
            bytes[4] = opt(select);
        }
        SynqRole::ReturnSpec { columns } => {
            bytes[0] = disc(SemanticRole::ReturnSpec { columns: 0 });
            bytes[1] = opt(columns);
        }
        SynqRole::Import { module } => {
            bytes[0] = disc(SemanticRole::Import { module: 0 });
            bytes[1] = fi(module);
        }
        // ── Column-list items ─────────────────────────────────────────────
        SynqRole::ColumnDef {
            name,
            type_name,
            constraints,
        } => {
            bytes[0] = disc(SemanticRole::ColumnDef {
                name: 0,
                type_: 0,
                constraints: 0,
            });
            bytes[1] = fi(name);
            bytes[2] = opt(type_name);
            bytes[3] = opt(constraints);
        }
        // ── Result columns ────────────────────────────────────────────────
        SynqRole::ResultColumn { flags, alias, expr } => {
            bytes[0] = disc(SemanticRole::ResultColumn {
                flags: 0,
                alias: 0,
                expr: 0,
            });
            bytes[1] = fi(flags);
            bytes[2] = fi(alias);
            bytes[3] = fi(expr);
        }
        // ── Expressions ───────────────────────────────────────────────────
        SynqRole::Call { name, args } => {
            bytes[0] = disc(SemanticRole::Call { name: 0, args: 0 });
            bytes[1] = fi(name);
            bytes[2] = fi(args);
        }
        SynqRole::ColumnRef { column, table } => {
            bytes[0] = disc(SemanticRole::ColumnRef {
                column: 0,
                table: 0,
            });
            bytes[1] = fi(column);
            bytes[2] = fi(table);
        }
        // ── Sources ───────────────────────────────────────────────────────
        SynqRole::SourceRef { kind, name, alias } => {
            bytes[0] = disc(SemanticRole::SourceRef {
                kind: RelationKind::Table,
                name: 0,
                alias: 0,
            });
            bytes[1] = match kind.as_str() {
                "table" => RelationKind::Table as u8,
                "view" => RelationKind::View as u8,
                "interval" => RelationKind::Interval as u8,
                "tree" => RelationKind::Tree as u8,
                "graph" => RelationKind::Graph as u8,
                other => panic!("unknown RelationKind literal '{other}' in source_ref"),
            };
            bytes[2] = fi(name);
            bytes[3] = fi(alias);
        }
        SynqRole::ScopedSource { body, alias } => {
            bytes[0] = disc(SemanticRole::ScopedSource { body: 0, alias: 0 });
            bytes[1] = fi(body);
            bytes[2] = fi(alias);
        }
        // ── Scope structure ───────────────────────────────────────────────
        SynqRole::Query {
            from,
            columns,
            where_clause,
            groupby,
            having,
            orderby,
            limit_clause,
        } => {
            bytes[0] = disc(SemanticRole::Query {
                from: 0,
                columns: 0,
                where_clause: 0,
                groupby: 0,
                having: 0,
                orderby: 0,
                limit_clause: 0,
            });
            bytes[1] = fi(from);
            bytes[2] = fi(columns);
            bytes[3] = fi(where_clause);
            bytes[4] = fi(groupby);
            bytes[5] = fi(having);
            bytes[6] = fi(orderby);
            bytes[7] = fi(limit_clause);
        }
        SynqRole::CteBinding {
            name,
            columns,
            body,
        } => {
            bytes[0] = disc(SemanticRole::CteBinding {
                name: 0,
                columns: 0,
                body: 0,
            });
            bytes[1] = fi(name);
            bytes[2] = opt(columns);
            bytes[3] = fi(body);
        }
        SynqRole::CteScope {
            recursive,
            bindings,
            body,
        } => {
            bytes[0] = disc(SemanticRole::CteScope {
                recursive: 0,
                bindings: 0,
                body: 0,
            });
            bytes[1] = fi(recursive);
            bytes[2] = fi(bindings);
            bytes[3] = fi(body);
        }
        SynqRole::TriggerScope { target, when, body } => {
            bytes[0] = disc(SemanticRole::TriggerScope {
                target: 0,
                when: 0,
                body: 0,
            });
            bytes[1] = fi(target);
            bytes[2] = fi(when);
            bytes[3] = fi(body);
        }
        SynqRole::DmlScope {
            with_recursive,
            with_ctes,
        } => {
            bytes[0] = disc(SemanticRole::DmlScope {
                with_recursive: 0,
                with_ctes: 0,
            });
            bytes[1] = opt(with_recursive);
            bytes[2] = opt(with_ctes);
        }
        SynqRole::UpsertScope {
            columns,
            target_where,
            setlist,
            update_where,
        } => {
            bytes[0] = disc(SemanticRole::UpsertScope {
                columns: 0,
                target_where: 0,
                setlist: 0,
                update_where: 0,
            });
            bytes[1] = opt(columns);
            bytes[2] = opt(target_where);
            bytes[3] = opt(setlist);
            bytes[4] = opt(update_where);
        }
        SynqRole::CompoundScope {
            left,
            right,
            orderby,
            limit_clause,
        } => {
            bytes[0] = disc(SemanticRole::CompoundScope {
                left: 0,
                right: 0,
                orderby: 0,
                limit_clause: 0,
            });
            bytes[1] = fi(left);
            bytes[2] = fi(right);
            bytes[3] = opt(orderby);
            bytes[4] = opt(limit_clause);
        }
        SynqRole::AlterTable {
            op,
            target,
            new_name,
            old_name,
            column,
        } => {
            bytes[0] = disc(SemanticRole::AlterTable {
                op: 0,
                target: 0,
                new_name: 0,
                old_name: 0,
                column: 0,
            });
            bytes[1] = fi(op);
            bytes[2] = fi(target);
            bytes[3] = fi(new_name);
            bytes[4] = fi(old_name);
            bytes[5] = opt(column);
        }
    }

    bytes
}

/// Generate a C header file containing the semantic role byte array for a dialect.
///
/// The output defines:
/// - `static const uint8_t <prefix>_roles_data[N * ROLE_SIZE]` — packed role bytes,
/// - `static const uint32_t <prefix>_roles_count` — number of `SemanticRole` entries.
///
/// `prefix` should be lowercase (e.g. `"sqlite"`), matching the dialect name.
/// The C symbols `syntaqlite_<prefix>_roles_data()` and
/// `syntaqlite_<prefix>_roles_count()` are exposed in `dialect.c`.
pub(crate) fn generate_c_roles_h(model: &AstModel, prefix: &str) -> String {
    let upper = prefix.to_uppercase();
    let guard = format!("SYNTAQLITE_{upper}_DIALECT_ROLES_H");

    let mut w = CWriter::new();
    w.file_header();
    w.header_guard_start(&guard);
    w.include_system("stdint.h");
    w.newline();
    w.line(&format!(
        "/* Semantic role byte array for the {prefix} dialect. */\n\
         /* Each entry is {ROLE_SIZE} bytes: 1 discriminant + up to 7 payload bytes. */"
    ));
    w.newline();

    // Collect all entries.
    let mut all_bytes: Vec<u8> = Vec::new();
    let mut count: u32 = 0;

    // Entry 0 — unused sentinel (Transparent).
    let transparent_bytes = role_to_bytes(&[], None, model);
    all_bytes.extend_from_slice(&transparent_bytes);
    count += 1;

    for node_like in model.node_like_items() {
        use super::NodeLikeRef;
        let (fields, synq_role) = match node_like {
            NodeLikeRef::Node(n) => (n.fields, n.semantic.map(|s| &s.role)),
            NodeLikeRef::List(_) => (&[][..], None),
        };
        let entry = role_to_bytes(fields, synq_role, model);
        all_bytes.extend_from_slice(&entry);
        count += 1;
    }

    // Emit the byte array.
    w.line(&format!("static const uint8_t {prefix}_roles_data[] = {{"));
    // 8 bytes per line (one role per line).
    for chunk in all_bytes.chunks(ROLE_SIZE) {
        let vals: Vec<String> = chunk.iter().map(|b| format!("0x{b:02x}")).collect();
        w.line(&format!("    {},", vals.join(",")));
    }
    w.line("};");
    w.newline();
    w.line(&format!(
        "static const uint32_t {prefix}_roles_count = {count};"
    ));

    // Append macro_def entries (if any nodes have macro_def annotations).
    append_macro_defs_to_header(&mut w, model, prefix);

    w.newline();
    w.header_guard_end(&guard);
    w.finish()
}

/// Append `macro_def` entries to the roles header.
///
/// Emits `static const uint8_t <prefix>_macro_defs_data[]` and
/// `static const uint32_t <prefix>_macro_defs_count`.
///
/// Each entry is `size_of::<MacroDef>()` = 8 bytes.
#[expect(clippy::cast_possible_truncation)]
pub(crate) fn append_macro_defs_to_header(w: &mut CWriter, model: &AstModel, prefix: &str) {
    use super::NodeLikeRef;
    const MACRO_DEF_SIZE: usize = std::mem::size_of::<MacroDef>();

    let mut entries: Vec<[u8; 8]> = Vec::new();

    // Tags are 1-based (0 is the sentinel). node_like_items are in tag order.
    for (idx, node_like) in model.node_like_items().iter().enumerate() {
        let tag = (idx + 1) as u16; // 1-based; node tags fit u16
        if let NodeLikeRef::Node(n) = node_like
            && let Some(mdef) = n.macro_def
        {
            let fi = |name: &str| -> u8 { field_index(n.fields, name) };

            // Resolve arg_name_field: look up the list's child type, find the field.
            let arg_name_field = if let Some(ref args_field_name) = mdef.args {
                let args_field = &n.fields[fi(args_field_name) as usize];
                // The args field points to a list type. Find that list's child type.
                let child_type = model
                    .lists()
                    .iter()
                    .find(|l| l.name == args_field.type_name)
                    .unwrap_or_else(|| {
                        panic!(
                            "args field '{args_field_name}' in '{}' references \
                             list type '{}' not found",
                            n.name, args_field.type_name
                        )
                    })
                    .child_type;
                // Find the child type's fields and look up arg_name.
                let arg_name = mdef
                    .arg_name
                    .as_ref()
                    .expect("arg_name required when args is set");
                let child_node = model
                    .nodes()
                    .iter()
                    .find(|cn| cn.name == child_type)
                    .unwrap_or_else(|| panic!("list child type '{child_type}' not found as node"));
                field_index(child_node.fields, arg_name)
            } else {
                FIELD_ABSENT
            };

            let mut bytes = [0u8; 8];
            bytes[0] = (tag & 0xFF) as u8;
            bytes[1] = (tag >> 8) as u8;
            bytes[2] = fi(&mdef.name);
            bytes[3] = fi(&mdef.body);
            bytes[4] = mdef.args.as_ref().map_or(FIELD_ABSENT, |a| fi(a));
            bytes[5] = arg_name_field;
            // bytes[6..8] = padding (zero)
            entries.push(bytes);
        }
    }

    w.newline();
    w.line(&format!(
        "/* Macro definition metadata for the {prefix} dialect. */\n\
         /* Each entry is {MACRO_DEF_SIZE} bytes: node_tag(u16) + 4 field indices + 2 pad. */"
    ));
    w.newline();
    if entries.is_empty() {
        // Empty array: use a single zero byte to avoid zero-length array.
        w.line(&format!(
            "static const uint8_t {prefix}_macro_defs_data[] = {{ 0 }};"
        ));
    } else {
        w.line(&format!(
            "static const uint8_t {prefix}_macro_defs_data[] = {{"
        ));
        for entry in &entries {
            let vals: Vec<String> = entry.iter().map(|b| format!("0x{b:02x}")).collect();
            w.line(&format!("    {},", vals.join(",")));
        }
        w.line("};");
    }
    w.newline();
    w.line(&format!(
        "static const uint32_t {prefix}_macro_defs_count = {};",
        entries.len()
    ));
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::dialect_codegen::AstModel;
    use crate::util::synq_parser::{Item, parse_synq_file};

    fn model_from(synq: &str) -> Vec<Item> {
        parse_synq_file(synq).expect("parse failed")
    }

    /// Parse roles out of bytes extracted from a header generated by `generate_c_roles_h`.
    ///
    /// The bytes come from `extract_bytes_from_header` which parses a C static array
    /// produced by the same binary, so the `SemanticRole` repr is guaranteed to match.
    fn parse_roles(bytes: &[u8]) -> &[SemanticRole] {
        assert_eq!(bytes.len() % ROLE_SIZE, 0);
        // SAFETY: bytes were extracted from a C header generated by `generate_c_roles_h`
        // in this same binary, so the SemanticRole #[repr(C, u8)] layout is identical.
        unsafe {
            std::slice::from_raw_parts(
                bytes.as_ptr().cast::<SemanticRole>(),
                bytes.len() / ROLE_SIZE,
            )
        }
    }

    /// Extract the raw role bytes from a generated header string.
    fn extract_bytes_from_header(header: &str, prefix: &str) -> Vec<u8> {
        let start_marker = format!("static const uint8_t {prefix}_roles_data[] = {{");
        let start = header.find(&start_marker).expect("data array not found") + start_marker.len();
        let end = header[start..].find("};").expect("closing brace not found") + start;
        let body = &header[start..end];
        let mut out = Vec::new();
        // Values are comma-separated with no spaces between them on each line.
        for tok in body.split(|c: char| c == ',' || c.is_whitespace()) {
            let tok = tok.trim();
            if tok.starts_with("0x") || tok.starts_with("0X") {
                out.push(u8::from_str_radix(&tok[2..], 16).expect("invalid hex byte"));
            }
        }
        out
    }

    #[test]
    fn transparent_for_node_without_annotation() {
        let items = model_from("node Foo { x: inline SyntaqliteTextSpan }");
        let model = AstModel::new(&items);
        let header = generate_c_roles_h(&model, "test");
        let bytes = extract_bytes_from_header(&header, "test");
        // 2 entries: sentinel + Foo
        assert_eq!(bytes.len(), 2 * ROLE_SIZE);
        let roles = parse_roles(&bytes);
        assert_eq!(roles[0], SemanticRole::Transparent); // sentinel
        assert_eq!(roles[1], SemanticRole::Transparent); // Foo — no annotation
    }

    #[test]
    fn define_table_with_correct_field_indices() {
        let items = model_from(
            r"node CreateTableStmt {
                table_name: inline SyntaqliteTextSpan
                schema: inline SyntaqliteTextSpan
                columns: index ColumnDefList
                as_select: index Select
                semantic { define_table(name: table_name, columns: columns, select: as_select) }
            }",
        );
        let model = AstModel::new(&items);
        let header = generate_c_roles_h(&model, "test");
        let bytes = extract_bytes_from_header(&header, "test");
        let roles = parse_roles(&bytes);
        // table_name = 0, columns = 2, as_select = 3
        assert_eq!(
            roles[1],
            SemanticRole::DefineTable {
                name: 0,
                columns: 2,
                select: 3,
                without_rowid: FlagSpec::ABSENT,
            }
        );
    }

    #[test]
    fn define_table_optional_fields_absent_when_not_given() {
        let items = model_from(
            r"node CreateTableStmt {
                table_name: inline SyntaqliteTextSpan
                semantic { define_table(name: table_name) }
            }",
        );
        let model = AstModel::new(&items);
        let header = generate_c_roles_h(&model, "test");
        let bytes = extract_bytes_from_header(&header, "test");
        let roles = parse_roles(&bytes);
        assert_eq!(
            roles[1],
            SemanticRole::DefineTable {
                name: 0,
                columns: FIELD_ABSENT,
                select: FIELD_ABSENT,
                without_rowid: FlagSpec::ABSENT,
            }
        );
    }

    #[test]
    fn define_view_with_correct_field_indices() {
        let items = model_from(
            r"node CreateViewStmt {
                view_name: inline SyntaqliteTextSpan
                schema: inline SyntaqliteTextSpan
                select: index Select
                semantic { define_view(name: view_name, select: select) }
            }",
        );
        let model = AstModel::new(&items);
        let header = generate_c_roles_h(&model, "test");
        let bytes = extract_bytes_from_header(&header, "test");
        let roles = parse_roles(&bytes);
        // view_name=0, no columns, select=2
        assert_eq!(
            roles[1],
            SemanticRole::DefineView {
                name: 0,
                columns: FIELD_ABSENT,
                select: 2
            }
        );
    }

    #[test]
    fn list_always_emits_transparent() {
        let items = model_from(
            r"node Foo { x: inline SyntaqliteTextSpan }
               list FooList { Foo }",
        );
        let model = AstModel::new(&items);
        let header = generate_c_roles_h(&model, "test");
        let bytes = extract_bytes_from_header(&header, "test");
        let roles = parse_roles(&bytes);
        // 3 entries: sentinel, Foo, FooList
        assert_eq!(roles[0], SemanticRole::Transparent);
        assert_eq!(roles[1], SemanticRole::Transparent); // Foo
        assert_eq!(roles[2], SemanticRole::Transparent); // FooList
    }

    #[test]
    fn count_matches_node_count() {
        let items = model_from(
            r"node Foo { x: inline SyntaqliteTextSpan }
               node Bar { y: inline SyntaqliteTextSpan }",
        );
        let model = AstModel::new(&items);
        let header = generate_c_roles_h(&model, "test");
        // 3 entries: sentinel + 2 nodes
        assert!(
            header.contains("static const uint32_t test_roles_count = 3;"),
            "got:\n{header}"
        );
    }

    #[test]
    fn source_ref_kind_bytes_correct() {
        let items = model_from(
            r"node TableRef {
                name: inline SyntaqliteTextSpan
                schema: inline SyntaqliteTextSpan
                alias: inline SyntaqliteTextSpan
                semantic { source_ref(kind: table, name: name, alias: alias) }
            }",
        );
        let model = AstModel::new(&items);
        let header = generate_c_roles_h(&model, "test");
        let bytes = extract_bytes_from_header(&header, "test");
        let roles = parse_roles(&bytes);
        assert_eq!(
            roles[1],
            SemanticRole::SourceRef {
                kind: RelationKind::Table,
                name: 0,
                alias: 2
            }
        );
    }
}
