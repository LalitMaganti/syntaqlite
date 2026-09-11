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

type Callback = unsafe extern "C" fn(*mut c_void, u32, *const c_char, u32, *const c_char, u32);
unsafe extern "C" {
    fn SynqSqliteParseLayoutSet(context: *mut c_void, callback: Option<Callback>);
}
#[derive(Clone, Copy)]
struct Fragment<'a> {
    doc: DocId,
    gap: DocId,
    first: &'a str,
    last: &'a str,
    symbol: &'static str,
}
impl Fragment<'_> {
    fn empty(symbol: &'static str) -> Self {
        Self {
            doc: NIL_DOC,
            gap: NIL_DOC,
            first: "",
            last: "",
            symbol,
        }
    }
}
#[derive(Default, Debug, Clone, Copy)]
pub struct Stats {
    pub shifts: usize,
    pub reductions: usize,
    pub max_stack: usize,
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
    fn trivia(&mut self, end: usize) -> DocId {
        let gap = &self.source[self.end..end];
        if gap.bytes().all(|b| b.is_ascii_whitespace()) {
            self.end = end;
            return NIL_DOC;
        }
        let mut doc = NIL_DOC;
        for token in self.tokenizer.tokenize(gap) {
            let text = token.text();
            if text.trim().is_empty() {
                continue;
            }
            let sep = if self.end == 0 && doc == NIL_DOC {
                NIL_DOC
            } else {
                self.arena.text(" ")
            };
            let comment = self.arena.text(text);
            doc = self.arena.cats(&[doc, sep, comment]);
            // A line comment must end before any following SQL, independently
            // of which grammar production eventually wraps this fragment.
            let after = if text.starts_with("--") {
                self.arena.comment_break()
            } else {
                NIL_DOC
            };
            doc = self.arena.cat(doc, after);
        }
        self.end = end;
        doc
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
            first: text,
            last: text,
            symbol,
        });
        self.end = start + len;
        self.stats.max_stack = self.stats.max_stack.max(self.stack.len());
    }
    fn separator(&mut self, left: Fragment<'a>, right: Fragment<'a>, lhs: &str) -> DocId {
        if [",", ")", ";", "."].contains(&right.first) || left.last == "." {
            return NIL_DOC;
        }
        if left.last == "(" || right.first == "(" {
            return NIL_DOC;
        }
        // Lists and clauses supply break opportunities based on grammar slots.
        // These are layout choices, not mappings between source and output.
        if left.last == ","
            || (lhs == "oneselect"
                && matches!(
                    right.symbol,
                    "from"
                        | "where_opt"
                        | "groupby_opt"
                        | "having_opt"
                        | "window_clause"
                        | "orderby_opt"
                        | "limit_opt"
                ))
        {
            return self.arena.line();
        }
        if lhs == "expr" && left.symbol == "expr" && right.symbol != "expr" {
            return self.arena.line();
        }
        self.arena.text(" ")
    }
    fn reduce(&mut self, rule: &'static str, count: usize) {
        self.stats.reductions += 1;
        if count > self.stack.len() {
            self.error = Some(format!("shadow stack underflow: {rule}"));
            return;
        }
        let lhs = rule.split_once(" ::=").map_or(rule, |(lhs, _)| lhs);
        self.scratch.clear();
        self.scratch
            .extend(self.stack.drain(self.stack.len() - count..));
        let mut result = Fragment::empty(lhs);
        // Collapse each reduction directly to a document: no reduction tree.
        for index in 0..self.scratch.len() {
            let child = self.scratch[index];
            if child.doc == NIL_DOC {
                continue;
            }
            if result.doc == NIL_DOC {
                result = Fragment {
                    symbol: lhs,
                    ..child
                };
            } else {
                let previous = self.scratch[..index]
                    .iter()
                    .rev()
                    .find(|f| f.doc != NIL_DOC)
                    .copied()
                    .expect("nonempty result has a preceding nonempty fragment");
                let sep = self.separator(previous, child, lhs);
                result.doc = self.arena.cats(&[result.doc, child.gap, sep, child.doc]);
                result.last = child.last;
            }
        }
        if count > 1 && result.doc != NIL_DOC {
            // Nest only expression/list bodies, not every grammar wrapper.
            if matches!(lhs, "expr" | "selcollist" | "exprlist" | "sortlist") {
                result.doc = self.arena.nest(1, result.doc);
            }
            result.doc = self.arena.group(result.doc);
        }
        self.stack.push(result);
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

pub struct TokenFormatter {
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
    pub fn with_dialect(dialect: Dialect) -> Self {
        Self {
            parser: TypedParser::new(dialect),
            tokenizer: Tokenizer::new(),
            buffers: RenderBuffers::new(),
            arena: None,
        }
    }
    pub fn format(&mut self, source: &str, width: u32) -> Result<(String, Stats), String> {
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
        let mut root = NIL_DOC;
        loop {
            match session.next() {
                ParseOutcome::Ok(_) => {
                    // The runtime restarts Lemon after each statement. Concatenate
                    // the remaining stack documents in their original order.
                    for fragment in &state.stack {
                        root = state.arena.cats(&[root, fragment.gap, fragment.doc]);
                    }
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
        for fragment in &state.stack {
            root = state.arena.cats(&[root, fragment.gap, fragment.doc]);
        }
        // A statement terminator may be consumed by the runtime without shifting
        // into Lemon. Preserve any such remaining lexical text verbatim here.
        let tail = state.arena.text(&source[state.end..]);
        root = state.arena.cat(root, tail);
        let config = FormatConfig::default().with_line_width(width as usize);
        self.buffers.clear();
        state.arena.render_into(root, &config, &mut self.buffers);
        let output = self.buffers.out.clone();
        self.arena = Some(DocArena::recycle(state.arena));
        Ok((output, state.stats))
    }
}
