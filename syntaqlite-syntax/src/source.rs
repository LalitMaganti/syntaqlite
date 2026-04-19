// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Typed source coordinates — positions, lengths, ranges, token indices,
//! line/column numbers, and source-text wrappers.
//!
//! Every position-like value crossing the syntaqlite API surface lives in one
//! of a small number of distinct *kinds*:
//!
//! * **Statement-relative byte offsets** ([`StmtOffset`]) — measured from
//!   the first byte of the current statement's source slice
//!   (`AnyParsedStatement::text()`).  Every offset emitted by the parser
//!   (token spans, comment spans, [`crate`]-local arena spans, error offsets,
//!   macro rewrite call offsets) is statement-relative.
//! * **Document-absolute byte offsets** ([`DocOffset`]) — measured from
//!   the first byte of the full bound source
//!   (`AnyParsedStatement::full_text()`).  The semantic layer and LSP
//!   protocol use these.
//! * **Layer-relative byte offsets** ([`LayerOffset`]) — measured from
//!   the start of an unspecified-but-contextual buffer (a macro expansion
//!   layer, a traceback snippet, a parent rewrite's expansion).  Used by
//!   APIs where the buffer is identified by a sibling field.
//! * **Token indices** ([`TokenIdx`]) — 0-based index into a statement's
//!   token stream.
//! * **1-based line / column** ([`LineNumber`], [`ColumnNumber`]) — as
//!   reported by the C parser for macro definition sites and traceback
//!   frames.  Zero means "unknown".
//! * **0-based UTF-16 line / column** ([`Utf16Line`], [`Utf16Col`]) — the
//!   LSP protocol's wire format.
//!
//! Lengths are kept distinct from positions ([`StmtLen`],
//! [`DocLen`]).  Position-plus-position is not meaningful and is not
//! supported; position-minus-position yields a length;
//! position-plus-length yields a position.  This catches the "I added two
//! positions together" bug that hides silently with raw integers.
//!
//! Storage is `u32` — halves AST memory compared to `usize` and is
//! sufficient for any source file that fits in a 32-bit address space.
//! Source files larger than 4 GiB are not supported.
//!
//! Conversion between statement-relative and document-absolute offsets goes
//! through [`StatementBase`], obtained from
//! `AnyParsedStatement::statement_base()`.  Conversion between byte
//! positions and line/UTF-16 positions requires the source text and lives
//! on a separate `SourceMap` type (added in a later change); these are not
//! `From` impls because they are not free.
//!
//! ## Typed source text
//!
//! Paired with the offset newtypes are three wrappers around `str` that
//! model the buffer each offset kind measures into:
//!
//! | Offset        | Corresponding text   |
//! |---------------|----------------------|
//! | [`StmtOffset`]  | [`StmtText`]         |
//! | [`DocOffset`]   | [`DocText`]          |
//! | [`LayerOffset`] | [`LayerText`]        |
//!
//! Each is `#[repr(transparent)]` over `str`, so wrapping a `&str` is
//! zero-cost.  They implement `Index<Range>` for their matching
//! range newtype, so `&stmt_text[stmt_range]` compiles but
//! `&stmt_text[doc_range]` does not — the type system rules out slicing
//! a statement-relative buffer with a document-absolute range.  Use
//! [`StmtText::as_str`] (and its peers) as the escape hatch when
//! interoperating with `std::str` APIs.

use core::fmt;
use core::ops::{Add, AddAssign, Index, Sub, SubAssign};

// ── Macro: define a u32 newtype with the standard set of impls ──────────────

macro_rules! define_u32_newtype {
    ($(#[$meta:meta])* $name:ident) => {
        $(#[$meta])*
        #[derive(Copy, Clone, Default, PartialEq, Eq, PartialOrd, Ord, Hash)]
        #[repr(transparent)]
        pub struct $name(u32);

        impl $name {
            /// Construct from a raw `u32`.
            ///
            /// Use only at FFI boundaries or when reading values from
            /// external sources (e.g. serialization).  Internal code should
            /// prefer typed arithmetic to obtain values of this type.
            pub const fn from_raw(v: u32) -> Self {
                Self(v)
            }

            /// Extract the underlying `u32`.
            ///
            /// Use only at FFI boundaries or for serialization.  Within
            /// Rust code, prefer typed arithmetic and `Index` impls (added
            /// in a later change) over reaching into the raw value.
            pub const fn as_u32(self) -> u32 {
                self.0
            }

            /// Extract the underlying value as `usize` for indexing into
            /// slices, vectors, and other `usize`-indexed collections.
            pub const fn as_usize(self) -> usize {
                self.0 as usize
            }
        }

        impl fmt::Debug for $name {
            fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
                write!(f, "{}({})", stringify!($name), self.0)
            }
        }

        impl fmt::Display for $name {
            fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
                fmt::Display::fmt(&self.0, f)
            }
        }
    };
}

// ── Macro: implement Position + Length = Position arithmetic ────────────────

macro_rules! impl_pos_len_arith {
    (pos: $pos:ident, len: $len:ident) => {
        impl Add<$len> for $pos {
            type Output = $pos;
            fn add(self, rhs: $len) -> $pos {
                $pos(self.0 + rhs.0)
            }
        }

        impl AddAssign<$len> for $pos {
            fn add_assign(&mut self, rhs: $len) {
                self.0 += rhs.0;
            }
        }

        impl Sub<$len> for $pos {
            type Output = $pos;
            fn sub(self, rhs: $len) -> $pos {
                $pos(self.0 - rhs.0)
            }
        }

        impl SubAssign<$len> for $pos {
            fn sub_assign(&mut self, rhs: $len) {
                self.0 -= rhs.0;
            }
        }

        impl Sub<$pos> for $pos {
            type Output = $len;
            fn sub(self, rhs: $pos) -> $len {
                $len(self.0 - rhs.0)
            }
        }

        impl Add<$len> for $len {
            type Output = $len;
            fn add(self, rhs: $len) -> $len {
                $len(self.0 + rhs.0)
            }
        }

        impl AddAssign<$len> for $len {
            fn add_assign(&mut self, rhs: $len) {
                self.0 += rhs.0;
            }
        }

        impl Sub<$len> for $len {
            type Output = $len;
            fn sub(self, rhs: $len) -> $len {
                $len(self.0 - rhs.0)
            }
        }

        impl SubAssign<$len> for $len {
            fn sub_assign(&mut self, rhs: $len) {
                self.0 -= rhs.0;
            }
        }
    };
}

// ── Statement-relative byte position / length ───────────────────────────────

define_u32_newtype! {
    /// A byte offset measured from the start of the current statement's
    /// source slice (`AnyParsedStatement::text()`).
    ///
    /// All offsets emitted by the parser are statement-relative — token
    /// spans, comment spans, `TextSpan` arena offsets, error offsets, and
    /// macro rewrite call offsets.
    ///
    /// To convert to a document-absolute offset, use
    /// [`StatementBase::to_doc`].
    StmtOffset
}

define_u32_newtype! {
    /// A byte length, paired with [`StmtOffset`].
    ///
    /// Adding a `StmtLen` to a [`StmtOffset`] yields another
    /// `StmtOffset`.  Subtracting two `StmtOffset`s yields a
    /// `StmtLen`.  This rules out the meaningless "offset + offset"
    /// operation at the type level.
    StmtLen
}

impl_pos_len_arith!(pos: StmtOffset, len: StmtLen);

/// A half-open range of statement-relative byte offsets `[start, end)`.
///
/// Carries the same semantics as `core::ops::Range<StmtOffset>` but
/// nominal — a function that wants a range can ask for `StmtRange` and
/// the caller can't accidentally swap two unrelated offsets.
#[derive(Copy, Clone, Default, PartialEq, Eq, Hash, Debug)]
pub struct StmtRange {
    /// Inclusive start of the range.
    pub start: StmtOffset,
    /// Exclusive end of the range.
    pub end: StmtOffset,
}

impl StmtRange {
    /// Construct a range from `(offset, length)` — the encoding the
    /// parser FFI uses.
    pub const fn from_offset_len(start: StmtOffset, len: StmtLen) -> Self {
        Self {
            start,
            end: StmtOffset(start.0 + len.0),
        }
    }

    /// The length of this range.
    pub const fn len(self) -> StmtLen {
        StmtLen(self.end.0 - self.start.0)
    }

    /// Whether this range covers zero bytes.
    pub const fn is_empty(self) -> bool {
        self.start.0 == self.end.0
    }

}

// ── Document-absolute byte position / length ────────────────────────────────

define_u32_newtype! {
    /// A byte offset measured from the start of the full bound source
    /// (`AnyParsedStatement::full_text()`).
    ///
    /// The semantic layer and LSP protocol boundary use document-absolute
    /// offsets.  To produce one from a [`StmtOffset`], use
    /// [`StatementBase::to_doc`].
    DocOffset
}

define_u32_newtype! {
    /// A byte length, paired with [`DocOffset`].  See [`StmtLen`]
    /// for the arithmetic rationale.
    DocLen
}

impl_pos_len_arith!(pos: DocOffset, len: DocLen);

/// A half-open range of document-absolute byte offsets `[start, end)`.
#[derive(Copy, Clone, Default, PartialEq, Eq, Hash, Debug)]
pub struct DocRange {
    /// Inclusive start of the range.
    pub start: DocOffset,
    /// Exclusive end of the range.
    pub end: DocOffset,
}

impl DocRange {
    /// Construct a range from `(offset, length)`.
    pub const fn from_offset_len(start: DocOffset, len: DocLen) -> Self {
        Self {
            start,
            end: DocOffset(start.0 + len.0),
        }
    }

    /// The length of this range.
    pub const fn len(self) -> DocLen {
        DocLen(self.end.0 - self.start.0)
    }

    /// Whether this range covers zero bytes.
    pub const fn is_empty(self) -> bool {
        self.start.0 == self.end.0
    }

}

// ── Layer-relative byte position / length ──────────────────────────────────

define_u32_newtype! {
    /// A byte offset measured from the start of an unspecified-but-contextual
    /// buffer — typically a macro expansion layer's buffer, a traceback
    /// snippet, or a parent rewrite's expansion text.
    ///
    /// Used by APIs whose offsets are interpreted relative to a buffer
    /// identified by a sibling field (e.g. `MacroRewrite`'s `parent`,
    /// `MacroArgSegment`'s `origin`).  Distinct from [`StmtOffset`] and
    /// [`DocOffset`] at the type level so callers can't accidentally use
    /// one in place of another.
    LayerOffset
}

define_u32_newtype! {
    /// A byte length, paired with [`LayerOffset`].  See [`StmtLen`] for the
    /// arithmetic rationale.
    LayerLen
}

impl_pos_len_arith!(pos: LayerOffset, len: LayerLen);

/// A half-open range of layer-relative byte offsets `[start, end)`.
#[derive(Copy, Clone, Default, PartialEq, Eq, Hash, Debug)]
pub struct LayerRange {
    /// Inclusive start of the range.
    pub start: LayerOffset,
    /// Exclusive end of the range.
    pub end: LayerOffset,
}

impl LayerRange {
    /// Construct a range from `(offset, length)`.
    pub const fn from_offset_len(start: LayerOffset, len: LayerLen) -> Self {
        Self {
            start,
            end: LayerOffset(start.0 + len.0),
        }
    }

    /// The length of this range.
    pub const fn len(self) -> LayerLen {
        LayerLen(self.end.0 - self.start.0)
    }

    /// Whether this range covers zero bytes.
    pub const fn is_empty(self) -> bool {
        self.start.0 == self.end.0
    }

}

// ── Statement base offset ───────────────────────────────────────────────────

/// The document-absolute offset of a statement's first byte.
///
/// Wraps [`DocOffset`] but is named distinctly so its purpose at API
/// boundaries (converting statement-relative offsets to document-absolute
/// ones, and back) is clear.
#[derive(Copy, Clone, Default, PartialEq, Eq, Hash, Debug)]
pub struct StatementBase(DocOffset);

impl StatementBase {
    /// Construct from a raw [`DocOffset`].
    pub const fn new(base: DocOffset) -> Self {
        Self(base)
    }

    /// The base offset, as a [`DocOffset`].
    pub const fn as_doc_offset(self) -> DocOffset {
        self.0
    }

    /// Convert a statement-relative offset to a document-absolute offset.
    pub const fn to_doc(self, off: StmtOffset) -> DocOffset {
        DocOffset(self.0.0 + off.0)
    }

    /// Convert a statement-relative range to a document-absolute range.
    pub const fn to_doc_range(self, range: StmtRange) -> DocRange {
        DocRange {
            start: self.to_doc(range.start),
            end: self.to_doc(range.end),
        }
    }

    /// Convert a document-absolute offset to a statement-relative offset.
    ///
    /// Returns `None` if `off` precedes the statement's start, in which
    /// case the offset cannot be expressed as statement-relative.
    pub const fn from_doc(self, off: DocOffset) -> Option<StmtOffset> {
        if off.0 < self.0.0 {
            None
        } else {
            Some(StmtOffset(off.0 - self.0.0))
        }
    }
}

// ── Token index ─────────────────────────────────────────────────────────────

define_u32_newtype! {
    /// A 0-based index into a statement's token stream.
    ///
    /// Used by [`crate`]-external code to identify a specific token within
    /// a parsed statement (for example, to look up the comments attached
    /// to it).  Distinct from byte offsets at the type level.
    TokenIdx
}

// ── 1-based line / column (C parser, traceback) ─────────────────────────────

define_u32_newtype! {
    /// A 1-based line number, as reported by the C parser for macro
    /// definition sites and traceback frames.
    ///
    /// The value `0` means "unknown" — the parser could not determine the
    /// line, which happens for offsets in macro expansion buffers without
    /// a recorded origin.
    LineNumber
}

define_u32_newtype! {
    /// A 1-based column number, as reported by the C parser.  See
    /// [`LineNumber`]; `0` means "unknown".
    ColumnNumber
}

// ── 0-based UTF-16 line / column (LSP protocol) ─────────────────────────────

define_u32_newtype! {
    /// A 0-based line number in UTF-16 code units, per the LSP protocol's
    /// `Position.line` field.
    ///
    /// LSP measures positions in UTF-16 code units rather than bytes;
    /// multi-byte UTF-8 characters count as their UTF-16 code-unit width
    /// (1 for the BMP, 2 for supplementary characters).
    Utf16Line
}

define_u32_newtype! {
    /// A 0-based column number in UTF-16 code units, per the LSP
    /// protocol's `Position.character` field.  See [`Utf16Line`].
    Utf16Col
}

// ── Typed source text ──────────────────────────────────────────────────────

macro_rules! define_text_newtype {
    (
        $(#[$meta:meta])*
        $name:ident,
        offset = $offset:ident,
        len = $len:ident,
        range = $range:ident,
    ) => {
        $(#[$meta])*
        #[repr(transparent)]
        pub struct $name(str);

        impl $name {
            /// Wrap a `&str` as a reference to this typed text.
            ///
            /// The caller asserts that positions into the string are in
            /// the coordinate system this newtype represents (e.g. for
            /// [`StmtText`], offsets are statement-relative).
            pub fn new(s: &str) -> &$name {
                // SAFETY: $name is `#[repr(transparent)]` over `str`, so
                // the layouts of `&str` and `&$name` are identical.
                unsafe { &*(std::ptr::from_ref(s) as *const $name) }
            }

            /// Underlying `&str` — use when interoperating with `std::str`
            /// APIs that don't understand the newtype.
            pub fn as_str(&self) -> &str {
                &self.0
            }

            /// Byte length, as a typed length.  Returns `u32::MAX` if the
            /// underlying string is longer than `u32::MAX` bytes (not
            /// possible for any syntaqlite-managed buffer).
            pub fn byte_len(&self) -> $len {
                $len::from_raw(u32::try_from(self.0.len()).unwrap_or(u32::MAX))
            }

            /// Whether the text is empty.
            pub fn is_empty(&self) -> bool {
                self.0.is_empty()
            }

            /// Range spanning the whole text `[0, byte_len())`.
            pub fn as_range(&self) -> $range {
                $range::from_offset_len($offset::default(), self.byte_len())
            }
        }

        impl AsRef<str> for $name {
            fn as_ref(&self) -> &str {
                &self.0
            }
        }

        impl fmt::Debug for $name {
            fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
                fmt::Debug::fmt(&self.0, f)
            }
        }

        impl fmt::Display for $name {
            fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
                fmt::Display::fmt(&self.0, f)
            }
        }

        impl PartialEq<str> for $name {
            fn eq(&self, other: &str) -> bool {
                &self.0 == other
            }
        }

        impl PartialEq<&str> for $name {
            fn eq(&self, other: &&str) -> bool {
                &self.0 == *other
            }
        }

        impl PartialEq<$name> for str {
            fn eq(&self, other: &$name) -> bool {
                self == &other.0
            }
        }

        impl PartialEq for $name {
            fn eq(&self, other: &Self) -> bool {
                self.0 == other.0
            }
        }

        impl Eq for $name {}

        impl core::hash::Hash for $name {
            fn hash<H: core::hash::Hasher>(&self, state: &mut H) {
                self.0.hash(state);
            }
        }

        impl Index<$range> for $name {
            type Output = str;
            fn index(&self, r: $range) -> &str {
                &self.0[r.start.as_usize()..r.end.as_usize()]
            }
        }
    };
}

define_text_newtype! {
    /// Source text for a single statement — the slice returned by
    /// `AnyParsedStatement::text()`.  Positions into a `StmtText` are
    /// measured in [`StmtOffset`] / [`StmtLen`] / [`StmtRange`].
    ///
    /// Slicing uses the typed range: `&stmt_text[range]` where
    /// `range: StmtRange`.  Indexing with a [`DocRange`] is a type error.
    StmtText,
    offset = StmtOffset,
    len = StmtLen,
    range = StmtRange,
}

define_text_newtype! {
    /// Full document source — the slice returned by
    /// `AnyParsedStatement::full_text()`.  Positions into a `DocText`
    /// are measured in [`DocOffset`] / [`DocLen`] / [`DocRange`].
    DocText,
    offset = DocOffset,
    len = DocLen,
    range = DocRange,
}

define_text_newtype! {
    /// Source text in an unspecified contextual buffer — typically a
    /// macro expansion layer, traceback snippet, or parent rewrite's
    /// expansion text.  Positions into a `LayerText` are measured in
    /// [`LayerOffset`] / [`LayerLen`] / [`LayerRange`].
    LayerText,
    offset = LayerOffset,
    len = LayerLen,
    range = LayerRange,
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn stmt_offset_plus_len_is_offset() {
        let o = StmtOffset::from_raw(10);
        let l = StmtLen::from_raw(5);
        assert_eq!((o + l).as_u32(), 15);
    }

    #[test]
    fn stmt_offset_minus_offset_is_len() {
        let a = StmtOffset::from_raw(15);
        let b = StmtOffset::from_raw(10);
        let diff: StmtLen = a - b;
        assert_eq!(diff.as_u32(), 5);
    }

    #[test]
    fn stmt_offset_minus_len_is_offset() {
        let o = StmtOffset::from_raw(15);
        let l = StmtLen::from_raw(5);
        let r: StmtOffset = o - l;
        assert_eq!(r.as_u32(), 10);
    }

    #[test]
    fn stmt_len_plus_len_is_len() {
        let a = StmtLen::from_raw(3);
        let b = StmtLen::from_raw(4);
        let r: StmtLen = a + b;
        assert_eq!(r.as_u32(), 7);
    }

    #[test]
    fn stmt_range_from_offset_len() {
        let r = StmtRange::from_offset_len(
            StmtOffset::from_raw(10),
            StmtLen::from_raw(5),
        );
        assert_eq!(r.start.as_u32(), 10);
        assert_eq!(r.end.as_u32(), 15);
        assert_eq!(r.len().as_u32(), 5);
        assert!(!r.is_empty());
    }

    #[test]
    fn empty_range() {
        let r = StmtRange::from_offset_len(
            StmtOffset::from_raw(10),
            StmtLen::from_raw(0),
        );
        assert!(r.is_empty());
    }

    #[test]
    fn statement_base_to_doc() {
        let base = StatementBase::new(DocOffset::from_raw(100));
        let off = StmtOffset::from_raw(7);
        let doc: DocOffset = base.to_doc(off);
        assert_eq!(doc.as_u32(), 107);
    }

    #[test]
    fn statement_base_to_doc_range() {
        let base = StatementBase::new(DocOffset::from_raw(100));
        let r = StmtRange::from_offset_len(
            StmtOffset::from_raw(5),
            StmtLen::from_raw(3),
        );
        let doc = base.to_doc_range(r);
        assert_eq!(doc.start.as_u32(), 105);
        assert_eq!(doc.end.as_u32(), 108);
    }

    #[test]
    fn statement_base_from_doc_in_range() {
        let base = StatementBase::new(DocOffset::from_raw(100));
        let doc = DocOffset::from_raw(107);
        assert_eq!(base.from_doc(doc).map(StmtOffset::as_u32), Some(7));
    }

    #[test]
    fn statement_base_from_doc_before_base() {
        let base = StatementBase::new(DocOffset::from_raw(100));
        let doc = DocOffset::from_raw(50);
        assert!(base.from_doc(doc).is_none());
    }

    #[test]
    fn doc_arithmetic_mirrors_stmt() {
        let o = DocOffset::from_raw(20);
        let l = DocLen::from_raw(6);
        assert_eq!((o + l).as_u32(), 26);
        let diff: DocLen = (o + l) - o;
        assert_eq!(diff.as_u32(), 6);
    }

    #[test]
    fn stmt_text_slice_by_range_returns_str() {
        let t = StmtText::new("SELECT 1");
        let r = StmtRange::from_offset_len(StmtOffset::from_raw(0), StmtLen::from_raw(6));
        let s: &str = &t[r];
        assert_eq!(s, "SELECT");
    }

    #[test]
    fn stmt_text_basic_queries() {
        let t = StmtText::new("SELECT 1");
        assert_eq!(t.byte_len().as_u32(), 8);
        assert!(!t.is_empty());
        assert_eq!(t.as_range().start.as_u32(), 0);
        assert_eq!(t.as_range().end.as_u32(), 8);
    }

    #[test]
    fn doc_text_slice_by_doc_range() {
        let d = DocText::new("create table t; select 1");
        let r = DocRange::from_offset_len(DocOffset::from_raw(16), DocLen::from_raw(8));
        assert_eq!(&d[r], "select 1");
    }

    #[test]
    fn layer_text_slice_by_layer_range() {
        let l = LayerText::new("(inner)");
        let r = LayerRange::from_offset_len(LayerOffset::from_raw(1), LayerLen::from_raw(5));
        assert_eq!(&l[r], "inner");
    }

    #[test]
    fn text_wrapping_is_zero_cost() {
        let s = "SELECT 1";
        let t = StmtText::new(s);
        // `#[repr(transparent)]`: pointer-identical.
        assert_eq!(
            std::ptr::from_ref::<str>(s).cast::<u8>(),
            std::ptr::from_ref::<StmtText>(t).cast::<u8>(),
        );
    }

    #[test]
    fn text_equality_with_str() {
        let t = StmtText::new("SELECT 1");
        assert_eq!(t, "SELECT 1");
        assert_eq!("SELECT 1", t.as_str());
    }

    #[test]
    fn types_are_expected_size() {
        assert_eq!(size_of::<StmtOffset>(), 4);
        assert_eq!(size_of::<StmtLen>(), 4);
        assert_eq!(size_of::<DocOffset>(), 4);
        assert_eq!(size_of::<DocLen>(), 4);
        assert_eq!(size_of::<LayerOffset>(), 4);
        assert_eq!(size_of::<LayerLen>(), 4);
        assert_eq!(size_of::<TokenIdx>(), 4);
        assert_eq!(size_of::<LineNumber>(), 4);
        assert_eq!(size_of::<ColumnNumber>(), 4);
        assert_eq!(size_of::<Utf16Line>(), 4);
        assert_eq!(size_of::<Utf16Col>(), 4);
        assert_eq!(size_of::<StmtRange>(), 8);
        assert_eq!(size_of::<DocRange>(), 8);
        assert_eq!(size_of::<LayerRange>(), 8);
        assert_eq!(size_of::<StatementBase>(), 4);
    }

    #[test]
    fn ordering() {
        assert!(StmtOffset::from_raw(1) < StmtOffset::from_raw(2));
        assert!(DocOffset::from_raw(1) < DocOffset::from_raw(2));
        assert!(TokenIdx::from_raw(1) < TokenIdx::from_raw(2));
    }

    #[test]
    fn debug_format() {
        let o = StmtOffset::from_raw(42);
        assert_eq!(format!("{o:?}"), "StmtOffset(42)");
    }
}
