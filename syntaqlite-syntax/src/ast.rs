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

/// Byte range in the original source text.
///
/// For spans inside a macro expansion, points at the macro call site in the
/// original source (not the expansion buffer).  Use
/// [`AnyParsedStatement::field_expansion_traceback`](crate::parser::AnyParsedStatement::field_expansion_traceback)
/// if you need position info inside the expansion.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct SourceRange {
    /// Inclusive start offset, in bytes.
    pub start: u32,
    /// Exclusive end offset, in bytes.
    pub end: u32,
}

impl SourceRange {
    /// Returns `true` if this range covers zero bytes.
    pub fn is_empty(self) -> bool {
        self.start == self.end
    }

    /// Byte length of the range.
    pub fn len(self) -> u32 {
        self.end - self.start
    }
}

impl From<SourceRange> for std::ops::Range<usize> {
    fn from(r: SourceRange) -> std::ops::Range<usize> {
        r.start as usize..r.end as usize
    }
}

/// Reflected field value extracted from a node.
///
/// Used by dialect-agnostic AST tooling built on
/// [`AnyParsedStatement::extract_fields`](crate::parser::AnyParsedStatement::extract_fields).
#[derive(Clone, Copy, Debug)]
pub enum FieldValue<'a> {
    /// A child node reference.
    NodeId(AnyNodeId),
    /// A source text span.
    Span {
        /// The span text.  When `quoted` is true this is the bare identifier
        /// with surrounding quotes stripped.  For spans inside a macro
        /// expansion, this is the resolved text in the expansion buffer.
        text: &'a str,
        /// Whether the identifier was quoted in source.  The formatter
        /// re-wraps quoted spans in standard double quotes (`"..."`).
        quoted: bool,
        /// Byte range in the original source.  For spans inside a macro
        /// expansion, points at the entire macro call site.
        source: SourceRange,
    },
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
pub struct NodeFields<'a> {
    buf: [std::mem::MaybeUninit<FieldValue<'a>>; 16],
    len: usize,
}

impl<'a> NodeFields<'a> {
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
    pub(crate) fn push(&mut self, val: FieldValue<'a>) {
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

impl<'a> std::ops::Index<usize> for NodeFields<'a> {
    type Output = FieldValue<'a>;

    fn index(&self, idx: usize) -> &FieldValue<'a> {
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

impl std::fmt::Debug for NodeFields<'_> {
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
        value: &'b FieldValue<'a>,
        stmt: &'b AnyParsedStatement<'a>,
    }

    impl serde::Serialize for FieldValueSerializer<'_, '_> {
        fn serialize<S: serde::Serializer>(&self, serializer: S) -> Result<S::Ok, S::Error> {
            match (self.meta.kind(), self.value) {
                // Node field: recurse, or null if absent.
                (FieldKind::NodeId, FieldValue::NodeId(id)) => {
                    if id.is_null() {
                        serializer.serialize_none()
                    } else {
                        match AnyNode::from_result(self.stmt, *id) {
                            Some(node) => node.serialize(serializer),
                            None => serializer.serialize_none(),
                        }
                    }
                }
                // Span: text string, or null for an absent/empty span.
                (FieldKind::Span, FieldValue::Span { text, .. }) => {
                    if text.is_empty() {
                        serializer.serialize_none()
                    } else {
                        serializer.serialize_str(text)
                    }
                }
                // Bool: plain boolean.
                (FieldKind::Bool, FieldValue::Bool(b)) => serializer.serialize_bool(*b),
                // Enum: display-name string, or null if no display name.
                (FieldKind::Enum, FieldValue::Enum(discriminant)) => {
                    match self.meta.display_name(*discriminant as usize) {
                        Some(s) => serializer.serialize_str(s),
                        None => serializer.serialize_none(),
                    }
                }
                // Flags: array of active flag-name strings (empty array when none set).
                (FieldKind::Flags, FieldValue::Flags(bits)) => {
                    use serde::ser::SerializeSeq;
                    let bits = *bits;
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
pub(crate) use ffi::CSourceSpan as SourceSpan;

mod ffi {
    use crate::ast::AnyNodeId;

    /// A source byte range stored in an AST node.
    ///
    /// Mirrors the C `SyntaqliteSourceSpan` layout.  Embedded in generated
    /// node structs for token-valued fields (identifiers, literals).
    ///
    /// Rust callers normally never see a raw `CSourceSpan`: span fields are
    /// eagerly resolved into [`FieldValue::Span`](super::FieldValue) values
    /// with text and a [`SourceRange`](super::SourceRange) already populated.
    #[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
    #[repr(C)]
    pub(crate) struct CSourceSpan {
        offset: u32,
        length: u16,
        flags: u8,
        /// Internal: 0 = original source, >0 = macro expansion buffer.
        /// Read by the C-side `syntaqlite_parser_resolve_span` helper; the
        /// Rust side never inspects it directly.
        _buf_idx: u8,
    }

    impl CSourceSpan {
        /// Returns `true` if the span covers zero bytes.
        pub(crate) fn is_empty(self) -> bool {
            self.length == 0
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
