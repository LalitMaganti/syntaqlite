// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

use syntaqlite_syntax::any::{AnyNodeId, AnyParsedStatement, FieldValue, MacroRegion};

use super::comment::{CommentCtx, DrainResult};
use super::doc::{DocArena, DocId, NIL_DOC};
use super::formatter::Formatter;
use crate::dialect::AnyDialect;
use syntaqlite_common::fmt::bytecode::opcodes;

/// Shared context threaded through the iterative formatting traversal.
pub(crate) struct FmtCtx<'a> {
    pub dialect: AnyDialect,
    pub reader: AnyParsedStatement<'a>,
    /// Owned comment context — no lifetime needed since `CommentCtx` owns its data.
    pub comment_ctx: Option<CommentCtx>,
    pub macro_regions: Vec<MacroRegion>,
}

impl<'a> FmtCtx<'a> {
    pub(crate) fn source(&self) -> &'a str {
        self.reader.source()
    }
}

/// Reusable scratch buffers for the iterative interpret loop.
pub(super) struct InterpretScratch {
    group_nest: Vec<GroupNestFrame>,
    calls: Vec<CallFrame>,
    for_each: Vec<ForEachState>,
}

impl InterpretScratch {
    pub(super) fn new() -> Self {
        InterpretScratch {
            group_nest: Vec::new(),
            calls: Vec::new(),
            for_each: Vec::new(),
        }
    }
}

// ── Iterative interpreter ────────────────────────────────────────────────

struct CallFrame {
    ip: usize,
    node_id: AnyNodeId,
    running: DocId,
    pending: DocId,
    gn_save: usize,
    fe_save: usize,
    return_action: ReturnAction,
}

#[derive(PartialEq)]
enum ReturnAction {
    CatOntoRunning,
    /// Wrap child output in parentheses before appending to parent.
    WrapInParens,
    Discard,
}

#[expect(clippy::too_many_lines)]
impl Formatter {
    pub(super) fn interpret_node<'a>(
        &mut self,
        ctx: &FmtCtx<'a>,
        root_id: AnyNodeId,
        arena: &mut DocArena<'a>,
    ) -> DocId {
        self.consumed_regions.clear();
        self.consumed_regions.resize(ctx.macro_regions.len(), false);
        let consumed_regions = &mut self.consumed_regions;
        let scratch = &mut self.interpret_scratch;
        let macro_tokenizer = &self.macro_tokenizer;

        if root_id.is_null() {
            return NIL_DOC;
        }

        let source = ctx.source();
        let Some((tag, fields)) = ctx.reader.extract_fields(root_id) else {
            return NIL_DOC;
        };
        let Some((ops_bytes, ops_len)) = ctx.dialect.fmt_dispatch(tag) else {
            return NIL_DOC;
        };
        scratch.group_nest.clear();
        scratch.calls.clear();
        scratch.for_each.clear();

        let mut cur_node_id: AnyNodeId = root_id;
        let mut ops: &[u8] = &ops_bytes[..ops_len * 6];
        let mut ops_count: usize = ops_len;
        let mut fields = fields;
        let mut running: DocId = NIL_DOC;
        let mut pending: DocId = NIL_DOC;
        let mut gn_save = scratch.group_nest.len();
        let mut fe_save = scratch.for_each.len();
        let mut ip: usize = 0;
        let has_comments = ctx.comment_ctx.is_some();

        macro_rules! push_call_frame {
            ($child_id:expr, $child_ops_bytes:expr, $child_ops_len:expr,
         $child_fields:expr, $return_action_val:expr) => {{
                let frame = CallFrame {
                    ip: ip + 1,
                    node_id: cur_node_id,
                    running,
                    pending,
                    gn_save,
                    fe_save,
                    return_action: $return_action_val,
                };
                scratch.calls.push(frame);

                cur_node_id = $child_id;
                ops = &$child_ops_bytes[..$child_ops_len * 6];
                ops_count = $child_ops_len;
                fields = $child_fields;
                running = NIL_DOC;
                pending = NIL_DOC;
                gn_save = scratch.group_nest.len();
                fe_save = scratch.for_each.len();
                ip = 0;
                continue;
            }};
        }

        loop {
            if ip >= ops_count {
                let result = arena.cat(running, pending);
                scratch.group_nest.truncate(gn_save);
                scratch.for_each.truncate(fe_save);

                if scratch.calls.is_empty() {
                    return result;
                }

                let frame = scratch
                    .calls
                    .pop()
                    .expect("call_stack must contain a parent frame");
                cur_node_id = frame.node_id;
                ip = frame.ip;
                let Some((restored_tag, restored_fields)) = ctx.reader.extract_fields(cur_node_id)
                else {
                    panic!("restored node must resolve to fields");
                };
                let Some((restored_ops, restored_ops_len)) = ctx.dialect.fmt_dispatch(restored_tag)
                else {
                    panic!("restored node must resolve to formatter ops");
                };
                ops = &restored_ops[..restored_ops_len * 6];
                ops_count = restored_ops_len;
                fields = restored_fields;
                running = frame.running;
                pending = frame.pending;
                gn_save = frame.gn_save;
                fe_save = frame.fe_save;

                match frame.return_action {
                    ReturnAction::CatOntoRunning => {
                        running = arena.cat(running, result);
                    }
                    ReturnAction::WrapInParens => {
                        let lp = arena.text("(");
                        let inner = arena.cat(lp, result);
                        let rp = arena.text(")");
                        let wrapped = arena.cat(inner, rp);
                        let grouped = arena.group(wrapped);
                        running = arena.cat(running, grouped);
                    }
                    ReturnAction::Discard => {}
                }
                continue;
            }

            let op = op_at(ops, ip);
            match op {
                FmtOp::Keyword(sid) => {
                    let kw_text = ctx.dialect.fmt_string(sid);

                    if let Some(ref cctx) = ctx.comment_ctx {
                        if let Some((tok_offset, word_count)) =
                            cctx.peek_keyword_tokens(kw_text, source)
                        {
                            let drain = cctx.drain_before(tok_offset, source, arena);
                            if word_count > 1 {
                                // For multi-word keywords, only flush drain_before
                                // if it found comments; otherwise defer pending to
                                // drain_keyword_interior to avoid a double hardline
                                // before interior comments.
                                if drain.trailing != NIL_DOC || drain.leading != NIL_DOC {
                                    flush_drain(&drain, &mut pending, &mut running, arena);
                                }
                                let interior =
                                    cctx.drain_keyword_interior(word_count, source, arena);
                                flush_drain(&interior, &mut pending, &mut running, arena);
                            } else {
                                flush_drain(&drain, &mut pending, &mut running, arena);
                            }
                            cctx.advance_token_cursor(word_count);
                        } else {
                            running = arena.cat(running, pending);
                            pending = NIL_DOC;
                        }
                    }
                    let kw = arena.keyword(kw_text);
                    running = arena.cat(running, kw);
                }
                FmtOp::Span(idx) => {
                    // INVARIANT: Span ops only target Span fields.
                    let FieldValue::Span { text: s, quoted } = fields[idx as usize] else {
                        panic!("Span: field {idx} is not a Span");
                    };

                    if !s.is_empty() {
                        let offset = byte_offset_in(source, s.as_ptr());
                        if let Some(ref cctx) = ctx.comment_ctx {
                            // For quoted spans, the original token in source
                            // starts one byte earlier (the opening quote) and
                            // extends one byte past (the closing quote).
                            let drain_offset = if quoted { offset - 1 } else { offset };
                            let token_len = usize_to_u32(s.len()) + if quoted { 2 } else { 0 };
                            let drain = cctx.drain_before(drain_offset, source, arena);
                            flush_drain(&drain, &mut pending, &mut running, arena);
                            cctx.advance_past(drain_offset + token_len);
                        }
                        if quoted {
                            let q = arena.text("\"");
                            let txt = arena.text(s);
                            running = arena.cat(running, q);
                            running = arena.cat(running, txt);
                            running = arena.cat(running, q);
                        } else {
                            let txt = arena.text(s);
                            running = arena.cat(running, txt);
                        }
                    }
                }
                FmtOp::Child(idx) => {
                    // INVARIANT: Child ops only target NodeId fields.
                    let FieldValue::NodeId(child_id) = fields[idx as usize] else {
                        panic!("Child: field {idx} is not a NodeId");
                    };

                    if !child_id.is_null() {
                        drain_comments_before_child(
                            ctx.comment_ctx.as_ref(),
                            source,
                            &mut pending,
                            &mut running,
                            arena,
                        );

                        let mut return_action = ReturnAction::CatOntoRunning;
                        let macro_regions = &ctx.macro_regions;
                        if !macro_regions.is_empty()
                            && ctx.reader.list_children(child_id).is_none()
                            && let Some(doc) = super::formatter::try_macro_verbatim(
                                ctx,
                                macro_regions,
                                arena,
                                consumed_regions,
                                macro_tokenizer,
                                child_id,
                            )
                        {
                            running = arena.cat(running, doc);
                            return_action = ReturnAction::Discard;
                        }

                        if let Some((ctag, child_fields)) = ctx.reader.extract_fields(child_id)
                            && let Some((child_ops_bytes, child_ops_len)) =
                                ctx.dialect.fmt_dispatch(ctag)
                        {
                            push_call_frame!(
                                child_id,
                                child_ops_bytes,
                                child_ops_len,
                                child_fields,
                                return_action
                            );
                        }
                    }
                }
                FmtOp::Line | FmtOp::SoftLine | FmtOp::HardLine => {
                    let doc = match op {
                        FmtOp::Line => arena.line(),
                        FmtOp::SoftLine => arena.softline(),
                        FmtOp::HardLine => arena.hardline(),
                        _ => unreachable!(),
                    };
                    if has_comments {
                        pending = arena.cat(pending, doc);
                    } else {
                        running = arena.cat(running, doc);
                    }
                }
                FmtOp::GroupStart => {
                    scratch.group_nest.push(GroupNestFrame::Group(running));
                    running = NIL_DOC;
                }
                FmtOp::GroupEnd => {
                    running = arena.cat(running, pending);
                    pending = NIL_DOC;
                    let inner = running;
                    match scratch.group_nest.pop().expect("unmatched GroupEnd") {
                        GroupNestFrame::Group(parent) => {
                            let g = arena.group(inner);
                            running = arena.cat(parent, g);
                        }
                        GroupNestFrame::Nest(..) => panic!("expected Group frame"),
                    }
                }
                FmtOp::NestStart(indent) => {
                    scratch
                        .group_nest
                        .push(GroupNestFrame::Nest(indent, running));
                    running = NIL_DOC;
                }
                FmtOp::NestEnd => {
                    running = arena.cat(running, pending);
                    pending = NIL_DOC;
                    let inner = running;
                    match scratch.group_nest.pop().expect("unmatched NestEnd") {
                        GroupNestFrame::Nest(indent, parent) => {
                            let n = arena.nest(indent, inner);
                            running = arena.cat(parent, n);
                        }
                        GroupNestFrame::Group(_) => panic!("expected Nest frame"),
                    }
                }
                FmtOp::IfSet(idx, skip) => {
                    // INVARIANT: IfSet ops only target NodeId fields.
                    let FieldValue::NodeId(id) = fields[idx as usize] else {
                        panic!("IfSet: field {idx} is not a NodeId");
                    };
                    if id.is_null() {
                        ip += skip as usize;
                    }
                }
                FmtOp::Else(skip) => {
                    ip += skip as usize;
                }
                FmtOp::EndIf => {}
                FmtOp::ForEachStart(idx) => {
                    // INVARIANT: ForEachStart ops only target NodeId fields.
                    let FieldValue::NodeId(list_id) = fields[idx as usize] else {
                        panic!("ForEachStart: field {idx} is not a NodeId");
                    };
                    if list_id.is_null() {
                        ip = skip_to_foreach_end(ops, ops_count, ip);
                    } else {
                        let children: &'a [AnyNodeId] =
                            ctx.reader.list_children(list_id).unwrap_or(&[]);
                        if children.is_empty() {
                            ip = skip_to_foreach_end(ops, ops_count, ip);
                        } else {
                            scratch.for_each.push(ForEachState {
                                list_id,
                                index: 0,
                                body_start: ip + 1,
                                sep_checkpoint: None,
                            });
                        }
                    }
                }
                FmtOp::ChildItem => {
                    let state = scratch.for_each.last().expect("ChildItem outside ForEach");
                    let children = ctx.reader.list_children(state.list_id).unwrap_or(&[]);
                    let child_id = children[state.index];

                    let macro_regions = &ctx.macro_regions;
                    let macro_doc = if !macro_regions.is_empty()
                        && ctx.reader.list_children(child_id).is_none()
                    {
                        super::formatter::try_macro_verbatim(
                            ctx,
                            macro_regions,
                            arena,
                            consumed_regions,
                            macro_tokenizer,
                            child_id,
                        )
                    } else {
                        None
                    };
                    let return_action;

                    if macro_doc == Some(NIL_DOC) {
                        let state = scratch
                            .for_each
                            .last_mut()
                            .expect("ForEachState must exist while handling ChildItem");
                        if let Some((saved_running, saved_pending)) = state.sep_checkpoint.take() {
                            running = saved_running;
                            pending = saved_pending;
                        }
                        return_action = ReturnAction::Discard;
                    } else {
                        drain_comments_before_child(
                            ctx.comment_ctx.as_ref(),
                            source,
                            &mut pending,
                            &mut running,
                            arena,
                        );

                        if let Some(verbatim) = macro_doc {
                            running = arena.cat(running, verbatim);
                            return_action = ReturnAction::Discard;
                        } else {
                            return_action = ReturnAction::CatOntoRunning;
                        }
                    }

                    if let Some((ctag, child_fields)) = ctx.reader.extract_fields(child_id)
                        && let Some((child_ops_bytes, child_ops_len)) =
                            ctx.dialect.fmt_dispatch(ctag)
                    {
                        push_call_frame!(
                            child_id,
                            child_ops_bytes,
                            child_ops_len,
                            child_fields,
                            return_action
                        );
                    }
                }
                FmtOp::ForEachSep(sid) => {
                    let state = scratch
                        .for_each
                        .last_mut()
                        .expect("ForEachSep outside ForEach");
                    let children = ctx.reader.list_children(state.list_id).unwrap_or(&[]);
                    if state.index < children.len() - 1 {
                        state.sep_checkpoint = Some((running, pending));
                        let sep_text = ctx.dialect.fmt_string(sid);
                        if let Some(ref cctx) = ctx.comment_ctx
                            && let Some((_, word_count)) =
                                cctx.peek_keyword_tokens(sep_text, source)
                        {
                            cctx.advance_token_cursor(word_count);
                        }
                        let sep = arena.text(sep_text);
                        running = arena.cat(running, sep);
                    } else {
                        ip = skip_to_foreach_end(ops, ops_count, ip);
                        continue;
                    }
                }
                FmtOp::ForEachEnd => {
                    let state = scratch
                        .for_each
                        .last_mut()
                        .expect("ForEachEnd outside ForEach");
                    state.index += 1;
                    let children = ctx.reader.list_children(state.list_id).unwrap_or(&[]);
                    if state.index < children.len() {
                        ip = state.body_start;
                        continue;
                    }
                    scratch.for_each.pop();
                }
                FmtOp::IfBool(idx, skip) => {
                    // INVARIANT: IfBool ops only target Bool fields.
                    let FieldValue::Bool(val) = fields[idx as usize] else {
                        panic!("IfBool: field {idx} is not a Bool");
                    };
                    if !val {
                        ip += skip as usize;
                    }
                }
                FmtOp::IfFlag(idx, mask, skip) => {
                    // INVARIANT: IfFlag ops only target Flags fields.
                    let FieldValue::Flags(f) = fields[idx as usize] else {
                        panic!("IfFlag: field {idx} is not Flags");
                    };
                    if f & mask == 0 {
                        ip += skip as usize;
                    }
                }
                FmtOp::IfEnum(idx, ordinal, skip) => {
                    // INVARIANT: IfEnum ops only target Enum fields.
                    let FieldValue::Enum(val) = fields[idx as usize] else {
                        panic!("IfEnum: field {idx} is not an Enum");
                    };
                    if val != u32::from(ordinal) {
                        ip += skip as usize;
                    }
                }
                FmtOp::IfSpan(idx, skip) => {
                    // INVARIANT: IfSpan ops only target Span fields.
                    let FieldValue::Span { text: s, .. } = fields[idx as usize] else {
                        panic!("IfSpan: field {idx} is not a Span");
                    };
                    if s.is_empty() {
                        ip += skip as usize;
                    }
                }
                FmtOp::EnumDisplay(idx, base) => {
                    // INVARIANT: EnumDisplay ops only target Enum fields.
                    let FieldValue::Enum(ordinal) = fields[idx as usize] else {
                        panic!("EnumDisplay: field {idx} is not an Enum");
                    };
                    let string_id = ctx
                        .dialect
                        .fmt_enum_display_val(base as usize + ordinal as usize);
                    let kw_text = ctx.dialect.fmt_string(string_id);
                    if let Some(ref cctx) = ctx.comment_ctx {
                        if let Some((tok_offset, word_count)) =
                            cctx.peek_keyword_tokens(kw_text, source)
                        {
                            let drain = cctx.drain_before(tok_offset, source, arena);
                            if word_count > 1 {
                                if drain.trailing != NIL_DOC || drain.leading != NIL_DOC {
                                    flush_drain(&drain, &mut pending, &mut running, arena);
                                }
                                let interior =
                                    cctx.drain_keyword_interior(word_count, source, arena);
                                flush_drain(&interior, &mut pending, &mut running, arena);
                            } else {
                                flush_drain(&drain, &mut pending, &mut running, arena);
                            }
                            cctx.advance_token_cursor(word_count);
                        } else {
                            running = arena.cat(running, pending);
                            pending = NIL_DOC;
                        }
                    }
                    let kw = arena.keyword(kw_text);
                    running = arena.cat(running, kw);
                }
                FmtOp::ForEachSelfStart => {
                    let children = ctx
                        .reader
                        .list_children(cur_node_id)
                        .expect("ForEachSelfStart on non-list node");
                    if children.is_empty() {
                        ip = skip_to_foreach_end(ops, ops_count, ip);
                    } else {
                        scratch.for_each.push(ForEachState {
                            list_id: cur_node_id,
                            index: 0,
                            body_start: ip + 1,
                            sep_checkpoint: None,
                        });
                    }
                }
                FmtOp::ChildPrec(child_idx, prec_base, packed_c) => {
                    let op_field_idx = (packed_c >> 8) as usize;
                    let is_right = (packed_c & 0xFF) != 0;

                    // Read the parent's operator ordinal.
                    let FieldValue::Enum(parent_op_ordinal) = fields[op_field_idx] else {
                        panic!("ChildPrec: op field {op_field_idx} is not an Enum");
                    };
                    let (parent_prec, parent_group_and_flags) =
                        ctx.dialect.fmt_prec_lookup(prec_base, parent_op_ordinal);

                    let FieldValue::NodeId(child_id) = fields[child_idx as usize] else {
                        panic!("ChildPrec: child field {child_idx} is not a NodeId");
                    };

                    if !child_id.is_null() {
                        drain_comments_before_child(
                            ctx.comment_ctx.as_ref(),
                            source,
                            &mut pending,
                            &mut running,
                            arena,
                        );

                        let mut return_action = ReturnAction::CatOntoRunning;
                        let macro_regions = &ctx.macro_regions;
                        if !macro_regions.is_empty()
                            && ctx.reader.list_children(child_id).is_none()
                            && let Some(doc) = super::formatter::try_macro_verbatim(
                                ctx,
                                macro_regions,
                                arena,
                                consumed_regions,
                                macro_tokenizer,
                                child_id,
                            )
                        {
                            running = arena.cat(running, doc);
                            return_action = ReturnAction::Discard;
                        }

                        if let Some((ctag, child_fields)) = ctx.reader.extract_fields(child_id)
                            && let Some((child_ops_bytes, child_ops_len)) =
                                ctx.dialect.fmt_dispatch(ctag)
                        {
                            if return_action != ReturnAction::Discard && parent_prec > 0 {
                                return_action = child_prec_action(
                                    ctx,
                                    parent_prec,
                                    parent_group_and_flags,
                                    is_right,
                                    ctag,
                                    &child_fields,
                                );
                            }
                            push_call_frame!(
                                child_id,
                                child_ops_bytes,
                                child_ops_len,
                                child_fields,
                                return_action
                            );
                        }
                    }
                }
                FmtOp::ChildParenList(child_idx) => {
                    let FieldValue::NodeId(child_id) = fields[child_idx as usize] else {
                        panic!("ChildParenList: field {child_idx} is not a NodeId");
                    };

                    if !child_id.is_null() {
                        drain_comments_before_child(
                            ctx.comment_ctx.as_ref(),
                            source,
                            &mut pending,
                            &mut running,
                            arena,
                        );

                        let mut return_action = ReturnAction::CatOntoRunning;
                        let macro_regions = &ctx.macro_regions;
                        if !macro_regions.is_empty()
                            && ctx.reader.list_children(child_id).is_none()
                            && let Some(doc) = super::formatter::try_macro_verbatim(
                                ctx,
                                macro_regions,
                                arena,
                                consumed_regions,
                                macro_tokenizer,
                                child_id,
                            )
                        {
                            running = arena.cat(running, doc);
                            return_action = ReturnAction::Discard;
                        }

                        if let Some((ctag, child_fields)) = ctx.reader.extract_fields(child_id)
                            && let Some((child_ops_bytes, child_ops_len)) =
                                ctx.dialect.fmt_dispatch(ctag)
                        {
                            if return_action != ReturnAction::Discard && ctx.dialect.is_list(ctag) {
                                return_action = ReturnAction::WrapInParens;
                            }
                            push_call_frame!(
                                child_id,
                                child_ops_bytes,
                                child_ops_len,
                                child_fields,
                                return_action
                            );
                        }
                    }
                }
                FmtOp::ChildPrecFixed(child_idx, packed_b, is_right_flag) => {
                    let parent_prec = (packed_b >> 8) as u8;
                    let parent_group_and_flags = (packed_b & 0xFF) as u8;
                    let is_right = is_right_flag != 0;

                    let FieldValue::NodeId(child_id) = fields[child_idx as usize] else {
                        panic!("ChildPrecFixed: field {child_idx} is not a NodeId");
                    };

                    if !child_id.is_null() {
                        drain_comments_before_child(
                            ctx.comment_ctx.as_ref(),
                            source,
                            &mut pending,
                            &mut running,
                            arena,
                        );

                        let mut return_action = ReturnAction::CatOntoRunning;
                        let macro_regions = &ctx.macro_regions;
                        if !macro_regions.is_empty()
                            && ctx.reader.list_children(child_id).is_none()
                            && let Some(doc) = super::formatter::try_macro_verbatim(
                                ctx,
                                macro_regions,
                                arena,
                                consumed_regions,
                                macro_tokenizer,
                                child_id,
                            )
                        {
                            running = arena.cat(running, doc);
                            return_action = ReturnAction::Discard;
                        }

                        if let Some((ctag, child_fields)) = ctx.reader.extract_fields(child_id)
                            && let Some((child_ops_bytes, child_ops_len)) =
                                ctx.dialect.fmt_dispatch(ctag)
                        {
                            if return_action != ReturnAction::Discard && parent_prec > 0 {
                                return_action = child_prec_action(
                                    ctx,
                                    parent_prec,
                                    parent_group_and_flags,
                                    is_right,
                                    ctag,
                                    &child_fields,
                                );
                            }
                            push_call_frame!(
                                child_id,
                                child_ops_bytes,
                                child_ops_len,
                                child_fields,
                                return_action
                            );
                        }
                    }
                }
            }
            ip += 1;
        }
    }
}

/// Decode a packed `group_and_flags` byte: bits 0-6 = group, bit 7 = `paren_boundary`.
/// Group value 0x7F means "no group".
#[inline]
fn decode_group(group_and_flags: u8) -> (u8, bool) {
    let group = group_and_flags & 0x7F;
    let is_boundary = group_and_flags & 0x80 != 0;
    (group, is_boundary)
}

/// Determine the return action for a `CHILD_PREC` / `CHILD_PREC_FIXED` opcode.
///
/// Compares the parent's operator precedence/group with the child's (if the
/// child tag has expr-meta). Also wraps list children (ExprList-as-expression).
///
/// The `parent_group_and_flags` byte packs group (bits 0-6) and `paren_boundary`
/// (bit 7). Group 0x7F means "no group".
fn child_prec_action(
    ctx: &FmtCtx,
    parent_prec: u8,
    parent_group_and_flags: u8,
    is_right: bool,
    child_tag: syntaqlite_syntax::any::AnyNodeTag,
    child_fields: &syntaqlite_syntax::any::NodeFields,
) -> ReturnAction {
    // Check if child carries operator precedence info.
    if let Some((child_op_field_idx, child_prec_base)) = ctx.dialect.fmt_expr_meta(child_tag) {
        // Resolve child's prec and group.
        let (child_prec, child_group_and_flags) = if child_op_field_idx == 0xFF {
            // Fixed-prec node: ordinal is always 0.
            ctx.dialect.fmt_prec_lookup(child_prec_base, 0)
        } else {
            let FieldValue::Enum(child_op_ordinal) = child_fields[child_op_field_idx as usize]
            else {
                return ReturnAction::CatOntoRunning;
            };
            ctx.dialect
                .fmt_prec_lookup(child_prec_base, child_op_ordinal)
        };
        if child_prec == 0 {
            // Child variant has no prec annotation — treat as plain child.
            return ReturnAction::CatOntoRunning;
        }

        // Correctness: parens when child binds less tightly,
        // or same precedence on the right (left-associative).
        if child_prec < parent_prec || (is_right && child_prec == parent_prec) {
            return ReturnAction::WrapInParens;
        }

        let (parent_group, _) = decode_group(parent_group_and_flags);
        let (child_group, child_is_boundary) = decode_group(child_group_and_flags);

        // Readability: parens when crossing operator groups.
        // 0x7F means "no group" — skip group comparison in that case.
        if parent_group != 0x7F && child_group != 0x7F && parent_group != child_group {
            return ReturnAction::WrapInParens;
        }

        // Readability: paren_boundary — when the child has the boundary flag
        // and is nested inside a different-precedence operator in the same group.
        if child_is_boundary
            && parent_group != 0x7F
            && parent_group == child_group
            && child_prec != parent_prec
        {
            return ReturnAction::WrapInParens;
        }

        return ReturnAction::CatOntoRunning;
    }

    // If child is a list node, wrap it (ExprList-as-expression).
    if ctx.dialect.is_list(child_tag) {
        return ReturnAction::WrapInParens;
    }

    ReturnAction::CatOntoRunning
}

// ── Comment drain helpers ───────────────────────────────────────────────

#[inline]
fn flush_drain(
    drain: &DrainResult,
    pending: &mut DocId,
    running: &mut DocId,
    arena: &mut DocArena,
) {
    if drain.trailing != NIL_DOC {
        *running = arena.cat(*running, drain.trailing);
    }
    if drain.leading == NIL_DOC {
        *running = arena.cat(*running, *pending);
        *pending = NIL_DOC;
    } else {
        *pending = NIL_DOC;
        *running = arena.cat(*running, drain.leading);
    }
}

#[inline]
fn drain_comments_before_child<'a>(
    comment_ctx: Option<&CommentCtx>,
    source: &'a str,
    pending: &mut DocId,
    running: &mut DocId,
    arena: &mut DocArena<'a>,
) {
    if let Some(cctx) = comment_ctx {
        if let Some((offset, _)) = cctx.peek_next_token() {
            let drain = cctx.drain_before(offset, source, arena);
            flush_drain(&drain, pending, running, arena);
        } else {
            *running = arena.cat(*running, *pending);
            *pending = NIL_DOC;
        }
    }
}

// ── Bytecode helpers ────────────────────────────────────────────────────

#[inline]
fn op_at(ops: &[u8], ip: usize) -> FmtOp {
    let base = ip * 6;
    let opcode = ops[base];
    let a = ops[base + 1];
    let b = u16::from_le_bytes([ops[base + 2], ops[base + 3]]);
    let c = u16::from_le_bytes([ops[base + 4], ops[base + 5]]);
    FmtOp::decode(opcode, a, b, c)
}

fn skip_to_foreach_end(ops: &[u8], ops_count: usize, from_ip: usize) -> usize {
    let mut depth = 1u32;
    let mut ip = from_ip + 1;
    while ip < ops_count {
        match op_at(ops, ip) {
            FmtOp::ForEachStart(_) | FmtOp::ForEachSelfStart => depth += 1,
            FmtOp::ForEachEnd => {
                depth -= 1;
                if depth == 0 {
                    return ip;
                }
            }
            _ => {}
        }
        ip += 1;
    }
    panic!("unmatched ForEachStart");
}

// ── Private types ───────────────────────────────────────────────────────

type StringId = u16;
type FieldIdx = u16;
type SkipCount = u16;

#[derive(Debug, Clone, Copy, PartialEq)]
enum FmtOp {
    Keyword(StringId),
    Span(FieldIdx),
    Child(FieldIdx),
    Line,
    SoftLine,
    HardLine,
    GroupStart,
    GroupEnd,
    NestStart(i16),
    NestEnd,
    IfSet(FieldIdx, SkipCount),
    Else(SkipCount),
    EndIf,
    ForEachStart(FieldIdx),
    ChildItem,
    ForEachSep(StringId),
    ForEachEnd,
    IfBool(FieldIdx, SkipCount),
    IfFlag(FieldIdx, u8, SkipCount),
    IfEnum(FieldIdx, u16, SkipCount),
    IfSpan(FieldIdx, SkipCount),
    EnumDisplay(FieldIdx, u16),
    ForEachSelfStart,
    /// Precedence-aware child: `a`=`child_field`, `b`=`prec_table_base`, `c`=packed(`op_field`<<`8|is_right`).
    ChildPrec(FieldIdx, u16, u16),
    /// Paren-list child: `a`=`child_field`. Wraps if child is a list node.
    ChildParenList(FieldIdx),
    /// Fixed-precedence child: `a`=`child_field`, `b`=packed(`prec<<8|group_and_flags`), `c`=`is_right`.
    ChildPrecFixed(FieldIdx, u16, u16),
}

impl FmtOp {
    #[inline]
    pub(crate) fn decode(opcode: u8, a: u8, b: u16, c: u16) -> Self {
        match opcode {
            opcodes::KEYWORD => FmtOp::Keyword(b),
            opcodes::SPAN => FmtOp::Span(a.into()),
            opcodes::CHILD => FmtOp::Child(a.into()),
            opcodes::LINE => FmtOp::Line,
            opcodes::SOFTLINE => FmtOp::SoftLine,
            opcodes::HARDLINE => FmtOp::HardLine,
            opcodes::GROUP_START => FmtOp::GroupStart,
            opcodes::GROUP_END => FmtOp::GroupEnd,
            opcodes::NEST_START => FmtOp::NestStart(i16::from_le_bytes(b.to_le_bytes())),
            opcodes::NEST_END => FmtOp::NestEnd,
            opcodes::IF_SET => FmtOp::IfSet(a.into(), c),
            opcodes::ELSE_OP => FmtOp::Else(c),
            opcodes::END_IF => FmtOp::EndIf,
            opcodes::FOR_EACH_START => FmtOp::ForEachStart(a.into()),
            opcodes::CHILD_ITEM => FmtOp::ChildItem,
            opcodes::FOR_EACH_SEP => FmtOp::ForEachSep(b),
            opcodes::FOR_EACH_END => FmtOp::ForEachEnd,
            opcodes::IF_BOOL => FmtOp::IfBool(a.into(), c),
            opcodes::IF_FLAG => FmtOp::IfFlag(
                a.into(),
                u8::try_from(b).expect("IF_FLAG mask must fit in u8"),
                c,
            ),
            opcodes::IF_ENUM => FmtOp::IfEnum(a.into(), b, c),
            opcodes::IF_SPAN => FmtOp::IfSpan(a.into(), c),
            opcodes::ENUM_DISPLAY => FmtOp::EnumDisplay(a.into(), b),
            opcodes::FOR_EACH_SELF_START => FmtOp::ForEachSelfStart,
            opcodes::CHILD_PREC => FmtOp::ChildPrec(a.into(), b, c),
            opcodes::CHILD_PAREN_LIST => FmtOp::ChildParenList(a.into()),
            opcodes::CHILD_PREC_FIXED => FmtOp::ChildPrecFixed(a.into(), b, c),
            _ => panic!("unknown opcode in fmt data"),
        }
    }
}

#[inline]
fn usize_to_u32(value: usize) -> u32 {
    u32::try_from(value).expect("value must fit in u32")
}

#[inline]
fn byte_offset_in(source: &str, ptr: *const u8) -> u32 {
    let base = source.as_ptr() as usize;
    let start = ptr as usize;
    let offset = start
        .checked_sub(base)
        .expect("span pointer must be within source");
    usize_to_u32(offset)
}

enum GroupNestFrame {
    Group(DocId),
    Nest(i16, DocId),
}

struct ForEachState {
    list_id: AnyNodeId,
    index: usize,
    body_start: usize,
    sep_checkpoint: Option<(DocId, DocId)>,
}
