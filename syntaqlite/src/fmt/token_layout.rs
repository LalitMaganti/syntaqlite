//! Documents from grammar reductions and authored token spans.
//! No AST traversal, emitted-keyword matching, or source-role annotations.
#![expect(
    unsafe_code,
    reason = "scoped C parser observer; pointer invariants are documented at each call"
)]
use super::doc::{DocArena, DocId, NIL_DOC};
use super::{FormatError, Formatter};
use std::ffi::{CStr, c_char, c_void};
use syntaqlite_syntax::any::{AnyTokenizer, ParseOutcome, TokenCategory};
use syntaqlite_syntax::source::{DocLen, DocOffset, DocRange};

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
    fn synq_parse_layout_set(context: *mut c_void, callback: Option<Callback>);
}
#[derive(Clone, Copy)]
struct Fragment {
    doc: DocId,
    gap: Gap,
    shape: Shape,
    trailing_prefix: bool,
    first: DocRange,
    last: DocRange,
    symbol: &'static str,
}
impl Fragment {
    fn empty(symbol: &'static str) -> Self {
        Self {
            doc: NIL_DOC,
            gap: Gap::EMPTY,
            shape: Shape::Atom,
            trailing_prefix: false,
            first: DocRange::default(),
            last: DocRange::default(),
            symbol,
        }
    }
}
pub(super) struct State<'a> {
    source: &'a str,
    tokenizer: AnyTokenizer,
    tokens: Vec<(usize, DocId, u32)>,
    terminator: Option<DocId>,
    end: usize,
    arena: DocArena<'a>,
    stack: Vec<Fragment>,
    scratch: Vec<Fragment>,
    error: Option<String>,
}
impl State<'_> {
    pub(super) fn new(tokenizer: AnyTokenizer) -> Self {
        Self {
            source: "",
            tokenizer,
            tokens: Vec::new(),
            terminator: None,
            end: 0,
            arena: DocArena::with_capacity(256),
            stack: Vec::new(),
            scratch: Vec::new(),
            error: None,
        }
    }
    fn recycle(mut self, source: &str) -> State<'_> {
        self.tokens.clear();
        self.stack.clear();
        self.scratch.clear();
        State {
            source,
            arena: DocArena::recycle(self.arena),
            end: 0,
            terminator: None,
            error: None,
            ..self
        }
    }

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
            if text == ";" {
                continue;
            }
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

    fn shift(&mut self, symbol: &'static str, start: u32, len: u32) {
        let span = DocRange::from_offset_len(DocOffset::from_raw(start), DocLen::from_raw(len));
        let start = start as usize;
        let len = len as usize;
        if len == 0 {
            self.stack.push(Fragment::empty(symbol));
            return;
        }
        if start + len > self.source.len() || start < self.end {
            self.error =
                Some("non-authored or non-monotone token: invalid parser layout span".into());
            return;
        }
        let prefix = self.trivia(start);
        let text = &self.source[start..start + len];
        // Tokens are placeholders until statement completion supplies the parser
        // flags distinguishing keywords from names.
        let token = self.arena.text(text);
        self.tokens.push((start, token, span.len().as_u32()));
        self.terminator = (symbol == "SEMI").then_some(token);
        let doc = token;
        self.stack.push(Fragment {
            doc,
            gap: prefix,
            shape: Shape::Atom,
            trailing_prefix: false,
            first: span,
            last: span,
            symbol,
        });
        self.end = start + len;
    }
    fn verbatim(&mut self, lhs: &'static str, children: &[Fragment], keywords: bool) -> Fragment {
        let mut result = children[0];
        result.last = children.last().expect("nonempty production").last;
        let start = result.first.start.as_usize();
        let end = result.last.end.as_usize();
        if keywords {
            // Preserve type spelling/spacing, but keep keyword placeholders live:
            // semantic disambiguation can remove keywords from the type span.
            let first = self.tokens.partition_point(|(offset, ..)| *offset < start);
            let mut cursor = start;
            let mut doc = NIL_DOC;
            for &(offset, token, token_len) in &self.tokens[first..] {
                if offset >= end {
                    break;
                }
                let gap = self.arena.text(&self.source[cursor..offset]);
                doc = self.arena.cats(&[doc, gap, token]);
                cursor = offset + token_len as usize;
            }
            let tail = self.arena.text(&self.source[cursor..end]);
            result.doc = self.arena.cat(doc, tail);
        } else {
            result.doc = self.arena.text(&self.source[start..end]);
        }
        result.symbol = lhs;
        result.shape = Shape::Atom;
        result
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
    fn content(&mut self, child: Fragment, enclosing: Shape) -> DocId {
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
        left: Fragment,
        right: Fragment,
        boundary: Break,
        shape: Shape,
    ) -> Fragment {
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
    fn sequence(&mut self, lhs: &'static str, children: &[Fragment], shape: Shape) -> Fragment {
        let mut out = Fragment::empty(lhs);
        for (index, &child) in children.iter().enumerate() {
            let boundary = if index == 0 {
                Break::Tight
            } else {
                rules::boundary(self.source, lhs, children, index, shape)
            };
            out = self.append(out, child, boundary, shape);
        }
        out.symbol = lhs;
        out
    }
    fn grouped(&mut self, mut fragment: Fragment) -> Fragment {
        fragment.doc = self.arena.group(fragment.doc);
        fragment.shape = Shape::Atom;
        fragment
    }
    // Hanging layout belongs to a syntactic owner, never to recursive list steps.
    fn hanging(&mut self, head: Fragment, body: Fragment) -> Fragment {
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
    fn suffix(&mut self, head: Fragment, body: Fragment) -> Fragment {
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
        open: Fragment,
        body: Fragment,
        close: Fragment,
        hard: bool,
    ) -> Fragment {
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
    fn delimiters(&mut self, children: &mut Vec<Fragment>) {
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
            state.shift(name, count, len);
        } else {
            state.reduce(name, count as usize);
        }
    }));
    if result.is_err() {
        state.error = Some("parser layout callback panicked".into());
    }
}
struct Registration;
impl Drop for Registration {
    fn drop(&mut self) {
        // SAFETY: Clearing the thread-local callback retains no Rust pointer.
        unsafe { synq_parse_layout_set(std::ptr::null_mut(), None) }
    }
}

pub(super) fn format(
    owner: &mut Formatter,
    source: &str,
    dump: bool,
) -> Result<String, FormatError> {
    let mut state = owner
        .layout
        .take()
        .expect("formatter is not reentrant")
        .recycle(source);
    let result = format_inner(owner, &mut state, dump);
    // Reclaim buffers on errors too. No references to this input survive reuse.
    owner.layout = Some(state.recycle(""));
    result
}

#[expect(
    clippy::too_many_lines,
    reason = "keep the parse, classify and render lifecycle together"
)]
fn format_inner(
    owner: &mut Formatter,
    state: &mut State<'_>,
    dump: bool,
) -> Result<String, FormatError> {
    let source = state.source;
    let mut session = owner.parser.parse(source);
    let mut root = Fragment::empty("input");
    loop {
        // SAFETY: The observer borrows stationary state only during next(). It
        // is removed before macro mini-parses or any other Rust work resumes.
        unsafe {
            synq_parse_layout_set(std::ptr::from_mut::<State<'_>>(state).cast(), Some(event));
        };
        let registration = Registration;
        let outcome = session.next();
        drop(registration);
        let stmt = match outcome {
            ParseOutcome::Done => break,
            ParseOutcome::Err(error) => {
                return Err(super::formatter::parse_error_to_format_error(&error));
            }
            ParseOutcome::Ok(stmt) => stmt,
        };
        if let Some(error) = state.error.take() {
            return Err(FormatError::new(error, None));
        }
        let erased = stmt.erase();
        if state.stack.is_empty() && !erased.root_id().is_null() {
            return Err(FormatError::new(
                "dialect parser has no layout events; regenerate the dialect".into(),
                None,
            ));
        }
        let base = erased.statement_base();
        let mut slots = state.tokens.iter().peekable();
        for token in erased.tokens() {
            let start = token.stmt_range().start.to_doc(base).as_usize();
            while slots.peek().is_some_and(|(offset, ..)| *offset < start) {
                slots.next();
            }
            if let Some(&(offset, doc, _)) = slots.peek().copied()
                && offset == start
                && owner
                    .dialect
                    .classify_token(token.token_type(), token.flags())
                    == TokenCategory::Keyword
            {
                state.arena.mark_keyword(doc);
            }
        }
        if erased.macro_rewrites().next().is_some() {
            owner.collect_side_channels(&erased);
            let macro_docs = super::macro_structured::compute_macro_docs(
                &owner.mini_parser,
                &owner.dialect,
                &erased,
                &owner.macro_tokenizer,
                &owner.comment_entries,
                &mut state.arena,
            );
            for ((offset, len), replacement) in owner.macro_rewrites.iter().zip(macro_docs) {
                let start = offset.to_doc(base).as_usize();
                if let Ok(index) = state
                    .tokens
                    .binary_search_by_key(&start, |(offset, ..)| *offset)
                {
                    let replacement = replacement.unwrap_or_else(|| {
                        super::formatter::reindent_macro(
                            &source[start..start + len.as_usize()],
                            &owner.macro_tokenizer,
                            &mut state.arena,
                        )
                    });
                    state.arena.replace(state.tokens[index].1, replacement);
                }
            }
        }
        let stack = std::mem::take(&mut state.stack);
        let mut statement = Fragment::empty("statement");
        for &fragment in &stack {
            statement = state.append(statement, fragment, Break::Tight, Shape::Atom);
        }
        if let Some(terminator) = state.terminator.take() {
            if !owner.config.semicolons() {
                let empty = state.arena.text("");
                state.arena.replace(terminator, empty);
            }
        } else if owner.config.semicolons() && !erased.root_id().is_null() {
            let semi = state.arena.text(";");
            statement.doc = state.arena.cat(statement.doc, semi);
        }
        root = state.append(root, statement, Break::Blank, Shape::Atom);
        state.stack = stack;
        state.stack.clear();
        state.tokens.clear();
    }
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
    if dump {
        return Ok(state.arena.dump(document));
    }
    owner.render_bufs.clear();
    state
        .arena
        .render_into(document, &owner.config, &mut owner.render_bufs);
    let output = &mut owner.render_bufs.out;
    output.truncate(output.trim_end().len());
    if !output.is_empty() {
        output.push('\n');
    }
    Ok(output.clone())
}
