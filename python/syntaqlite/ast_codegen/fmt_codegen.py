# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Generate src/fmt/fmt.c from FmtDoc annotations on node definitions.

Compiles the FmtDoc DSL tree on each NodeDef/ListDef into C formatting
functions, then emits the format_node dispatcher and public API.
"""

from __future__ import annotations

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


class _Compiler:
    """Compiles a FmtDoc tree into C statements that build a doc tree."""

    def __init__(self, node_def: AnyNodeDef):
        self.node_def = node_def
        self.var_counter = 0
        self.lines: list[str] = []

    def _var(self, prefix: str = "d") -> str:
        self.var_counter += 1
        return f"{prefix}_{self.var_counter}"

    def _accessor(self, field: str) -> str:
        """Get C accessor for a field."""
        return f"node->{field}"

    def compile(self, doc: FmtDoc, indent: str = "    ") -> str:
        """Compile a FmtDoc and return the C variable name holding the doc ID."""
        ind = indent
        inner_ind = indent + "    "

        if isinstance(doc, FmtKw):
            v = self._var("kw")
            self.lines.append(f"{ind}uint32_t {v} = kw(ctx, {_c_str(doc.text)});")
            return v

        if isinstance(doc, FmtSpan):
            v = self._var("sp")
            self.lines.append(f"{ind}uint32_t {v} = span_text(ctx, {self._accessor(doc.field)});")
            return v

        if isinstance(doc, FmtChild):
            if doc.field == "_item":
                v = self._var("item")
                self.lines.append(f"{ind}uint32_t {v} = format_node(ctx, _child_id);")
                return v
            v = self._var("ch")
            self.lines.append(f"{ind}uint32_t {v} = format_node(ctx, {self._accessor(doc.field)});")
            return v

        if isinstance(doc, FmtLine):
            v = self._var("ln")
            self.lines.append(f"{ind}uint32_t {v} = doc_line(&ctx->docs);")
            return v

        if isinstance(doc, FmtSoftline):
            v = self._var("sl")
            self.lines.append(f"{ind}uint32_t {v} = doc_softline(&ctx->docs);")
            return v

        if isinstance(doc, FmtHardline):
            v = self._var("hl")
            self.lines.append(f"{ind}uint32_t {v} = doc_hardline(&ctx->docs);")
            return v

        if isinstance(doc, FmtSeq):
            # Compile children, collect non-NULL into stack array, single alloc.
            n = len(doc.items)
            item_vars = [self.compile(item, ind) for item in doc.items]
            buf = self._var("_buf")
            cnt = self._var("_n")
            v = self._var("cat")
            self.lines.append(f"{ind}uint32_t {buf}[{n}];")
            self.lines.append(f"{ind}uint32_t {cnt} = 0;")
            for item_var in item_vars:
                self.lines.append(f"{ind}if ({item_var} != SYNTAQLITE_NULL_DOC) {buf}[{cnt}++] = {item_var};")
            self.lines.append(f"{ind}uint32_t {v} = doc_concat(&ctx->docs, {buf}, {cnt});")
            return v

        if isinstance(doc, FmtGroup):
            inner = self.compile(doc.child, ind)
            v = self._var("grp")
            self.lines.append(f"{ind}uint32_t {v} = doc_group(&ctx->docs, {inner});")
            return v

        if isinstance(doc, FmtNest):
            inner = self.compile(doc.child, ind)
            v = self._var("nst")
            self.lines.append(
                f"{ind}uint32_t {v} = doc_nest(&ctx->docs,"
                f" (int32_t)ctx->options->indent_width, {inner});")
            return v

        if isinstance(doc, FmtIfSet):
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
            body = self.compile(doc.body, ind)
            v = self._var("cl")
            self.lines.append(
                f"{ind}uint32_t {v} = format_clause(ctx,"
                f" {_c_str(doc.keyword)}, {body});")
            return v

        if isinstance(doc, FmtEnumDisplay):
            return self._compile_enum_display(doc, ind)

        if isinstance(doc, FmtSwitch):
            return self._compile_switch(doc, ind)

        if isinstance(doc, FmtForEachChild):
            return self._compile_for_each_child(doc, ind)

        raise TypeError(f"Unknown FmtDoc type: {type(doc)}")

    def _compile_if(self, ind: str, condition: str,
                    then_doc: FmtDoc, else_doc: FmtDoc | None) -> str:
        inner_ind = ind + "    "
        v = self._var("cond")
        self.lines.append(f"{ind}uint32_t {v} = SYNTAQLITE_NULL_DOC;")
        self.lines.append(f"{ind}if ({condition}) {{")
        then_var = self.compile(then_doc, inner_ind)
        self.lines.append(f"{inner_ind}{v} = {then_var};")
        if else_doc:
            self.lines.append(f"{ind}}} else {{")
            else_var = self.compile(else_doc, inner_ind)
            self.lines.append(f"{inner_ind}{v} = {else_var};")
        self.lines.append(f"{ind}}}")
        return v

    def _compile_enum_display(self, doc: FmtEnumDisplay, ind: str) -> str:
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
        return v

    def _compile_switch(self, doc: FmtSwitch, ind: str) -> str:
        """Multi-branch on enum field with arbitrary FmtDoc bodies."""
        case_ind = ind + "    "
        body_ind = ind + "        "
        v = self._var("sw")
        self.lines.append(f"{ind}uint32_t {v} = SYNTAQLITE_NULL_DOC;")
        self.lines.append(f"{ind}switch ({self._accessor(doc.field)}) {{")
        for case_val, case_doc in doc.cases.items():
            c_enum = self._resolve_enum_value(doc.field, case_val)
            self.lines.append(f"{case_ind}case {c_enum}: {{")
            body_var = self.compile(case_doc, body_ind)
            self.lines.append(f"{body_ind}{v} = {body_var};")
            self.lines.append(f"{body_ind}break;")
            self.lines.append(f"{case_ind}}}")
        if doc.default:
            self.lines.append(f"{case_ind}default: {{")
            def_var = self.compile(doc.default, body_ind)
            self.lines.append(f"{body_ind}{v} = {def_var};")
            self.lines.append(f"{body_ind}break;")
            self.lines.append(f"{case_ind}}}")
        else:
            self.lines.append(f"{case_ind}default: break;")
        self.lines.append(f"{ind}}}")
        return v

    def _compile_for_each_child(self, doc: FmtForEachChild, ind: str) -> str:
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
            sep_var = self.compile(doc.separator, sep_ind)
            self.lines.append(f"{sep_ind}{buf}[{cnt}++] = {sep_var};")
            self.lines.append(f"{loop_ind}}}")
        template_var = self.compile(doc.template, loop_ind)
        self.lines.append(f"{loop_ind}{buf}[{cnt}++] = {template_var};")
        self.lines.append(f"{ind}}}")
        self.lines.append(f"{ind}uint32_t {v} = doc_concat(&ctx->docs, {buf}, {cnt});")
        return v

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
    lines.append('#include "src/fmt/doc.h"')
    lines.append('#include "src/fmt/doc_layout.h"')
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
    lines.append("} FmtCtx;")
    lines.append("")

    # Helper functions
    lines.append("// ============ Helpers ============")
    lines.append("")
    lines.append("static uint32_t kw(FmtCtx *ctx, const char *text) {")
    lines.append("    return doc_text(&ctx->docs, text, (uint32_t)strlen(text));")
    lines.append("}")
    lines.append("")
    lines.append("static uint32_t span_text(FmtCtx *ctx, SyntaqliteSourceSpan span) {")
    lines.append("    if (span.length == 0) return SYNTAQLITE_NULL_DOC;")
    lines.append("    return doc_text(&ctx->docs, ctx->source + span.offset, span.length);")
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
    lines.append("    return doc_concat(&ctx->docs, buf, n);")
    lines.append("}")
    lines.append("")

    # Clause helper
    lines.append("// ============ Clause Helper ============")
    lines.append("")
    lines.append("static uint32_t format_clause(FmtCtx *ctx, const char *keyword, uint32_t body_doc) {")
    lines.append("    if (body_doc == SYNTAQLITE_NULL_DOC) return SYNTAQLITE_NULL_DOC;")
    lines.append("    uint32_t inner_items[] = { doc_line(&ctx->docs), body_doc };")
    lines.append("    uint32_t inner = doc_concat(&ctx->docs, inner_items, 2);")
    lines.append("    uint32_t items[] = {")
    lines.append("        doc_line(&ctx->docs),")
    lines.append("        kw(ctx, keyword),")
    lines.append("        doc_nest(&ctx->docs, (int32_t)ctx->options->indent_width, inner),")
    lines.append("    };")
    lines.append("    return doc_concat(&ctx->docs, items, 3);")
    lines.append("}")
    lines.append("")

    # Node formatters
    lines.append("// ============ Node Formatters ============")
    lines.append("")

    for node in node_defs:
        if node.fmt is None:
            continue

        snake = pascal_to_snake(node.name)
        struct = _struct_name(node.name)

        compiler = _Compiler(node)
        result_var = compiler.compile(node.fmt)

        lines.append(f"static uint32_t format_{snake}(FmtCtx *ctx, {struct} *node) {{")
        lines.extend(compiler.lines)
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
    lines.append("")
    lines.append("    uint32_t root_doc = format_node(&ctx, root_id);")
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
