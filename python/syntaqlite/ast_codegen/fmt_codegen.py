# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Generate src/fmt/fmt.c from FmtDoc annotations on node definitions.

Compiles the FmtDoc DSL tree on each NodeDef/ListDef into C formatting
functions, then emits the format_node dispatcher and public API.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from .defs import (
    AnyNodeDef,
    NodeDef,
    ListDef,
    EnumDef,
    FlagsDef,
    InlineField,
    pascal_to_snake,
)

from .fmt_dsl import (
    FmtDoc,
    FmtKw,
    FmtSpan,
    FmtChild,
    FmtLine,
    FmtSoftline,
    FmtHardline,
    FmtSeq,
    FmtGroup,
    FmtNest,
    FmtIfSet,
    FmtIfFlag,
    FmtIfEnum,
    FmtIfSpan,
    FmtClause,
    FmtEnumDisplay,
    FmtSwitch,
    FmtForEachChild,
)


def _tag_name(node_name: str) -> str:
    return f"SYNTAQLITE_NODE_{pascal_to_snake(node_name).upper()}"


def _struct_name(node_name: str) -> str:
    return f"Syntaqlite{node_name}"


def _enum_prefix(enum_name: str) -> str:
    return f"SYNTAQLITE_{pascal_to_snake(enum_name).upper()}"


def _c_str(s: str) -> str:
    """Escape a string for C."""
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


@dataclass
class _Result:
    """Result of compiling a FmtDoc node: C variable name + nullability."""
    var: str
    nullable: bool


class _Compiler:
    """Compiles a FmtDoc tree into C statements that build a doc tree."""

    def __init__(self, node_def: AnyNodeDef):
        self.node_def = node_def
        self.var_counter = 0
        self.lines: list[str] = []
        self.needs_concat_nullable = False

    def _var(self, prefix: str = "d") -> str:
        self.var_counter += 1
        return f"{prefix}_{self.var_counter}"

    def _accessor(self, field: str) -> str:
        """Get C accessor for a field."""
        return f"node->{field}"

    def _compile_to_buffer(self, doc: FmtDoc, indent: str) -> tuple[_Result, list[str]]:
        """Compile doc into a temporary line buffer, returning (result, lines)."""
        saved = self.lines
        self.lines = []
        result = self.compile(doc, indent)
        buf = self.lines
        self.lines = saved
        return result, buf

    def _extract_single_expr(self, result: _Result, lines: list[str]) -> str | None:
        """If lines is a single 'uint32_t var = expr;', return the expr string."""
        if len(lines) != 1:
            return None
        stripped = lines[0].lstrip()
        prefix = f"uint32_t {result.var} = "
        if stripped.startswith(prefix) and stripped.endswith(";"):
            return stripped[len(prefix):-1]
        return None

    def compile(self, doc: FmtDoc, indent: str = "    ") -> _Result:
        """Compile a FmtDoc and return a _Result with C variable name and nullability."""
        ind = indent

        if isinstance(doc, FmtKw):
            v = self._var("kw")
            self.lines.append(f"{ind}uint32_t {v} = kw(ctx, {_c_str(doc.text)});")
            return _Result(v, False)

        if isinstance(doc, FmtSpan):
            v = self._var("sp")
            self.lines.append(f"{ind}uint32_t {v} = span_text(ctx, {self._accessor(doc.field)});")
            return _Result(v, True)

        if isinstance(doc, FmtChild):
            if doc.field == "_item":
                v = self._var("item")
                self.lines.append(f"{ind}uint32_t {v} = format_node(ctx, _child_id);")
                return _Result(v, True)
            v = self._var("ch")
            self.lines.append(f"{ind}uint32_t {v} = format_node(ctx, {self._accessor(doc.field)});")
            return _Result(v, True)

        if isinstance(doc, FmtLine):
            v = self._var("ln")
            self.lines.append(f"{ind}uint32_t {v} = doc_line(&ctx->docs);")
            return _Result(v, False)

        if isinstance(doc, FmtSoftline):
            v = self._var("sl")
            self.lines.append(f"{ind}uint32_t {v} = doc_softline(&ctx->docs);")
            return _Result(v, False)

        if isinstance(doc, FmtHardline):
            v = self._var("hl")
            self.lines.append(f"{ind}uint32_t {v} = doc_hardline(&ctx->docs);")
            return _Result(v, False)

        if isinstance(doc, FmtSeq):
            return self._compile_seq(doc, ind)

        if isinstance(doc, FmtGroup):
            inner = self.compile(doc.child, ind)
            v = self._var("grp")
            self.lines.append(f"{ind}uint32_t {v} = doc_group(&ctx->docs, {inner.var});")
            return _Result(v, False)

        if isinstance(doc, FmtNest):
            inner = self.compile(doc.child, ind)
            v = self._var("nst")
            self.lines.append(
                f"{ind}uint32_t {v} = doc_nest(&ctx->docs,"
                f" (int32_t)ctx->options->indent_width, {inner.var});")
            return _Result(v, False)

        if isinstance(doc, FmtIfSet):
            # Optimization: if_set(field, clause(kw, child(field))) → format_clause(ctx, kw, format_node(...))
            opt = self._try_optimize_if_set(doc, ind)
            if opt is not None:
                return opt
            return self._compile_if(ind,
                f"{self._accessor(doc.field)} != SYNTAQLITE_NULL_NODE",
                doc.then, doc.else_)

        if isinstance(doc, FmtIfFlag):
            return self._compile_if(ind,
                f"{self._accessor(doc.field)}",
                doc.then, doc.else_)

        if isinstance(doc, FmtIfEnum):
            enum_value = self._resolve_enum_value(doc.field, doc.value)
            return self._compile_if(ind,
                f"{self._accessor(doc.field)} == {enum_value}",
                doc.then, doc.else_)

        if isinstance(doc, FmtIfSpan):
            return self._compile_if(ind,
                f"{self._accessor(doc.field)}.length > 0",
                doc.then, doc.else_)

        if isinstance(doc, FmtClause):
            # format_clause takes a node_id directly (not a pre-formatted doc)
            # to ensure kw() runs before format_node() for correct cursor order.
            if isinstance(doc.body, FmtChild) and doc.body.field != "_item":
                v = self._var("cl")
                self.lines.append(
                    f"{ind}uint32_t {v} = format_clause(ctx,"
                    f" {_c_str(doc.keyword)}, {self._accessor(doc.body.field)});")
                return _Result(v, True)
            # Fallback: compile body normally (for non-child bodies)
            body = self.compile(doc.body, ind)
            v = self._var("cl")
            self.lines.append(
                f"{ind}uint32_t {v} = format_clause(ctx,"
                f" {_c_str(doc.keyword)}, {body.var});")
            return _Result(v, True)

        if isinstance(doc, FmtEnumDisplay):
            return self._compile_enum_display(doc, ind)

        if isinstance(doc, FmtSwitch):
            return self._compile_switch(doc, ind)

        if isinstance(doc, FmtForEachChild):
            return self._compile_for_each_child(doc, ind)

        raise TypeError(f"Unknown FmtDoc type: {type(doc)}")

    def _compile_seq(self, doc: FmtSeq, ind: str) -> _Result:
        """Compile FmtSeq with nullability-aware optimization."""
        n = len(doc.items)
        item_results = [self.compile(item, ind) for item in doc.items]
        any_nullable = any(r.nullable for r in item_results)

        if not any_nullable:
            # All children non-nullable: direct array initializer
            v = self._var("cat")
            items_str = ", ".join(r.var for r in item_results)
            self.lines.append(f"{ind}uint32_t {v}_items[] = {{ {items_str} }};")
            self.lines.append(f"{ind}uint32_t {v} = doc_concat(&ctx->docs, {v}_items, {n});")
            return _Result(v, False)
        else:
            # Some children nullable: use doc_concat_nullable helper
            self.needs_concat_nullable = True
            v = self._var("cat")
            items_str = ", ".join(r.var for r in item_results)
            self.lines.append(f"{ind}uint32_t {v}_items[] = {{ {items_str} }};")
            self.lines.append(f"{ind}uint32_t {v} = doc_concat_nullable(&ctx->docs, {v}_items, {n});")
            return _Result(v, True)

    def _try_optimize_if_set(self, doc: FmtIfSet, ind: str) -> _Result | None:
        """Try to optimize if_set patterns into direct calls."""
        if doc.else_ is not None:
            return None

        # if_set(field, clause(kw, child(field))) → format_clause(ctx, kw, node->field)
        if (isinstance(doc.then, FmtClause)
                and isinstance(doc.then.body, FmtChild)
                and doc.then.body.field == doc.field):
            v = self._var("cl")
            self.lines.append(
                f"{ind}uint32_t {v} = format_clause(ctx,"
                f" {_c_str(doc.then.keyword)},"
                f" {self._accessor(doc.field)});")
            return _Result(v, True)

        # if_set(field, child(field)) → format_node(ctx, node->field)
        if (isinstance(doc.then, FmtChild)
                and doc.then.field == doc.field):
            v = self._var("ch")
            self.lines.append(
                f"{ind}uint32_t {v} = format_node(ctx, {self._accessor(doc.field)});")
            return _Result(v, True)

        return None

    def _compile_if(self, ind: str, condition: str,
                    then_doc: FmtDoc, else_doc: FmtDoc | None) -> _Result:
        inner_ind = ind + "    "
        v = self._var("cond")

        # Compile branches into temporary buffers for inspection.
        then_result, then_lines = self._compile_to_buffer(then_doc, inner_ind)
        then_expr = self._extract_single_expr(then_result, then_lines)

        if else_doc:
            else_result, else_lines = self._compile_to_buffer(else_doc, inner_ind)
            else_expr = self._extract_single_expr(else_result, else_lines)
        else:
            else_result = None
            else_lines = []
            else_expr = None

        # Both branches are single expressions → ternary.
        if then_expr is not None and else_expr is not None:
            nullable = then_result.nullable or else_result.nullable
            self.lines.append(
                f"{ind}uint32_t {v} = {condition}"
                f" ? {then_expr} : {else_expr};")
            return _Result(v, nullable)

        # Single-expression then, no else → one-line if.
        if then_expr is not None and else_doc is None:
            self.lines.append(f"{ind}uint32_t {v} = SYNTAQLITE_NULL_DOC;")
            self.lines.append(f"{ind}if ({condition}) {v} = {then_expr};")
            return _Result(v, True)

        # General case: emit block structure.
        self.lines.append(f"{ind}uint32_t {v} = SYNTAQLITE_NULL_DOC;")
        self.lines.append(f"{ind}if ({condition}) {{")
        if then_expr is not None:
            self.lines.append(f"{inner_ind}{v} = {then_expr};")
        else:
            self.lines.extend(then_lines)
            self.lines.append(f"{inner_ind}{v} = {then_result.var};")
        nullable = True
        if else_doc:
            self.lines.append(f"{ind}}} else {{")
            if else_expr is not None:
                self.lines.append(f"{inner_ind}{v} = {else_expr};")
            else:
                self.lines.extend(else_lines)
                self.lines.append(f"{inner_ind}{v} = {else_result.var};")
            nullable = then_result.nullable or else_result.nullable
        self.lines.append(f"{ind}}}")
        return _Result(v, nullable)

    def _compile_enum_display(self, doc: FmtEnumDisplay, ind: str) -> _Result:
        """Switch mapping enum values to kw() display strings."""
        v = self._var("ed")
        self.lines.append(f"{ind}uint32_t {v} = SYNTAQLITE_NULL_DOC;")
        self.lines.append(f"{ind}switch ({self._accessor(doc.field)}) {{")
        for enum_val, display_str in doc.mapping.items():
            c_enum = self._resolve_enum_value(doc.field, enum_val)
            self.lines.append(
                f"{ind}    case {c_enum}:"
                f" {v} = kw(ctx, {_c_str(display_str)}); break;")
        self.lines.append(f"{ind}    default: break;")
        self.lines.append(f"{ind}}}")
        return _Result(v, True)

    def _compile_switch(self, doc: FmtSwitch, ind: str) -> _Result:
        """Multi-branch on enum field with arbitrary FmtDoc bodies."""
        case_ind = ind + "    "
        body_ind = ind + "        "
        v = self._var("sw")
        self.lines.append(f"{ind}uint32_t {v} = SYNTAQLITE_NULL_DOC;")
        self.lines.append(f"{ind}switch ({self._accessor(doc.field)}) {{")
        for case_val, case_doc in doc.cases.items():
            c_enum = self._resolve_enum_value(doc.field, case_val)
            body_result, body_lines = self._compile_to_buffer(case_doc, body_ind)
            body_expr = self._extract_single_expr(body_result, body_lines)
            if body_expr is not None:
                # Flat case: single expression.
                self.lines.append(
                    f"{case_ind}case {c_enum}:"
                    f" {v} = {body_expr}; break;")
            else:
                self.lines.append(f"{case_ind}case {c_enum}: {{")
                self.lines.extend(body_lines)
                self.lines.append(f"{body_ind}{v} = {body_result.var};")
                self.lines.append(f"{body_ind}break;")
                self.lines.append(f"{case_ind}}}")
        if doc.default:
            def_result, def_lines = self._compile_to_buffer(doc.default, body_ind)
            def_expr = self._extract_single_expr(def_result, def_lines)
            if def_expr is not None:
                self.lines.append(
                    f"{case_ind}default:"
                    f" {v} = {def_expr}; break;")
            else:
                self.lines.append(f"{case_ind}default: {{")
                self.lines.extend(def_lines)
                self.lines.append(f"{body_ind}{v} = {def_result.var};")
                self.lines.append(f"{body_ind}break;")
                self.lines.append(f"{case_ind}}}")
        else:
            self.lines.append(f"{case_ind}default: break;")
        self.lines.append(f"{ind}}}")
        return _Result(v, True)

    def _compile_for_each_child(self, doc: FmtForEachChild, ind: str) -> _Result:
        """Iterate list children with per-item formatting."""
        loop_ind = ind + "    "
        max_per = 2 if doc.separator else 1
        buf = self._var("_buf")
        cnt = self._var("_n")
        v = self._var("lst")
        cap_expr = f"node->count * {max_per}" if max_per > 1 else "node->count"
        self.lines.append(f"{ind}uint32_t {buf}[{cap_expr} > 0 ? {cap_expr} : 1];")
        self.lines.append(f"{ind}uint32_t {cnt} = 0;")
        self.lines.append(
            f"{ind}for (uint32_t _i = 0; _i < node->count; _i++) {{")
        self.lines.append(f"{loop_ind}uint32_t _child_id = node->children[_i];")
        if doc.separator:
            sep_ind = loop_ind + "    "
            self.lines.append(f"{loop_ind}if (_i > 0) {{")
            sep_result = self.compile(doc.separator, sep_ind)
            self.lines.append(f"{sep_ind}{buf}[{cnt}++] = {sep_result.var};")
            self.lines.append(f"{loop_ind}}}")
        template_result = self.compile(doc.template, loop_ind)
        self.lines.append(f"{loop_ind}{buf}[{cnt}++] = {template_result.var};")
        self.lines.append(f"{ind}}}")
        self.lines.append(f"{ind}uint32_t {v} = doc_concat(&ctx->docs, {buf}, {cnt});")
        return _Result(v, False)

    def _resolve_enum_value(self, field_name: str, value: str) -> str:
        """Resolve an enum value to its C constant name."""
        type_name = self._get_field_type_name(field_name)
        prefix = _enum_prefix(type_name)
        return f"{prefix}_{value}"

    def _get_field_type_name(self, field_name: str) -> str:
        """Get the type name for a field."""
        if isinstance(self.node_def, NodeDef) and field_name in self.node_def.fields:
            field = self.node_def.fields[field_name]
            if isinstance(field, InlineField):
                return field.type_name
        # Fallback: derive from field name
        return field_name


def generate_fmt_c(
    node_defs: list[AnyNodeDef],
    enum_defs: list[EnumDef],
    flags_defs: list[FlagsDef],
    output: Path,
) -> None:
    """Generate src/fmt/fmt.c from fmt annotations on node definitions."""
    lines: list[str] = []

    # Header
    lines.append("// Copyright 2025 The syntaqlite Authors. All rights reserved.")
    lines.append("// Licensed under the Apache License, Version 2.0.")
    lines.append("")
    lines.append("// Generated from ast_codegen node definitions - DO NOT EDIT")
    lines.append("// Regenerate with: python3 python/tools/extract_sqlite.py")
    lines.append("")
    lines.append('#include "src/fmt/fmt.h"')
    lines.append("")
    lines.append('#include "src/ast/ast_nodes.h"')
    lines.append('#include "src/fmt/comment_map.h"')
    lines.append('#include "src/fmt/doc.h"')
    lines.append('#include "src/fmt/doc_layout.h"')
    lines.append('#include "src/sqlite_tokens.h"')
    lines.append("")
    lines.append("#include <stdlib.h>")
    lines.append("#include <string.h>")
    lines.append("")

    # FmtCtx struct
    lines.append("// ============ Formatter Context ============")
    lines.append("")
    lines.append("typedef struct {")
    lines.append("    SyntaqliteDocContext docs;")
    lines.append("    SyntaqliteArena *ast;")
    lines.append("    const char *source;")
    lines.append("    SyntaqliteTokenList *token_list;")
    lines.append("    SyntaqliteFmtOptions *options;")
    lines.append("    SyntaqliteCommentMap *comment_map;")
    lines.append("    uint32_t token_cursor;")
    lines.append("} FmtCtx;")
    lines.append("")

    # Helper functions
    lines.append("// ============ Helpers ============")
    lines.append("")
    lines.append("// Emit a single comment token as a doc using pre-classified kind.")
    lines.append("static uint32_t emit_comment_doc(FmtCtx *ctx, uint32_t tok_idx) {")
    lines.append("    SyntaqliteRawToken *tok = &ctx->token_list->data[tok_idx];")
    lines.append("    int is_line = (ctx->source[tok->offset] == '-' &&")
    lines.append("                   tok->length >= 2 && ctx->source[tok->offset + 1] == '-');")
    lines.append("    if (ctx->comment_map->kinds[tok_idx] == SYNTAQLITE_COMMENT_TRAILING) {")
    lines.append("        // Trailing comments use line_suffix so they don't force")
    lines.append("        // enclosing groups to break. The suffix is flushed before")
    lines.append("        // the next line break in the layout engine.")
    lines.append('        uint32_t parts[2];')
    lines.append('        parts[0] = doc_text(&ctx->docs, " ", 1);')
    lines.append("        parts[1] = doc_text(&ctx->docs, ctx->source + tok->offset, tok->length);")
    lines.append("        uint32_t inner = doc_concat(&ctx->docs, parts, 2);")
    lines.append("        if (is_line) return doc_line_suffix(&ctx->docs, inner);")
    lines.append("        return inner;")
    lines.append("    } else {")
    lines.append("        uint32_t items[2];")
    lines.append("        uint32_t k = 0;")
    lines.append("        items[k++] = doc_text(&ctx->docs, ctx->source + tok->offset, tok->length);")
    lines.append("        if (is_line)")
    lines.append("            items[k++] = doc_hardline(&ctx->docs);")
    lines.append("        else")
    lines.append('            items[k++] = doc_text(&ctx->docs, " ", 1);')
    lines.append("        return doc_concat(&ctx->docs, items, k);")
    lines.append("    }")
    lines.append("}")
    lines.append("")
    lines.append("// Advance cursor past comments and one real token.")
    lines.append("// Returns leading comments (before real token). Sets *out_trailing to")
    lines.append("// trailing comments (after real token). Either may be SYNTAQLITE_NULL_DOC.")
    lines.append("static uint32_t advance_with_comments(FmtCtx *ctx, uint32_t *out_trailing) {")
    lines.append("    *out_trailing = SYNTAQLITE_NULL_DOC;")
    lines.append("    if (!ctx->token_list) return SYNTAQLITE_NULL_DOC;")
    lines.append("    uint32_t lead[24], trail[24];")
    lines.append("    uint32_t nl = 0, nt = 0;")
    lines.append("    while (ctx->token_cursor < ctx->token_list->count) {")
    lines.append("        SyntaqliteRawToken *tok = &ctx->token_list->data[ctx->token_cursor];")
    lines.append("        if (tok->type == TK_SPACE) { ctx->token_cursor++; continue; }")
    lines.append("        if (tok->type == TK_COMMENT) {")
    lines.append("            if (ctx->comment_map && nl < 24)")
    lines.append("                lead[nl++] = emit_comment_doc(ctx, ctx->token_cursor);")
    lines.append("            ctx->token_cursor++;")
    lines.append("            continue;")
    lines.append("        }")
    lines.append("        // Real token - advance past it")
    lines.append("        ctx->token_cursor++;")
    lines.append("        // Collect trailing comments after this real token")
    lines.append("        while (ctx->token_cursor < ctx->token_list->count) {")
    lines.append("            SyntaqliteRawToken *next = &ctx->token_list->data[ctx->token_cursor];")
    lines.append("            if (next->type == TK_SPACE) { ctx->token_cursor++; continue; }")
    lines.append("            if (next->type == TK_COMMENT && ctx->comment_map &&")
    lines.append("                ctx->comment_map->kinds[ctx->token_cursor] == SYNTAQLITE_COMMENT_TRAILING) {")
    lines.append("                if (nt < 24) trail[nt++] = emit_comment_doc(ctx, ctx->token_cursor);")
    lines.append("                ctx->token_cursor++;")
    lines.append("                continue;")
    lines.append("            }")
    lines.append("            break;")
    lines.append("        }")
    lines.append("        break;")
    lines.append("    }")
    lines.append("    if (nt > 0) *out_trailing = (nt == 1) ? trail[0] : doc_concat(&ctx->docs, trail, nt);")
    lines.append("    if (nl == 0) return SYNTAQLITE_NULL_DOC;")
    lines.append("    return (nl == 1) ? lead[0] : doc_concat(&ctx->docs, lead, nl);")
    lines.append("}")
    lines.append("")
    lines.append("// Count how many source tokens a keyword string consumes.")
    lines.append("// Each word (letters/digits) and each punctuation char is one token.")
    lines.append("static int count_kw_tokens(const char *text) {")
    lines.append("    int n = 0;")
    lines.append("    const char *p = text;")
    lines.append("    while (*p) {")
    lines.append("        if (*p == ' ') { p++; continue; }")
    lines.append("        if ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') || *p == '_') {")
    lines.append("            n++;")
    lines.append("            while (*p && ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') ||")
    lines.append("                          *p == '_' || (*p >= '0' && *p <= '9'))) p++;")
    lines.append("        } else {")
    lines.append("            n++; p++;")
    lines.append("        }")
    lines.append("    }")
    lines.append("    return n;")
    lines.append("}")
    lines.append("")
    lines.append("static uint32_t kw(FmtCtx *ctx, const char *text) {")
    lines.append("    int n = count_kw_tokens(text);")
    lines.append("    uint32_t pre = SYNTAQLITE_NULL_DOC;")
    lines.append("    uint32_t post = SYNTAQLITE_NULL_DOC;")
    lines.append("    for (int i = 0; i < n; i++) {")
    lines.append("        uint32_t trailing;")
    lines.append("        uint32_t c = advance_with_comments(ctx, &trailing);")
    lines.append("        if (c != SYNTAQLITE_NULL_DOC) {")
    lines.append("            if (pre == SYNTAQLITE_NULL_DOC) { pre = c; }")
    lines.append("            else {")
    lines.append("                uint32_t pair[] = { pre, c };")
    lines.append("                pre = doc_concat(&ctx->docs, pair, 2);")
    lines.append("            }")
    lines.append("        }")
    lines.append("        if (trailing != SYNTAQLITE_NULL_DOC) {")
    lines.append("            if (post == SYNTAQLITE_NULL_DOC) { post = trailing; }")
    lines.append("            else {")
    lines.append("                uint32_t pair[] = { post, trailing };")
    lines.append("                post = doc_concat(&ctx->docs, pair, 2);")
    lines.append("            }")
    lines.append("        }")
    lines.append("    }")
    lines.append("    uint32_t txt = doc_text(&ctx->docs, text, (uint32_t)strlen(text));")
    lines.append("    uint32_t parts[3];")
    lines.append("    uint32_t np = 0;")
    lines.append("    if (pre != SYNTAQLITE_NULL_DOC) parts[np++] = pre;")
    lines.append("    parts[np++] = txt;")
    lines.append("    if (post != SYNTAQLITE_NULL_DOC) parts[np++] = post;")
    lines.append("    if (np == 1) return parts[0];")
    lines.append("    return doc_concat(&ctx->docs, parts, np);")
    lines.append("}")
    lines.append("")
    lines.append("static uint32_t span_text(FmtCtx *ctx, SyntaqliteSourceSpan span) {")
    lines.append("    if (span.length == 0) return SYNTAQLITE_NULL_DOC;")
    lines.append("    uint32_t trailing;")
    lines.append("    uint32_t pre = advance_with_comments(ctx, &trailing);")
    lines.append("    uint32_t txt = doc_text(&ctx->docs, ctx->source + span.offset, span.length);")
    lines.append("    uint32_t parts[3];")
    lines.append("    uint32_t np = 0;")
    lines.append("    if (pre != SYNTAQLITE_NULL_DOC) parts[np++] = pre;")
    lines.append("    parts[np++] = txt;")
    lines.append("    if (trailing != SYNTAQLITE_NULL_DOC) parts[np++] = trailing;")
    lines.append("    if (np == 1) return parts[0];")
    lines.append("    return doc_concat(&ctx->docs, parts, np);")
    lines.append("}")
    lines.append("")

    # Forward declaration
    lines.append("static uint32_t format_node(FmtCtx *ctx, uint32_t node_id);")
    lines.append("")

    # Comma-separated list helper
    lines.append("// ============ Comma-Separated List ============")
    lines.append("")
    lines.append("static uint32_t format_comma_list(FmtCtx *ctx, uint32_t *children, uint32_t count) {")
    lines.append('    if (count == 0) return kw(ctx, "");')
    lines.append("    uint32_t buf[count * 3];")
    lines.append("    uint32_t n = 0;")
    lines.append("    for (uint32_t i = 0; i < count; i++) {")
    lines.append("        if (i > 0) {")
    lines.append('            buf[n++] = kw(ctx, ",");')
    lines.append("            buf[n++] = doc_line(&ctx->docs);")
    lines.append("        }")
    lines.append("        buf[n++] = format_node(ctx, children[i]);")
    lines.append("    }")
    lines.append("    return doc_group(&ctx->docs, doc_concat(&ctx->docs, buf, n));")
    lines.append("}")
    lines.append("")

    # Clause helper
    lines.append("// ============ Clause Helper ============")
    lines.append("")
    lines.append("static uint32_t format_clause(FmtCtx *ctx, const char *keyword, uint32_t body_id) {")
    lines.append("    if (body_id == SYNTAQLITE_NULL_NODE) return SYNTAQLITE_NULL_DOC;")
    lines.append("    uint32_t kw_doc = kw(ctx, keyword);")
    lines.append("    uint32_t body_doc = format_node(ctx, body_id);")
    lines.append("    if (body_doc == SYNTAQLITE_NULL_DOC) return SYNTAQLITE_NULL_DOC;")
    lines.append("    uint32_t inner_items[] = { doc_line(&ctx->docs), body_doc };")
    lines.append("    uint32_t inner = doc_concat(&ctx->docs, inner_items, 2);")
    lines.append("    uint32_t items[] = {")
    lines.append("        doc_line(&ctx->docs),")
    lines.append("        kw_doc,")
    lines.append("        doc_group(&ctx->docs, doc_nest(&ctx->docs, (int32_t)ctx->options->indent_width, inner)),")
    lines.append("    };")
    lines.append("    return doc_concat(&ctx->docs, items, 3);")
    lines.append("}")
    lines.append("")

    # Compile all node formatters first to check if we need doc_concat_nullable
    compiled_nodes: list[tuple[str, str, _Compiler, _Result]] = []
    needs_concat_nullable = False

    for node in node_defs:
        if node.fmt is None:
            continue

        snake = pascal_to_snake(node.name)
        struct = _struct_name(node.name)

        compiler = _Compiler(node)
        result = compiler.compile(node.fmt)
        compiled_nodes.append((snake, struct, compiler, result))
        if compiler.needs_concat_nullable:
            needs_concat_nullable = True

    # Emit concat_nullable helper if needed
    if needs_concat_nullable:
        lines.append("// ============ Nullable Concat Helper ============")
        lines.append("")
        lines.append("static uint32_t doc_concat_nullable(SyntaqliteDocContext *docs, uint32_t *items, uint32_t count) {")
        lines.append("    uint32_t buf[count > 0 ? count : 1];")
        lines.append("    uint32_t n = 0;")
        lines.append("    for (uint32_t i = 0; i < count; i++) {")
        lines.append("        if (items[i] != SYNTAQLITE_NULL_DOC) buf[n++] = items[i];")
        lines.append("    }")
        lines.append("    if (n == 0) return SYNTAQLITE_NULL_DOC;")
        lines.append("    return doc_concat(docs, buf, n);")
        lines.append("}")
        lines.append("")

    # Node formatters
    lines.append("// ============ Node Formatters ============")
    lines.append("")

    for snake, struct, compiler, result in compiled_nodes:
        func_lines = compiler.lines
        result_var = result.var

        # Optimization: inline single-use returns
        # If the last line is a variable assignment and it's the return var,
        # inline it into the return statement.
        if func_lines and result_var != "":
            last_line = func_lines[-1]
            prefix = f"    uint32_t {result_var} = "
            if last_line.startswith(prefix) and last_line.endswith(";"):
                expr = last_line[len(prefix):-1]
                func_lines = func_lines[:-1]
                lines.append(f"static uint32_t format_{snake}(FmtCtx *ctx, {struct} *node) {{")
                lines.extend(func_lines)
                lines.append(f"    return {expr};")
                lines.append("}")
                lines.append("")
                continue

        lines.append(f"static uint32_t format_{snake}(FmtCtx *ctx, {struct} *node) {{")
        lines.extend(func_lines)
        lines.append(f"    return {result_var};")
        lines.append("}")
        lines.append("")

    # Main dispatcher
    lines.append("// ============ Main Dispatcher ============")
    lines.append("")
    lines.append("static uint32_t format_node(FmtCtx *ctx, uint32_t node_id) {")
    lines.append("    if (node_id == SYNTAQLITE_NULL_NODE) return SYNTAQLITE_NULL_DOC;")
    lines.append("")
    lines.append("    SyntaqliteNode *node = AST_NODE(ctx->ast, node_id);")
    lines.append("    if (!node) return SYNTAQLITE_NULL_DOC;")
    lines.append("")
    lines.append("    switch (node->tag) {")

    for node in node_defs:
        tag = _tag_name(node.name)
        snake = pascal_to_snake(node.name)

        if node.fmt is not None:
            lines.append(f"        case {tag}:")
            lines.append(f"            return format_{snake}(ctx, &node->{snake});")
        elif isinstance(node, ListDef):
            # Default list handling: comma-separated
            lines.append(f"        case {tag}:")
            lines.append(
                f"            return format_comma_list(ctx,"
                f" node->{snake}.children, node->{snake}.count);")

    lines.append("        default:")
    lines.append('            return kw(ctx, "/* UNSUPPORTED */");')
    lines.append("    }")
    lines.append("}")
    lines.append("")

    # Public API
    lines.append("// ============ Public API ============")
    lines.append("")
    lines.append("char *syntaqlite_format(SyntaqliteArena *ast, uint32_t root_id,")
    lines.append("                        const char *source, SyntaqliteTokenList *token_list,")
    lines.append("                        SyntaqliteFmtOptions *options) {")
    lines.append("    SyntaqliteFmtOptions default_options = SYNTAQLITE_FMT_OPTIONS_DEFAULT;")
    lines.append("    if (!options) options = &default_options;")
    lines.append("")
    lines.append("    FmtCtx ctx;")
    lines.append("    syntaqlite_doc_context_init(&ctx.docs);")
    lines.append("    ctx.ast = ast;")
    lines.append("    ctx.source = source;")
    lines.append("    ctx.token_list = token_list;")
    lines.append("    ctx.options = options;")
    lines.append("    ctx.comment_map = syntaqlite_comment_map_build(source, token_list);")
    lines.append("    ctx.token_cursor = 0;")
    lines.append("")
    lines.append("    // Flush any leading comments before the first real token so they")
    lines.append("    // don't end up inside the statement's group (where a hardline")
    lines.append("    // from a -- comment would force the group to break).")
    lines.append("    uint32_t _pre_comments = SYNTAQLITE_NULL_DOC;")
    lines.append("    {")
    lines.append("        uint32_t _pre[24];")
    lines.append("        uint32_t _pn = 0;")
    lines.append("        while (ctx.token_cursor < ctx.token_list->count) {")
    lines.append("            SyntaqliteRawToken *_pt = &ctx.token_list->data[ctx.token_cursor];")
    lines.append("            if (_pt->type == TK_SPACE) { ctx.token_cursor++; continue; }")
    lines.append("            if (_pt->type == TK_COMMENT && ctx.comment_map &&")
    lines.append("                ctx.comment_map->kinds[ctx.token_cursor] == SYNTAQLITE_COMMENT_LEADING) {")
    lines.append("                if (_pn < 24) _pre[_pn++] = emit_comment_doc(&ctx, ctx.token_cursor);")
    lines.append("                ctx.token_cursor++;")
    lines.append("                continue;")
    lines.append("            }")
    lines.append("            break;")
    lines.append("        }")
    lines.append("        if (_pn > 0) _pre_comments = (_pn == 1) ? _pre[0] : doc_concat(&ctx.docs, _pre, _pn);")
    lines.append("    }")
    lines.append("")
    lines.append("    uint32_t root_doc = format_node(&ctx, root_id);")
    lines.append("")
    lines.append("    // Prepend leading comments outside the statement group")
    lines.append("    if (_pre_comments != SYNTAQLITE_NULL_DOC && root_doc != SYNTAQLITE_NULL_DOC) {")
    lines.append("        uint32_t items[] = { _pre_comments, root_doc };")
    lines.append("        root_doc = doc_concat(&ctx.docs, items, 2);")
    lines.append("    }")
    lines.append("")
    lines.append("    // Flush any remaining comments (trailing after last token)")
    lines.append("    uint32_t _rem_trailing;")
    lines.append("    uint32_t _rem_leading = advance_with_comments(&ctx, &_rem_trailing);")
    lines.append("    uint32_t _flush[2];")
    lines.append("    uint32_t _fn = 0;")
    lines.append("    if (_rem_leading != SYNTAQLITE_NULL_DOC) _flush[_fn++] = _rem_leading;")
    lines.append("    if (_rem_trailing != SYNTAQLITE_NULL_DOC) _flush[_fn++] = _rem_trailing;")
    lines.append("    if (_fn > 0 && root_doc != SYNTAQLITE_NULL_DOC) {")
    lines.append("        uint32_t _flush_doc = (_fn == 1) ? _flush[0] : doc_concat(&ctx.docs, _flush, 2);")
    lines.append("        uint32_t items[] = { root_doc, _flush_doc };")
    lines.append("        root_doc = doc_concat(&ctx.docs, items, 2);")
    lines.append("    }")
    lines.append("")
    lines.append("    syntaqlite_comment_map_free(ctx.comment_map);")
    lines.append("")
    lines.append("    if (root_doc == SYNTAQLITE_NULL_DOC) {")
    lines.append("        syntaqlite_doc_context_cleanup(&ctx.docs);")
    lines.append("        char *empty = (char*)malloc(1);")
    lines.append("        if (empty) empty[0] = '\\0';")
    lines.append("        return empty;")
    lines.append("    }")
    lines.append("")
    lines.append("    char *result = syntaqlite_doc_layout(&ctx.docs, root_doc, options->target_width);")
    lines.append("    syntaqlite_doc_context_cleanup(&ctx.docs);")
    lines.append("    return result;")
    lines.append("}")
    lines.append("")

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n".join(lines))
