// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

#![cfg_attr(test, allow(clippy::unwrap_used, clippy::similar_names))]

//! Internal shared primitives for syntaqlite — **not intended for direct use**.
//!
//! This crate is published to crates.io only because `syntaqlite` depends on it.
//! All types are `#[doc(hidden)]` and may change without notice.
//! Use the [`syntaqlite`](https://crates.io/crates/syntaqlite) crate instead.

#[doc(hidden)]
/// Semantic role types shared between the codegen tool and the runtime.
///
/// `SemanticRole` is `#[repr(C, u8)]` so the Rust in-memory layout
/// **is** the C wire format — the codegen tool byte-encodes constructed values
/// and the runtime casts the pointer directly to `&[SemanticRole]` without
/// any decoding.
pub mod roles {
    /// Sentinel value for a `u8` field that is absent (replaces `Option<u8>`).
    ///
    /// Using a sentinel avoids `Option<u8>` in `SemanticRole` variant fields,
    /// which would inflate the size from 1 byte to 2 bytes per field and break
    /// the flat `#[repr(C, u8)]` layout.
    pub const FIELD_ABSENT: u8 = 0xFF;

    /// Index into a node's field array (0-based).
    pub type FieldIdx = u8;

    /// A resolved reference to a single bit within a flags field.
    ///
    /// At codegen time, `flags.without_rowid` is resolved to a `FlagSpec`
    /// containing the field index and the specific bit mask, so the runtime
    /// never needs to know the flag type's layout.
    #[repr(C)]
    #[derive(Debug, Clone, Copy, PartialEq, Eq)]
    pub struct FlagSpec {
        /// Field index of the flags field (or `FIELD_ABSENT` if not present).
        pub field: FieldIdx,
        /// Bit mask for the specific flag (e.g. `1` for WITHOUT ROWID).
        pub mask: u8,
    }

    impl FlagSpec {
        /// A flag spec that is absent (no flag to check).
        pub const ABSENT: Self = Self {
            field: FIELD_ABSENT,
            mask: 0,
        };

        /// Check whether this flag is set in the given field value byte.
        pub fn is_set(self, flags_byte: u8) -> bool {
            self.field != FIELD_ABSENT && (flags_byte & self.mask) != 0
        }
    }

    /// The kind of relation a [`SemanticRole::SourceRef`] binding introduces.
    ///
    /// `#[repr(u8)]` so it fits in a single byte inside `SemanticRole`.
    #[repr(u8)]
    #[derive(Debug, Clone, Copy, PartialEq, Eq)]
    pub enum RelationKind {
        /// Standard SQL table.
        Table = 0,
        /// View — kept separate from `Table` for catalog queries.
        View = 1,
        /// Perfetto interval-structured data.
        Interval = 2,
        /// Perfetto tree-structured data.
        Tree = 3,
        /// Perfetto graph-structured data.
        Graph = 4,
    }

    /// The semantic role assigned to an AST node type.
    ///
    /// Generated from `semantic { ... }` annotations in `.synq` files and stored
    /// in a byte array indexed by node tag.  `Transparent` means the engine
    /// recurses into children without special handling.
    ///
    /// ## Layout
    ///
    /// `#[repr(C, u8)]` guarantees that:
    /// - byte 0 is the discriminant tag,
    /// - bytes 1–N are the payload fields (all `u8`), in declaration order,
    /// - remaining bytes up to `size_of::<SemanticRole>()` are zero-padding.
    ///
    /// The largest variant (`Query`) has 7 `u8` fields, so
    /// `size_of::<SemanticRole>() == 8` on all supported targets.
    ///
    /// Optional fields use [`FIELD_ABSENT`] (`0xFF`) as a sentinel instead of
    /// `Option<u8>` to keep every field exactly 1 byte.
    #[repr(C, u8)]
    #[derive(Debug, Clone, Copy, PartialEq, Eq)]
    pub enum SemanticRole {
        // ── Catalog roles ─────────────────────────────────────────────────
        /// CREATE TABLE statement: registers a table in the catalog.
        DefineTable {
            /// Field index of the table name.
            name: FieldIdx,
            /// Field index of the column-definition list (`FIELD_ABSENT` if absent).
            columns: FieldIdx,
            /// Field index of an AS-SELECT body (`FIELD_ABSENT` if absent).
            select: FieldIdx,
            /// Resolved flag: WITHOUT ROWID (`FlagSpec::ABSENT` if not present).
            without_rowid: FlagSpec,
        } = 0,
        /// CREATE VIEW statement: registers a view in the catalog.
        DefineView {
            /// Field index of the view name.
            name: FieldIdx,
            /// Field index of the optional declared column list (`FIELD_ABSENT` if absent).
            columns: FieldIdx,
            /// Field index of the SELECT body.
            select: FieldIdx,
        } = 1,
        /// CREATE FUNCTION statement: registers a function in the catalog.
        DefineFunction {
            /// Field index of the function name.
            name: FieldIdx,
            /// Field index of the argument list (`FIELD_ABSENT` if absent).
            args: FieldIdx,
            /// Field index of a return-type child node (`FIELD_ABSENT` if absent).
            return_type: FieldIdx,
            /// Field index of an AS-SELECT body (`FIELD_ABSENT` if absent).
            select: FieldIdx,
        } = 2,
        /// Annotates a return-type descriptor node.
        ReturnSpec {
            /// Field index of the column list child (`FIELD_ABSENT` if scalar-returning).
            columns: FieldIdx,
        } = 3,
        /// Module import statement: registers an imported module name.
        Import {
            /// Field index of the module name.
            module: FieldIdx,
        } = 4,

        // ── Column-list items ──────────────────────────────────────────────
        /// A single column definition within a CREATE TABLE column list.
        ColumnDef {
            /// Field index of the column name.
            name: FieldIdx,
            /// Field index of the type annotation (`FIELD_ABSENT` if absent).
            type_: FieldIdx,
            /// Field index of the constraint list (`FIELD_ABSENT` if absent).
            constraints: FieldIdx,
        } = 5,

        // ── Result columns ─────────────────────────────────────────────────
        /// A single result column in a SELECT list.
        ResultColumn {
            /// Field index of the flags bitfield (e.g. `STAR = 1`).
            flags: FieldIdx,
            /// Field index of the alias.
            alias: FieldIdx,
            /// Field index of the value expression.
            expr: FieldIdx,
        } = 6,

        // ── Expressions ────────────────────────────────────────────────────
        /// Function/aggregate/window call: validate name and arg count.
        Call {
            /// Field index of the function name.
            name: FieldIdx,
            /// Field index of the argument list.
            args: FieldIdx,
        } = 7,
        /// Column reference: validate column and optional table qualifier.
        ColumnRef {
            /// Field index of the column name.
            column: FieldIdx,
            /// Field index of the optional table qualifier.
            table: FieldIdx,
        } = 8,

        // ── Sources ────────────────────────────────────────────────────────
        /// Table/view reference in FROM — adds binding to current scope.
        SourceRef {
            /// The kind of relation being referenced (`#[repr(u8)]`).
            kind: RelationKind,
            /// Field index of the relation name.
            name: FieldIdx,
            /// Field index of the alias.
            alias: FieldIdx,
        } = 9,
        /// Subquery in FROM — opens a fresh scope, then binds alias in outer scope.
        ScopedSource {
            /// Field index of the subquery body.
            body: FieldIdx,
            /// Field index of the alias.
            alias: FieldIdx,
        } = 10,

        // ── Scope structure ────────────────────────────────────────────────
        /// SELECT statement: process `from` first, then validate `exprs`.
        Query {
            /// Field index of the FROM clause.
            from: FieldIdx,
            /// Field index of the result-column list.
            columns: FieldIdx,
            /// Field index of the WHERE clause.
            where_clause: FieldIdx,
            /// Field index of the GROUP BY clause.
            groupby: FieldIdx,
            /// Field index of the HAVING clause.
            having: FieldIdx,
            /// Field index of the ORDER BY clause.
            orderby: FieldIdx,
            /// Field index of the LIMIT clause.
            limit_clause: FieldIdx,
        } = 11,
        /// CTE definition: binds a name to a subquery body.
        CteBinding {
            /// Field index of the CTE name.
            name: FieldIdx,
            /// Optional declared column list (`FIELD_ABSENT` if absent).
            columns: FieldIdx,
            /// Field index of the SELECT body.
            body: FieldIdx,
        } = 12,
        /// WITH clause: sequential CTE scope wrapping a main query.
        CteScope {
            /// Field index of the RECURSIVE flag.
            recursive: FieldIdx,
            /// Field index of the CTE binding list.
            bindings: FieldIdx,
            /// Field index of the main query body.
            body: FieldIdx,
        } = 13,
        /// CREATE TRIGGER: injects OLD/NEW into the trigger body scope.
        TriggerScope {
            /// Field index of the target table.
            target: FieldIdx,
            /// Field index of the WHEN expression.
            when: FieldIdx,
            /// Field index of the trigger body.
            body: FieldIdx,
        } = 14,

        /// DML statement (INSERT/UPDATE/DELETE): push a query scope, then
        /// recurse into children (field-declaration order ensures the target
        /// table's `SourceRef` is visited first, registering it in scope
        /// before column refs are resolved).
        ///
        /// When the DML carries a `WITH` prefix, `with_ctes` points at the
        /// CTE binding list and `with_recursive` at the RECURSIVE flag;
        /// both are `FIELD_ABSENT` when the DML has no CTE prefix.
        DmlScope {
            /// Field index of the RECURSIVE flag (`FIELD_ABSENT` if absent).
            with_recursive: FieldIdx,
            /// Field index of the CTE binding list (`FIELD_ABSENT` if absent).
            with_ctes: FieldIdx,
        } = 15,

        /// No semantic role — recurse into children generically.
        Transparent = 16,

        /// INSERT upsert clause (`ON CONFLICT ... DO UPDATE`): the conflict
        /// target is walked in the enclosing DML scope, then `excluded` is
        /// injected for the DO UPDATE set list and WHERE.
        UpsertScope {
            /// Field index of the conflict-target column list (`FIELD_ABSENT` if absent).
            columns: FieldIdx,
            /// Field index of the conflict-target WHERE (`FIELD_ABSENT` if absent).
            target_where: FieldIdx,
            /// Field index of the DO UPDATE set list (`FIELD_ABSENT` if absent).
            setlist: FieldIdx,
            /// Field index of the DO UPDATE WHERE (`FIELD_ABSENT` if absent).
            update_where: FieldIdx,
        } = 17,

        /// Compound SELECT (UNION/INTERSECT/EXCEPT): arms are walked in
        /// their own scopes, then the compound-level ORDER BY / LIMIT
        /// resolve against every arm's sources and select aliases.
        CompoundScope {
            /// Field index of the left arm.
            left: FieldIdx,
            /// Field index of the right arm.
            right: FieldIdx,
            /// Field index of the hoisted ORDER BY (`FIELD_ABSENT` if absent).
            orderby: FieldIdx,
            /// Field index of the hoisted LIMIT (`FIELD_ABSENT` if absent).
            limit_clause: FieldIdx,
        } = 18,

        /// ALTER TABLE statement: existence-checks the target and mutates the
        /// catalog (add/drop/rename column, rename table). The `op` field
        /// selects which mutation to apply; the runtime interprets its enum
        /// discriminant against the dialect's alter-op ordering.
        AlterTable {
            /// Field index of the alter-op enum discriminant.
            op: FieldIdx,
            /// Field index of the target relation (a qualified-name node).
            target: FieldIdx,
            /// Field index of the new name (rename table / rename column).
            new_name: FieldIdx,
            /// Field index of the old name (rename/drop column name).
            old_name: FieldIdx,
            /// Field index of the added column definition (`FIELD_ABSENT` if absent).
            column: FieldIdx,
        } = 19,
    }

    /// Metadata for a node type that defines a template macro.
    ///
    /// Stored in a small array in the dialect template, indexed linearly
    /// (typically 0–1 entries). The formatter checks each parsed statement's
    /// root tag against these entries to auto-register macros.
    ///
    /// ## Layout
    ///
    /// All fields are `u8` (alignment = 1), so it can be safely cast from
    /// a `*const u8` byte array. Total size is 8 bytes (6 payload + 2 pad).
    #[repr(C)]
    #[derive(Debug, Clone, Copy, PartialEq, Eq)]
    pub struct MacroDef {
        /// Node tag low byte (little-endian).
        pub node_tag_lo: u8,
        /// Node tag high byte (little-endian).
        pub node_tag_hi: u8,
        /// Field index for the macro name (`SyntaqliteTextSpan`).
        pub name_field: FieldIdx,
        /// Field index for the macro body (`SyntaqliteTextSpan`).
        pub body_field: FieldIdx,
        /// Field index for the args list node (`FIELD_ABSENT` if no params).
        pub args_field: FieldIdx,
        /// Within each args list item, field index of the param name span.
        /// Only meaningful when `args_field != FIELD_ABSENT`.
        pub arg_name_field: FieldIdx,
        _pad0: u8,
        _pad1: u8,
    }

    impl MacroDef {
        /// Reconstruct the node tag as a `u32`.
        pub fn node_tag(self) -> u32 {
            u32::from(self.node_tag_lo) | (u32::from(self.node_tag_hi) << 8)
        }
    }

    #[cfg(test)]
    mod tests {
        use super::*;
        use std::mem::size_of;

        #[test]
        fn macro_def_is_8_bytes() {
            assert_eq!(size_of::<MacroDef>(), 8);
        }

        #[test]
        fn semantic_role_is_8_bytes() {
            // Largest variant (Query) has 7 u8 fields → 1 (tag) + 7 (payload) = 8.
            assert_eq!(size_of::<SemanticRole>(), 8);
        }

        #[test]
        fn relation_kind_is_1_byte() {
            assert_eq!(size_of::<RelationKind>(), 1);
        }

        #[test]
        fn discriminants_are_correct() {
            // Verify byte-0 discriminants match the explicit `= N` values.
            let check = |role: SemanticRole, expected: u8| {
                // SAFETY: `SemanticRole` is `#[repr(C, u8)]`; byte 0 is the discriminant.
                let byte0 = unsafe { *std::ptr::addr_of!(role).cast::<u8>() };
                assert_eq!(byte0, expected, "wrong discriminant for variant");
            };
            check(
                SemanticRole::DefineTable {
                    name: 0,
                    columns: 0,
                    select: 0,
                    without_rowid: FlagSpec::ABSENT,
                },
                0,
            );
            check(SemanticRole::Transparent, 16);
            check(
                SemanticRole::UpsertScope {
                    columns: 0,
                    target_where: 0,
                    setlist: 0,
                    update_where: 0,
                },
                17,
            );
            check(
                SemanticRole::CompoundScope {
                    left: 0,
                    right: 0,
                    orderby: 0,
                    limit_clause: 0,
                },
                18,
            );
        }

        #[test]
        fn payload_bytes_are_at_expected_offsets() {
            // Query has 7 fields starting at byte 1.
            let role = SemanticRole::Query {
                from: 1,
                columns: 2,
                where_clause: 3,
                groupby: 4,
                having: 5,
                orderby: 6,
                limit_clause: 7,
            };
            // SAFETY: `SemanticRole` is `#[repr(C, u8)]` with size 8; all 8 bytes are valid u8.
            let bytes =
                unsafe { std::slice::from_raw_parts(std::ptr::addr_of!(role).cast::<u8>(), 8) };
            assert_eq!(bytes[0], 11); // discriminant
            assert_eq!(bytes[1], 1); // from
            assert_eq!(bytes[2], 2); // columns
            assert_eq!(bytes[3], 3); // where_clause
            assert_eq!(bytes[4], 4); // groupby
            assert_eq!(bytes[5], 5); // having
            assert_eq!(bytes[6], 6); // orderby
            assert_eq!(bytes[7], 7); // limit_clause
        }
    }
}

#[doc(hidden)]
/// Formatter bytecode types shared between the codegen tool and the runtime.
pub mod fmt {
    /// Binary encoding of formatter bytecode programs.
    pub mod bytecode {
        /// Opcode constants for the formatter bytecode instruction set.
        pub mod opcodes {
            /// Emit a fixed keyword string from the dialect's string table.
            pub const KEYWORD: u8 = 0;
            /// Emit a source span (verbatim text slice) from the AST node.
            pub const SPAN: u8 = 1;
            /// Recurse into a child node field.
            pub const CHILD: u8 = 2;
            /// Emit a line break (collapses to a space when the group fits on one line).
            pub const LINE: u8 = 3;
            /// Emit a line break only if the enclosing group does not fit on one line.
            pub const SOFTLINE: u8 = 4;
            /// Always emit a hard (unconditional) line break.
            pub const HARDLINE: u8 = 5;
            /// Begin a group; the renderer tries to fit the group on one line.
            pub const GROUP_START: u8 = 6;
            /// End a group started by `GROUP_START`.
            pub const GROUP_END: u8 = 7;
            /// Begin an indented nesting block.
            pub const NEST_START: u8 = 8;
            /// End a nesting block started by `NEST_START`.
            pub const NEST_END: u8 = 9;
            /// Begin a conditional block gated on whether a node field is present.
            pub const IF_SET: u8 = 10;
            /// Else branch of an `IF_SET` / `IF_BOOL` / `IF_FLAG` / `IF_ENUM` block.
            pub const ELSE_OP: u8 = 11;
            /// End a conditional block.
            pub const END_IF: u8 = 12;
            /// Begin iteration over a list field.
            pub const FOR_EACH_START: u8 = 13;
            /// Emit the current list element inside a `FOR_EACH` loop.
            pub const CHILD_ITEM: u8 = 14;
            /// Emit a separator between list elements.
            pub const FOR_EACH_SEP: u8 = 15;
            /// End a `FOR_EACH` loop.
            pub const FOR_EACH_END: u8 = 16;
            /// Begin a conditional block gated on a boolean inline field.
            pub const IF_BOOL: u8 = 17;
            /// Begin a conditional block gated on a flags bitmask field.
            pub const IF_FLAG: u8 = 18;
            /// Begin a conditional block gated on an enum field value.
            pub const IF_ENUM: u8 = 19;
            /// Begin a conditional block gated on whether a span field is non-empty.
            pub const IF_SPAN: u8 = 20;
            /// Emit the display string for an enum field value.
            pub const ENUM_DISPLAY: u8 = 21;
            /// Begin iteration over the node itself as a list (self-referential list node).
            pub const FOR_EACH_SELF_START: u8 = 22;
            /// Recurse into a child node with precedence-aware parenthesization.
            ///
            /// `a` = child field index, `b` = prec table base (u16),
            /// `c` = packed `(op_field_idx << 8) | is_right`.
            /// At runtime, reads the parent's operator ordinal from `fields[op_field_idx]`,
            /// looks up `(prec, group)` in the precedence table, then compares with the
            /// child's precedence (if the child tag has expr-meta). Also wraps if the
            /// child is a list node.
            pub const CHILD_PREC: u8 = 23;
            /// Recurse into a child node, wrapping in parens if the child is a list node.
            ///
            /// `a` = child field index. Handles the "ExprList-as-expression" case where
            /// a list node appears in a single-expression context.
            pub const CHILD_PAREN_LIST: u8 = 24;
            /// Recurse into a child node with fixed-precedence parenthesization.
            ///
            /// `a` = child field index, `b` = packed `(prec << 8) | group_and_flags`,
            /// `c` = `is_right` (0 or 1).
            /// Like `CHILD_PREC` but the parent's precedence is fixed (not from an enum field).
            pub const CHILD_PREC_FIXED: u8 = 25;
        }

        /// A compiled op in its binary encoding: 6 bytes total.
        /// `a` is used for field indices, `b` for string IDs / ordinals / masks,
        /// `c` for skip counts.
        #[derive(Clone, Copy)]
        pub struct RawOp {
            /// The opcode byte identifying the instruction type.
            pub opcode: u8,
            /// Field index or auxiliary operand A.
            pub a: u8,
            /// String ID, ordinal, or bitmask operand B.
            pub b: u16,
            /// Skip count or auxiliary operand C.
            pub c: u16,
        }

        impl RawOp {
            /// Construct an instruction with only an opcode and all other fields zeroed.
            #[must_use]
            pub const fn simple(opcode: u8) -> Self {
                Self {
                    opcode,
                    a: 0,
                    b: 0,
                    c: 0,
                }
            }
        }
    }
}
