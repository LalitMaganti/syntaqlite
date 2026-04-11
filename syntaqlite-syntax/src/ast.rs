// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use std::marker::PhantomData;

use crate::parser::AnyParsedStatement;

// ── Public API ───────────────────────────────────────────────────────────────

/// Trait for AST node views that can be materialized from arena IDs.
///
/// Implemented by generated node wrappers and used by generic traversals such
/// as [`TypedNodeList`].
pub trait GrammarNodeType<'a>: Sized {
    /// Resolve `id` to `Self`, or `None` if null, invalid, or tag mismatch.
    fn from_result(stmt_result: &'a AnyParsedStatement<'a>, id: AnyNodeId) -> Option<Self>;
}

/// Trait for token enums that support typed <-> raw conversion.
///
/// Enables tokenizer/parser code that is generic over a dialect's token type.
pub trait GrammarTokenType: Sized + Clone + Copy + std::fmt::Debug + Into<u32> {
    /// Convert a type-erased [`AnyTokenType`] into this dialect's typed token
    /// variant, or `None` if the ordinal is out of range.
    fn from_token_type(raw: AnyTokenType) -> Option<Self>;
}

/// Type-erased token kind represented as a raw ordinal.
///
/// Use this in dialect-agnostic paths where concrete token enums are unknown.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[repr(transparent)]
pub struct AnyTokenType(pub(crate) u32);

impl AnyTokenType {
    /// Construct from a raw token-type ordinal.
    ///
    /// This does not validate that `v` is a known token for any particular
    /// dialect. Prefer typed token enums when available.
    pub fn from_raw(v: u32) -> Self {
        AnyTokenType(v)
    }
}

impl From<AnyTokenType> for u32 {
    fn from(t: AnyTokenType) -> u32 {
        t.0
    }
}

impl GrammarTokenType for AnyTokenType {
    fn from_token_type(raw: AnyTokenType) -> Option<Self> {
        Some(raw)
    }
}

/// Type-erased AST node tag represented as a raw ordinal.
///
/// Use this for dialect-agnostic AST introspection.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[repr(transparent)]
pub struct AnyNodeTag(pub(crate) u32);

impl From<AnyNodeTag> for u32 {
    fn from(t: AnyNodeTag) -> u32 {
        t.0
    }
}

impl AnyNodeTag {
    /// Construct from a raw node tag ordinal.
    ///
    /// This does not validate that `v` is a known tag for any particular
    /// dialect. Prefer typed tags when available.
    pub fn from_raw(v: u32) -> Self {
        AnyNodeTag(v)
    }
}

/// Lifetime-free handle to a node in the parser arena.
///
/// Store this when you need stable node identity outside a borrowed AST view.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[repr(transparent)]
pub struct AnyNodeId(pub(crate) u32);

impl AnyNodeId {
    /// Sentinel value representing a missing/null node.
    pub(crate) const NULL: AnyNodeId = AnyNodeId(0xFFFF_FFFF);

    /// Returns `true` if this is the null sentinel.
    pub fn is_null(&self) -> bool {
        self.0 == Self::NULL.0
    }
}

/// Dialect-agnostic node view.
///
/// Useful for tooling that traverses trees without generated node enums.
#[derive(Clone, Copy)]
pub struct AnyNode<'a> {
    pub(crate) id: AnyNodeId,
    pub(crate) stmt_result: &'a AnyParsedStatement<'a>,
}

impl<'a> GrammarNodeType<'a> for AnyNode<'a> {
    fn from_result(stmt_result: &'a AnyParsedStatement<'a>, id: AnyNodeId) -> Option<Self> {
        stmt_result.node_ptr(id)?; // validate the node exists
        Some(AnyNode { id, stmt_result })
    }
}

impl std::fmt::Debug for AnyNode<'_> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("Node").field("id", &self.id).finish()
    }
}

impl std::fmt::Display for AnyNode<'_> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let mut buf = String::new();
        self.stmt_result.dump_node(self.id, &mut buf, 0);
        f.write_str(&buf)
    }
}

/// Typed read-only view over a list node in the arena.
///
/// Used throughout generated AST APIs for child collections.
#[derive(Clone)]
pub struct TypedNodeList<'a, G: crate::dialect::TypedDialect, T> {
    raw: &'a RawNodeList,
    stmt_result: &'a AnyParsedStatement<'a>,
    id: AnyNodeId,
    _phantom: PhantomData<fn() -> (G, T)>,
}

// Manual Copy impl: all fields are Copy regardless of G or T.
// `derive(Copy)` would add a spurious `G: Copy` bound via PhantomData,
// which would propagate to every generated list alias.
impl<G: crate::dialect::TypedDialect, T: Clone> Copy for TypedNodeList<'_, G, T> {}

impl<G: crate::dialect::TypedDialect, T> std::fmt::Debug for TypedNodeList<'_, G, T> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("TypedNodeList")
            .field("len", &self.raw.children().len())
            .finish()
    }
}

impl<G: crate::dialect::TypedDialect, T> TypedNodeList<'_, G, T> {
    /// The arena node ID of this list, as the dialect's typed node ID.
    pub fn node_id(&self) -> G::NodeId {
        G::NodeId::from(self.id)
    }

    /// Number of children.
    pub fn len(&self) -> usize {
        self.raw.children().len()
    }

    /// Whether this list has no children.
    pub fn is_empty(&self) -> bool {
        self.raw.children().is_empty()
    }
}

impl<'a, G: crate::dialect::TypedDialect, T: GrammarNodeType<'a>> TypedNodeList<'a, G, T> {
    /// Get a child by index, or `None` if out of bounds or unresolvable.
    pub fn get(&self, index: usize) -> Option<T> {
        let id = *self.raw.children().get(index)?;
        T::from_result(self.stmt_result, id)
    }

    /// Iterate over children. Unresolvable IDs are silently skipped.
    pub fn iter(&self) -> impl Iterator<Item = T> + 'a {
        let stmt_result = self.stmt_result;
        let children = self.raw.children();
        children
            .iter()
            .filter_map(move |&id| T::from_result(stmt_result, id))
    }
}

/// Trait for typed node IDs generated per AST node kind.
///
/// IDs are cheap, storable handles that can later be resolved against a parse
/// result back into typed node views.
pub trait TypedNodeId: Copy + Into<AnyNodeId> {
    /// The typed view produced when this ID is resolved against an arena.
    type Node<'a>: GrammarNodeType<'a>;
}

/// Reflected field value extracted from a node.
///
/// Used by dialect-agnostic AST tooling built on
/// [`AnyParsedStatement::extract_fields`](crate::parser::AnyParsedStatement::extract_fields).
///
/// Mirrors the C-side 1:1: `Span` wraps the raw `TextSpan` struct and
/// callers use the accessor methods on
/// [`AnyParsedStatement`](crate::parser::AnyParsedStatement) (e.g.
/// [`span_text`](crate::parser::AnyParsedStatement::span_text)) to get
/// the bytes / authored offset / expansion-layer view.
#[derive(Clone, Copy, Debug)]
pub enum FieldValue {
    /// A child node reference.
    NodeId(AnyNodeId),
    /// A source text span.  Use
    /// [`AnyParsedStatement::span_text`](crate::parser::AnyParsedStatement::span_text)
    /// to resolve to authored bytes.
    Span(TextSpan),
    /// A boolean flag.
    Bool(bool),
    /// A compact bitfield of flags.
    Flags(u8),
    /// An enum discriminant.
    Enum(u32),
}

/// Compact reflected field collection for one AST node.
///
/// Returned by [`AnyParsedStatement::extract_fields`](crate::parser::AnyParsedStatement::extract_fields)
/// and indexable via `fields[idx]`.
pub struct NodeFields {
    buf: [std::mem::MaybeUninit<FieldValue>; 16],
    len: usize,
}

impl NodeFields {
    /// Create an empty `NodeFields`.
    pub(crate) fn new() -> Self {
        Self {
            buf: [const { std::mem::MaybeUninit::uninit() }; 16],
            len: 0,
        }
    }

    /// Append a field value.
    ///
    /// # Panics
    /// Panics if more than 16 fields are pushed.
    pub(crate) fn push(&mut self, val: FieldValue) {
        assert!(self.len < 16, "NodeFields overflow: more than 16 fields");
        self.buf[self.len] = std::mem::MaybeUninit::new(val);
        self.len += 1;
    }

    /// Number of fields.
    pub fn len(&self) -> usize {
        self.len
    }

    /// Whether there are no fields.
    pub fn is_empty(&self) -> bool {
        self.len == 0
    }
}

impl std::ops::Index<usize> for NodeFields {
    type Output = FieldValue;

    fn index(&self, idx: usize) -> &FieldValue {
        assert!(
            idx < self.len,
            "field index {} out of bounds (len={})",
            idx,
            self.len
        );
        // SAFETY: buf[..len] are all initialised via `push`.
        unsafe { self.buf[idx].assume_init_ref() }
    }
}

impl std::fmt::Debug for NodeFields {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let mut list = f.debug_list();
        for i in 0..self.len {
            list.entry(&self[i]);
        }
        list.finish()
    }
}

// ── Crate-internal ───────────────────────────────────────────────────────────

/// Blanket [`GrammarNodeType`] impl for [`TypedNodeList`] — resolves the ID as a list node.
impl<'a, G: crate::dialect::TypedDialect, T> GrammarNodeType<'a> for TypedNodeList<'a, G, T> {
    fn from_result(stmt_result: &'a AnyParsedStatement<'a>, id: AnyNodeId) -> Option<Self> {
        let raw = stmt_result.resolve_list(id)?;
        Some(TypedNodeList {
            raw,
            stmt_result,
            id,
            _phantom: PhantomData,
        })
    }
}

/// Implemented by each `#[repr(C)]` arena node struct to declare its type tag.
///
/// # Safety
/// Implementors must guarantee that `TAG` matches the `tag` field value
/// that the C parser writes into the first `u32` of the struct.
pub(crate) unsafe trait ArenaNode {
    const TAG: u32;
}

// ── serde::Serialize (feature = "serde") ─────────────────────────────────────

#[cfg(feature = "serde")]
mod serde_impl {
    use super::{AnyNode, FieldValue, GrammarNodeType};
    use crate::dialect::{FieldKind, FieldMeta};
    use crate::parser::AnyParsedStatement;

    /// Serializes an AST node to the JSON equivalent of the text dump format.
    ///
    /// Regular nodes become `{ "type": "NodeName", "field1": value, ... }`.
    /// List nodes become `{ "type": "ListName", "count": N, "children": [...] }`.
    /// Field values mirror the dump: spans as strings, bools as booleans,
    /// enums as their display-name strings, flags as arrays of active names,
    /// absent nodes/spans as `null`.
    impl serde::Serialize for AnyNode<'_> {
        fn serialize<S: serde::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
            use serde::ser::SerializeMap;

            let stmt = self.stmt_result;
            let dialect = &stmt.dialect;
            let id = self.id;

            let Some((tag, fields)) = stmt.extract_fields(id) else {
                return serializer.serialize_none();
            };

            let name = dialect.node_name(tag);

            if dialect.is_list(tag) {
                // { "type": "ListName", "count": N, "children": [...] }
                let raw_children = stmt.list_children(id).unwrap_or(&[]);
                let children: Vec<AnyNode<'_>> = raw_children
                    .iter()
                    .filter(|id| !id.is_null())
                    .filter_map(|&id| AnyNode::from_result(stmt, id))
                    .collect();
                let mut map = serializer.serialize_map(Some(3))?;
                map.serialize_entry("type", name)?;
                map.serialize_entry("count", &children.len())?;
                map.serialize_entry("children", &children)?;
                map.end()
            } else {
                // { "type": "NodeName", "field1": value1, ... }
                let metas: Vec<FieldMeta<'static>> = dialect.field_meta(tag).collect();
                let field_count = metas.len().min(fields.len());
                let mut map = serializer.serialize_map(Some(1 + field_count))?;
                map.serialize_entry("type", name)?;
                for i in 0..field_count {
                    let meta = &metas[i];
                    let value = &fields[i];
                    map.serialize_entry(meta.name(), &FieldValueSerializer { meta, value, stmt })?;
                }
                map.end()
            }
        }
    }

    /// Serializes the value side of a single field — the right-hand side of
    /// `"field_name": <this>`.
    struct FieldValueSerializer<'a, 'b> {
        meta: &'b FieldMeta<'static>,
        value: &'b FieldValue,
        stmt: &'b AnyParsedStatement<'a>,
    }

    impl serde::Serialize for FieldValueSerializer<'_, '_> {
        fn serialize<S: serde::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
            match (self.meta.kind(), *self.value) {
                // Node field: recurse, or null if absent.
                (FieldKind::NodeId, FieldValue::NodeId(id)) => {
                    if id.is_null() {
                        serializer.serialize_none()
                    } else {
                        match AnyNode::from_result(self.stmt, id) {
                            Some(node) => node.serialize(serializer),
                            None => serializer.serialize_none(),
                        }
                    }
                }
                // Span: authored text slice of the source, or null when empty.
                (FieldKind::Span, FieldValue::Span(span)) => {
                    let (text, _) = self.stmt.span_text(span);
                    if text.is_empty() {
                        serializer.serialize_none()
                    } else {
                        serializer.serialize_str(text)
                    }
                }
                // Bool: plain boolean.
                (FieldKind::Bool, FieldValue::Bool(b)) => serializer.serialize_bool(b),
                // Enum: display-name string, or null if no display name.
                (FieldKind::Enum, FieldValue::Enum(discriminant)) => {
                    match self.meta.display_name(discriminant as usize) {
                        Some(s) => serializer.serialize_str(s),
                        None => serializer.serialize_none(),
                    }
                }
                // Flags: array of active flag-name strings (empty array when none set).
                (FieldKind::Flags, FieldValue::Flags(bits)) => {
                    use serde::ser::SerializeSeq;
                    let active: Vec<&'static str> = (0..self.meta.display_count())
                        .filter(|&i| bits & (1u8 << i) != 0)
                        .filter_map(|i| self.meta.display_name(i))
                        .collect();
                    let mut seq = serializer.serialize_seq(Some(active.len()))?;
                    for s in &active {
                        seq.serialize_element(s)?;
                    }
                    seq.end()
                }
                // Shouldn't occur (kind/value mismatch would be a codegen bug).
                _ => serializer.serialize_none(),
            }
        }
    }
}

// ── ffi ───────────────────────────────────────────────────────────────────────

pub(crate) use ffi::CNodeList as RawNodeList;
pub use ffi::CTextSpan as TextSpan;

mod ffi {
    use crate::ast::AnyNodeId;

    /// A source byte range stored in an AST node.
    ///
    /// Mirrors the C `SyntaqliteTextSpan` layout 1:1.  The fields are
    /// deliberately opaque to Rust callers: the encoded position may
    /// refer to either the input source or an internal macro expansion
    /// buffer.  Resolve a `TextSpan` via the accessor methods on
    /// [`AnyParsedStatement`](crate::parser::AnyParsedStatement):
    /// [`span_text`](crate::parser::AnyParsedStatement::span_text),
    /// [`span_expanded_text`](crate::parser::AnyParsedStatement::span_expanded_text),
    /// [`span_text_offset`](crate::parser::AnyParsedStatement::span_text_offset).
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
    #[repr(C)]
    pub struct CTextSpan {
        pub(crate) offset: u32,
        pub(crate) length: u16,
        pub(crate) flags: u8,
        /// Internal: 0 = original source, >0 = macro expansion layer id.
        /// Read by the C-side span accessors; the Rust side passes the
        /// whole struct across the FFI boundary without inspecting it.
        pub(crate) _layer_id: u8,
    }

    /// Mirror of C `SYNTAQLITE_SPAN_FLAG_QUOTED` from `types.h`.
    const SPAN_FLAG_QUOTED: u8 = 1;

    impl CTextSpan {
        /// Returns `true` if the span covers zero bytes.
        pub fn is_empty(self) -> bool {
            self.length == 0
        }

        /// Returns `true` if the span was quoted in source (`"..."`,
        /// `` `...` ``, or `[...]`).  The span points at the dequoted inner
        /// text; the formatter re-wraps quoted spans in standard double
        /// quotes.
        pub fn is_quoted(self) -> bool {
            (self.flags & SPAN_FLAG_QUOTED) != 0
        }
    }

    /// List node header — `tag` + `count`, followed by `count` child [`AnyNodeId`]s
    /// in trailing data. The parser arena guarantees this contiguous layout.
    #[derive(Debug)]
    #[repr(C)]
    pub(crate) struct CNodeList {
        pub(crate) tag: u32,
        pub(crate) count: u32,
    }

    impl CNodeList {
        /// The child node IDs stored after this header in the arena.
        pub(crate) fn children(&self) -> &[AnyNodeId] {
            // SAFETY: The arena allocates list nodes as { tag, count, children[count] }
            // contiguously, so `count` u32 values immediately follow this header.
            // CNodeList is only constructed from valid arena pointers (validated tag).
            // AnyNodeId is #[repr(transparent)] over u32, so &[AnyNodeId] is
            // layout-compatible with &[u32].
            unsafe {
                let base = std::ptr::from_ref::<CNodeList>(self)
                    .add(1)
                    .cast::<AnyNodeId>();
                std::slice::from_raw_parts(base, self.count as usize)
            }
        }
    }
}
