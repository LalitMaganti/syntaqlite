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
    return f"SYNQ_NODE_{pascal_to_snake(node_name).upper()}"


def _struct_name(node_name: str) -> str:
    return f"Synq{node_name}"


def _enum_prefix(enum_name: str) -> str:
    return f"SYNQ_{pascal_to_snake(enum_name).upper()}"


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
            self.lines.append(f"{ind}uint32_t {v} = synq_kw(ctx, {_c_str(doc.text)});")
            return _Result(v, False)

        if isinstance(doc, FmtSpan):
            v = self._var("sp")
            self.lines.append(f"{ind}uint32_t {v} = synq_span_text(ctx, {self._accessor(doc.field)});")
            return _Result(v, True)

        if isinstance(doc, FmtChild):
            if doc.field == "_item":
                v = self._var("item")
                self.lines.append(f"{ind}uint32_t {v} = synq_format_node(ctx, _child_id);")
                return _Result(v, True)
            v = self._var("ch")
            self.lines.append(f"{ind}uint32_t {v} = synq_format_node(ctx, {self._accessor(doc.field)});")
            return _Result(v, True)

        if isinstance(doc, FmtLine):
            v = self._var("ln")
            self.lines.append(f"{ind}uint32_t {v} = synq_doc_line(&ctx->docs);")
            return _Result(v, False)

        if isinstance(doc, FmtSoftline):
            v = self._var("sl")
            self.lines.append(f"{ind}uint32_t {v} = synq_doc_softline(&ctx->docs);")
            return _Result(v, False)

        if isinstance(doc, FmtHardline):
            v = self._var("hl")
            self.lines.append(f"{ind}uint32_t {v} = synq_doc_hardline(&ctx->docs);")
            return _Result(v, False)

        if isinstance(doc, FmtSeq):
            return self._compile_seq(doc, ind)

        if isinstance(doc, FmtGroup):
            inner = self.compile(doc.child, ind)
            v = self._var("grp")
            self.lines.append(f"{ind}uint32_t {v} = synq_doc_group(&ctx->docs, {inner.var});")
            return _Result(v, False)

        if isinstance(doc, FmtNest):
            inner = self.compile(doc.child, ind)
            v = self._var("nst")
            self.lines.append(
                f"{ind}uint32_t {v} = synq_doc_nest(&ctx->docs,"
                f" (int32_t)ctx->options->indent_width, {inner.var});")
            return _Result(v, False)

        if isinstance(doc, FmtIfSet):
            # Optimization: if_set(field, clause(kw, child(field))) -> format_clause(ctx, kw, format_node(...))
            opt = self._try_optimize_if_set(doc, ind)
            if opt is not None:
                return opt
            return self._compile_if(ind,
                f"{self._accessor(doc.field)} != SYNQ_NULL_NODE",
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
            if isinstance(doc.body, FmtChild) and doc.body.field != "_item":
                v = self._var("cl")
                self.lines.append(
                    f"{ind}uint32_t {v} = synq_format_clause(ctx,"
                    f" {_c_str(doc.keyword)}, {self._accessor(doc.body.field)});")
                return _Result(v, True)
            # Fallback: compile body normally (for non-child bodies)
            body = self.compile(doc.body, ind)
            v = self._var("cl")
            self.lines.append(
                f"{ind}uint32_t {v} = synq_format_clause(ctx,"
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
        """Compile FmtSeq - synq_doc_concat filters NULLs so always use it directly."""
        n = len(doc.items)
        item_results = [self.compile(item, ind) for item in doc.items]
        any_nullable = any(r.nullable for r in item_results)

        v = self._var("cat")
        items_str = ", ".join(r.var for r in item_results)
        self.lines.append(f"{ind}uint32_t {v}_items[] = {{ {items_str} }};")
        self.lines.append(f"{ind}uint32_t {v} = synq_doc_concat(&ctx->docs, {v}_items, {n});")
        return _Result(v, any_nullable)

    def _try_optimize_if_set(self, doc: FmtIfSet, ind: str) -> _Result | None:
        """Try to optimize if_set patterns into direct calls."""
        if doc.else_ is not None:
            return None

        # if_set(field, clause(kw, child(field))) -> format_clause(ctx, kw, node->field)
        if (isinstance(doc.then, FmtClause)
                and isinstance(doc.then.body, FmtChild)
                and doc.then.body.field == doc.field):
            v = self._var("cl")
            self.lines.append(
                f"{ind}uint32_t {v} = synq_format_clause(ctx,"
                f" {_c_str(doc.then.keyword)},"
                f" {self._accessor(doc.field)});")
            return _Result(v, True)

        # if_set(field, child(field)) -> format_node(ctx, node->field)
        if (isinstance(doc.then, FmtChild)
                and doc.then.field == doc.field):
            v = self._var("ch")
            self.lines.append(
                f"{ind}uint32_t {v} = synq_format_node(ctx, {self._accessor(doc.field)});")
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

        # Both branches are single expressions -> ternary.
        if then_expr is not None and else_expr is not None:
            nullable = then_result.nullable or else_result.nullable
            self.lines.append(
                f"{ind}uint32_t {v} = {condition}"
                f" ? {then_expr} : {else_expr};")
            return _Result(v, nullable)

        # Single-expression then, no else -> one-line if.
        if then_expr is not None and else_doc is None:
            self.lines.append(f"{ind}uint32_t {v} = SYNQ_NULL_DOC;")
            self.lines.append(f"{ind}if ({condition}) {v} = {then_expr};")
            return _Result(v, True)

        # General case: emit block structure.
        self.lines.append(f"{ind}uint32_t {v} = SYNQ_NULL_DOC;")
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
        self.lines.append(f"{ind}uint32_t {v} = SYNQ_NULL_DOC;")
        self.lines.append(f"{ind}switch ({self._accessor(doc.field)}) {{")
        for enum_val, display_str in doc.mapping.items():
            c_enum = self._resolve_enum_value(doc.field, enum_val)
            self.lines.append(
                f"{ind}    case {c_enum}:"
                f" {v} = synq_kw(ctx, {_c_str(display_str)}); break;")
        self.lines.append(f"{ind}    default: break;")
        self.lines.append(f"{ind}}}")
        return _Result(v, True)

    def _compile_switch(self, doc: FmtSwitch, ind: str) -> _Result:
        """Multi-branch on enum field with arbitrary FmtDoc bodies."""
        case_ind = ind + "    "
        body_ind = ind + "        "
        v = self._var("sw")
        self.lines.append(f"{ind}uint32_t {v} = SYNQ_NULL_DOC;")
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
        self.lines.append(f"{ind}uint32_t {v} = synq_doc_concat(&ctx->docs, {buf}, {cnt});")
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
    lines.append('#include "src/fmt/fmt_helpers.h"')
    lines.append("")
    lines.append('#include "src/ast/ast_nodes_gen.h"')
    lines.append("")

    # Compile all node formatters
    compiled_nodes: list[tuple[str, str, _Compiler, _Result]] = []

    for node in node_defs:
        if node.fmt is None:
            continue

        snake = pascal_to_snake(node.name)
        struct = _struct_name(node.name)

        compiler = _Compiler(node)
        result = compiler.compile(node.fmt)
        compiled_nodes.append((snake, struct, compiler, result))

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
                lines.append(f"static uint32_t format_{snake}(SynqFmtCtx *ctx, {struct} *node) {{")
                lines.extend(func_lines)
                lines.append(f"    return {expr};")
                lines.append("}")
                lines.append("")
                continue

        lines.append(f"static uint32_t format_{snake}(SynqFmtCtx *ctx, {struct} *node) {{")
        lines.extend(func_lines)
        lines.append(f"    return {result_var};")
        lines.append("}")
        lines.append("")

    # Dispatch function (pure switch, no comment handling)
    lines.append("// ============ Dispatch ============")
    lines.append("")
    lines.append("uint32_t synq_dispatch_format(SynqFmtCtx *ctx, uint32_t node_id) {")
    lines.append("    SynqNode *node = AST_NODE(ctx->ast, node_id);")
    lines.append("    if (!node) return SYNQ_NULL_DOC;")
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
                f"            return synq_format_comma_list(ctx,"
                f" node->{snake}.children, node->{snake}.count);")

    lines.append("        default:")
    lines.append('            return synq_kw(ctx, "/* UNSUPPORTED */");')
    lines.append("    }")
    lines.append("}")
    lines.append("")

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n".join(lines))
