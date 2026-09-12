//! Isolated experiment: documents from Lemon reductions, text from shifted tokens.
//! No AST traversal, emitted-keyword matching, or source-role annotations.
#![allow(clippy::all, clippy::pedantic, missing_docs, unsafe_code)]
use super::{
    FormatConfig,
    doc::{DocArena, DocId, NIL_DOC, RenderBuffers},
};
use std::ffi::{CStr, c_char, c_void};
use syntaqlite_syntax::typed::{Dialect, TypedParser, dialect};
use syntaqlite_syntax::{ParseOutcome, Tokenizer};

#[path = "token_layout_rules.rs"]
mod rules;

#[derive(Clone, Copy, PartialEq, Eq)]
enum Shape {
    Atom,
    List,
    Chain(&'static str),
}
#[derive(Clone, Copy)]
enum Break {
    Tight,
    Space,
    Line,
    Soft,
    Hard,
    Blank,
}
#[derive(Clone, Copy)]
struct Gap {
    doc: DocId,
    ends_line: bool,
    blank_line: bool,
}
impl Gap {
    const EMPTY: Self = Self {
        doc: NIL_DOC,
        ends_line: false,
        blank_line: false,
    };
}

type Callback = unsafe extern "C" fn(*mut c_void, u32, *const c_char, u32, *const c_char, u32);
unsafe extern "C" {
    fn SynqSqliteParseLayoutSet(context: *mut c_void, callback: Option<Callback>);
}
#[derive(Clone, Copy)]
struct Fragment<'a> {
    doc: DocId,
    gap: Gap,
    shape: Shape,
    trailing_prefix: bool,
    first: &'a str,
    last: &'a str,
    symbol: &'static str,
}
impl Fragment<'_> {
    fn empty(symbol: &'static str) -> Self {
        Self {
            doc: NIL_DOC,
            gap: Gap::EMPTY,
            shape: Shape::Atom,
            trailing_prefix: false,
            first: "",
            last: "",
            symbol,
        }
    }
}
#[derive(Default, Debug, Clone, Copy)]
pub(crate) struct Stats {
    pub(crate) shifts: usize,
    pub(crate) reductions: usize,
    pub(crate) max_stack: usize,
}
struct State<'a> {
    source: &'a str,
    tokenizer: &'a Tokenizer,
    end: usize,
    arena: DocArena<'a>,
    stack: Vec<Fragment<'a>>,
    scratch: Vec<Fragment<'a>>,
    error: Option<String>,
    stats: Stats,
}
impl<'a> State<'a> {
    // Only trivia is read between lexer-provided token spans. SQL is never
    // recognized here: structural context comes exclusively from reductions.
    fn trivia(&mut self, end: usize) -> Gap {
        let gap = &self.source[self.end..end];
        if gap.bytes().all(|b| b.is_ascii_whitespace()) {
            self.end = end;
            return Gap::EMPTY;
        }
        let mut doc = NIL_DOC;
        let mut ends_line = false;
        let mut newlines = 0;
        for token in self.tokenizer.tokenize(gap) {
            let text = token.text();
            if text.trim().is_empty() {
                newlines += text.bytes().filter(|b| *b == b'\n').count();
                continue;
            }
            let separator = if self.end == 0 && doc == NIL_DOC {
                NIL_DOC
            } else if newlines >= 2 {
                let line = self.arena.hardline();
                self.arena.cat(line, line)
            } else if ends_line {
                self.arena.hardline()
            } else {
                self.arena.text(" ")
            };
            let comment = self.arena.text(text);
            doc = self.arena.cats(&[doc, separator, comment]);
            // Defer the final mandatory break to the receiving layout boundary.
            // That boundary, not the preceding token, owns the next indentation.
            ends_line = text.starts_with("--");
            newlines = 0;
        }
        self.end = end;
        Gap {
            doc,
            ends_line,
            blank_line: newlines >= 2 && doc != NIL_DOC,
        }
    }

    fn shift(&mut self, symbol: &'static str, start: usize, len: usize) {
        self.stats.shifts += 1;
        if len == 0 {
            self.stack.push(Fragment::empty(symbol));
            return;
        }
        if start + len > self.source.len() || start < self.end {
            self.error = Some("non-authored or non-monotone token: outside prototype scope".into());
            return;
        }
        let prefix = self.trivia(start);
        let text = &self.source[start..start + len];
        // Preserve spelling as well as order in this experiment. Keyword casing
        // would need parser-resolved keyword/identifier classification.
        let token = self.arena.text(text);
        let doc = token;
        self.stack.push(Fragment {
            doc,
            gap: prefix,
            shape: Shape::Atom,
            trailing_prefix: false,
            first: text,
            last: text,
            symbol,
        });
        self.end = start + len;
        self.stats.max_stack = self.stats.max_stack.max(self.stack.len());
    }
    fn break_doc(&mut self, boundary: Break, gap: Gap) -> DocId {
        if gap.blank_line || matches!(boundary, Break::Blank) {
            let line = self.arena.hardline();
            return self.arena.cat(line, line);
        }
        if gap.ends_line {
            return self.arena.hardline();
        }
        match boundary {
            Break::Tight => NIL_DOC,
            Break::Space => self.arena.text(" "),
            Break::Line => self.arena.line(),
            Break::Soft => self.arena.softline(),
            Break::Hard => self.arena.hardline(),
            Break::Blank => {
                let a = self.arena.hardline();
                self.arena.cat(a, a)
            }
        }
    }
    fn content(&mut self, child: Fragment<'a>, enclosing: Shape) -> DocId {
        if child.shape == Shape::Atom || child.shape == enclosing {
            child.doc
        } else {
            let doc = if matches!((child.shape, enclosing), (Shape::Chain(_), Shape::Chain(_))) {
                self.arena.nest(1, child.doc)
            } else {
                child.doc
            };
            self.arena.group(doc)
        }
    }
    fn append(
        &mut self,
        left: Fragment<'a>,
        right: Fragment<'a>,
        boundary: Break,
        shape: Shape,
    ) -> Fragment<'a> {
        if right.doc == NIL_DOC {
            return left;
        }
        if left.doc == NIL_DOC {
            return Fragment {
                doc: self.content(right, shape),
                shape,
                ..right
            };
        }
        let separator = self.break_doc(boundary, right.gap);
        let right_doc = self.content(right, shape);
        Fragment {
            doc: self
                .arena
                .cats(&[left.doc, right.gap.doc, separator, right_doc]),
            last: right.last,
            trailing_prefix: right.trailing_prefix,
            shape,
            ..left
        }
    }
    fn sequence(
        &mut self,
        lhs: &'static str,
        children: &[Fragment<'a>],
        shape: Shape,
    ) -> Fragment<'a> {
        let mut out = Fragment::empty(lhs);
        for (index, &child) in children.iter().enumerate() {
            let boundary = if index == 0 {
                Break::Tight
            } else {
                rules::boundary(lhs, children, index, shape)
            };
            out = self.append(out, child, boundary, shape);
        }
        out.symbol = lhs;
        out
    }
    fn grouped(&mut self, mut fragment: Fragment<'a>) -> Fragment<'a> {
        fragment.doc = self.arena.group(fragment.doc);
        fragment.shape = Shape::Atom;
        fragment
    }
    // Hanging layout belongs to a syntactic owner, never to recursive list steps.
    fn hanging(&mut self, head: Fragment<'a>, body: Fragment<'a>) -> Fragment<'a> {
        if head.doc == NIL_DOC {
            return self.grouped(body);
        }
        if body.doc == NIL_DOC {
            return self.grouped(head);
        }
        let line = self.break_doc(Break::Line, body.gap);
        let contents = self.arena.cats(&[line, body.doc]);
        let nested = self.arena.nest(1, contents);
        let doc = self.arena.cats(&[head.doc, body.gap.doc, nested]);
        let out = Fragment {
            doc,
            last: body.last,
            shape: Shape::Atom,
            ..head
        };
        self.grouped(out)
    }
    // A suffix chooses its break using the actual final column of its head.
    // A multiline head must not force a short alias onto another line.
    fn suffix(&mut self, head: Fragment<'a>, body: Fragment<'a>) -> Fragment<'a> {
        if head.doc == NIL_DOC {
            return self.grouped(body);
        }
        if body.doc == NIL_DOC {
            return self.grouped(head);
        }
        let line = self.break_doc(Break::Line, body.gap);
        let tail = self.arena.cats(&[line, body.doc]);
        let tail = self.arena.nest(1, tail);
        let tail = self.arena.group(tail);
        Fragment {
            doc: self.arena.cats(&[head.doc, body.gap.doc, tail]),
            last: body.last,
            shape: Shape::Atom,
            ..head
        }
    }
    // An enclosure consumes original opening/closing tokens. No delimiters are
    // synthesized. The body owns one indentation level and the closing gap.
    fn enclosure(
        &mut self,
        open: Fragment<'a>,
        body: Fragment<'a>,
        close: Fragment<'a>,
        hard: bool,
    ) -> Fragment<'a> {
        if body.doc == NIL_DOC {
            return self.append(open, close, Break::Tight, Shape::Atom);
        }
        let boundary = if hard { Break::Hard } else { Break::Soft };
        let before = self.break_doc(boundary, body.gap);
        let after = self.break_doc(boundary, close.gap);
        let inside = self.arena.cats(&[before, body.doc, close.gap.doc]);
        let nested = self.arena.nest(1, inside);
        let doc = self
            .arena
            .cats(&[open.doc, body.gap.doc, nested, after, close.doc]);
        let result = Fragment {
            doc,
            last: close.last,
            symbol: "enclosure",
            shape: Shape::Atom,
            ..open
        };
        self.grouped(result)
    }
    fn delimiters(&mut self, children: &mut Vec<Fragment<'a>>) {
        let mut index = 0;
        while index < children.len() {
            if children[index].symbol != "LP" {
                index += 1;
                continue;
            }
            let mut depth = 1;
            let mut end = index + 1;
            while end < children.len() {
                if children[end].symbol == "LP" {
                    depth += 1;
                }
                if children[end].symbol == "RP" {
                    depth -= 1;
                }
                if depth == 0 {
                    break;
                }
                end += 1;
            }
            if end == children.len() {
                index += 1;
                continue;
            }
            let body = self.sequence("enclosure", &children[index + 1..end], Shape::List);
            let enclosed = self.enclosure(children[index], body, children[end], false);
            children.splice(index..=end, [enclosed]);
            index += 1;
        }
    }
    fn reduce(&mut self, rule: &'static str, count: usize) {
        self.stats.reductions += 1;
        if count > self.stack.len() {
            self.error = Some(format!("shadow stack underflow: {rule}"));
            return;
        }
        let lhs = rule.split_once(" ::=").map_or(rule, |(lhs, _)| lhs);
        let mut children = std::mem::take(&mut self.scratch);
        children.clear();
        children.extend(
            self.stack
                .drain(self.stack.len() - count..)
                .filter(|f| f.doc != NIL_DOC),
        );
        self.delimiters(&mut children);
        let mut result = rules::layout(self, lhs, rule, &mut children);
        result.symbol = lhs;
        self.stack.push(result);
        children.clear();
        self.scratch = children;
    }
}
unsafe extern "C" fn event(
    context: *mut c_void,
    kind: u32,
    name: *const c_char,
    count: u32,
    _text: *const c_char,
    len: u32,
) {
    // SAFETY: Registered only for the synchronous parse on this thread; the
    // state remains at this address and no other Rust reference accesses it
    // while this callback executes.
    let state = unsafe { &mut *context.cast::<State<'_>>() };
    if state.error.is_some() {
        return;
    }
    let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        // SAFETY: Names are NUL-terminated ASCII in static generated C tables.
        let name: &'static str = unsafe { CStr::from_ptr(name) }
            .to_str()
            .expect("grammar ASCII");
        if kind == 0 {
            state.shift(name, count as usize, len as usize);
        } else {
            state.reduce(name, count as usize);
        }
    }));
    if result.is_err() {
        state.error = Some("prototype callback panicked".into());
    }
}
struct Registration;
impl Drop for Registration {
    fn drop(&mut self) {
        // SAFETY: Clearing the thread-local callback retains no Rust pointer.
        unsafe { SynqSqliteParseLayoutSet(std::ptr::null_mut(), None) }
    }
}

pub(crate) struct TokenFormatter {
    parser: TypedParser<Dialect>,
    tokenizer: Tokenizer,
    buffers: RenderBuffers,
    arena: Option<DocArena<'static>>,
}
impl Default for TokenFormatter {
    fn default() -> Self {
        Self {
            parser: TypedParser::new(dialect()),
            tokenizer: Tokenizer::new(),
            buffers: RenderBuffers::new(),
            arena: None,
        }
    }
}
impl TokenFormatter {
    pub(crate) fn with_dialect(dialect: Dialect) -> Self {
        Self {
            parser: TypedParser::new(dialect),
            tokenizer: Tokenizer::new(),
            buffers: RenderBuffers::new(),
            arena: None,
        }
    }
    pub(crate) fn format(&mut self, source: &str, width: u32) -> Result<(String, Stats), String> {
        let arena = self
            .arena
            .take()
            .map_or_else(DocArena::new, DocArena::recycle);
        let mut state = State {
            source,
            tokenizer: &self.tokenizer,
            end: 0,
            arena,
            stack: Vec::new(),
            scratch: Vec::new(),
            error: None,
            stats: Stats::default(),
        };
        // SAFETY: State remains live and stationary during synchronous parsing.
        // Registration clears the callback before state can be dropped.
        unsafe { SynqSqliteParseLayoutSet((&raw mut state).cast(), Some(event)) };
        let registration = Registration;
        let mut session = self.parser.parse(source);
        let mut root = Fragment::empty("input");
        loop {
            match session.next() {
                ParseOutcome::Ok(_) => {
                    // The runtime restarts Lemon after each statement. Concatenate
                    // the remaining stack documents in their original order.
                    let mut statement = Fragment::empty("statement");
                    let stack = std::mem::take(&mut state.stack);
                    for &fragment in &stack {
                        let boundary = if statement.last == ";" && fragment.first != ";" {
                            Break::Blank
                        } else {
                            Break::Tight
                        };
                        statement = state.append(statement, fragment, boundary, Shape::Atom);
                    }
                    root = state.append(root, statement, Break::Blank, Shape::Atom);
                    state.stack = stack;
                    state.stack.clear();
                }
                ParseOutcome::Done => break,
                ParseOutcome::Err(e) => {
                    state.error = Some(e.message().to_owned());
                    break;
                }
            }
            if state.error.is_some() {
                break;
            }
        }
        drop(registration);
        if let Some(error) = state.error {
            return Err(error);
        }
        let remaining = std::mem::take(&mut state.stack);
        for fragment in remaining {
            root = state.append(root, fragment, Break::Tight, Shape::Atom);
        }
        // A statement terminator may be consumed by the runtime without shifting
        // into Lemon. Preserve any such remaining lexical text verbatim here.
        let tail = state.trivia(source.len());
        let header = state.break_doc(
            if root.gap.doc == NIL_DOC {
                Break::Tight
            } else {
                Break::Hard
            },
            root.gap,
        );
        let footer = state.break_doc(Break::Tight, tail);
        let document = state
            .arena
            .cats(&[root.gap.doc, header, root.doc, tail.doc, footer]);
        let config = FormatConfig::default().with_line_width(width as usize);
        self.buffers.clear();
        state
            .arena
            .render_into(document, &config, &mut self.buffers);
        let output = self.buffers.out.clone();
        self.arena = Some(DocArena::recycle(state.arena));
        Ok((output, state.stats))
    }
}
