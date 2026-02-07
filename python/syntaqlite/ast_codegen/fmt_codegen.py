# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Generate src/fmt/fmt_gen.c from FmtDoc annotations on node definitions.

Compiles the FmtDoc DSL tree on each NodeDef/ListDef into static const
SynqFmtOp arrays (format recipes), then emits a dispatch function that
calls the interpreter for each node type.
"""

from __future__ import annotations

from pathlib import Path

from .defs import (
    AnyNodeDef,
    NodeDef,
    ListDef,
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


def _enum_prefix(enum_name: str) -> str:
    return f"SYNTAQLITE_{pascal_to_snake(enum_name).upper()}"


def _c_str(s: str) -> str:
    """Escape a string for C."""
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


class _OpCompiler:
    """Compiles a FmtDoc tree into a flat list of SynqFmtOp C initializers."""

    def __init__(self, node_def: AnyNodeDef, flags_lookup: dict[str, FlagsDef]):
        self.node_def = node_def
        self.node_name = node_def.name  # PascalCase without Synq prefix
        self.flags_lookup = flags_lookup
        self.ops: list[str] = []
        self.aux_lines: list[str] = []  # supporting data (enum entry arrays)
        self.aux_counter = 0

    def _aux_var(self, suffix: str) -> str:
        self.aux_counter += 1
        snake = pascal_to_snake(self.node_name)
        return f"fmt_{snake}_{suffix}_{self.aux_counter}"

    def compile(self, doc: FmtDoc) -> None:
        """Compile a FmtDoc tree into the ops list (prefix-tree order)."""
        T = self.node_name

        # Apply optimizations first.
        if self._try_optimize(doc):
            return

        if isinstance(doc, FmtKw):
            self.ops.append(f'FOP_KW({_c_str(doc.text)})')

        elif isinstance(doc, FmtSpan):
            self.ops.append(f'FOP_SPAN({T}, {doc.field})')

        elif isinstance(doc, FmtChild):
            if doc.field == "_item":
                self.ops.append('FOP_CHILD_ITEM')
            else:
                self.ops.append(f'FOP_CHILD({T}, {doc.field})')

        elif isinstance(doc, FmtLine):
            self.ops.append('FOP_LINE')

        elif isinstance(doc, FmtSoftline):
            self.ops.append('FOP_SOFTLINE')

        elif isinstance(doc, FmtHardline):
            self.ops.append('FOP_HARDLINE')

        elif isinstance(doc, FmtGroup):
            self.ops.append('FOP_GROUP')
            self.compile(doc.child)

        elif isinstance(doc, FmtNest):
            self.ops.append('FOP_NEST')
            self.compile(doc.child)

        elif isinstance(doc, FmtSeq):
            self.ops.append(f'FOP_SEQ({len(doc.items)})')
            for item in doc.items:
                self.compile(item)

        elif isinstance(doc, FmtIfSet):
            self._compile_if_set(doc)

        elif isinstance(doc, FmtIfFlag):
            self._compile_if_flag(doc)

        elif isinstance(doc, FmtIfEnum):
            self._compile_if_enum(doc)

        elif isinstance(doc, FmtIfSpan):
            self._compile_if_span(doc)

        elif isinstance(doc, FmtClause):
            self._compile_clause(doc)

        elif isinstance(doc, FmtEnumDisplay):
            self._compile_enum_display(doc)

        elif isinstance(doc, FmtSwitch):
            self._compile_switch(doc)

        elif isinstance(doc, FmtForEachChild):
            self._compile_for_each(doc)

        else:
            raise TypeError(f"Unknown FmtDoc type: {type(doc)}")

    # ---- Optimizations ----

    def _try_optimize(self, doc: FmtDoc) -> bool:
        """Try to apply peephole optimizations. Returns True if handled."""
        T = self.node_name

        if not isinstance(doc, FmtIfSet) or doc.else_ is not None:
            return False

        # if_set(field, clause(kw, child(field))) → FOP_CLAUSE
        # synq_format_clause handles NULL internally, so if_set is redundant.
        if (isinstance(doc.then, FmtClause)
                and isinstance(doc.then.body, FmtChild)
                and doc.then.body.field == doc.field):
            self.ops.append(
                f'FOP_CLAUSE({T}, {doc.field}, {_c_str(doc.then.keyword)})')
            return True

        # if_set(field, child(field)) → FOP_CHILD
        # synq_format_node handles NULL internally, so if_set is redundant.
        if isinstance(doc.then, FmtChild) and doc.then.field == doc.field:
            self.ops.append(f'FOP_CHILD({T}, {doc.field})')
            return True

        return False

    # ---- Conditional compilers ----

    def _compile_if_set(self, doc: FmtIfSet) -> None:
        T = self.node_name
        if doc.else_ is not None:
            self.ops.append(f'FOP_IF_SET_ELSE({T}, {doc.field})')
            self.compile(doc.then)
            self.compile(doc.else_)
        else:
            self.ops.append(f'FOP_IF_SET({T}, {doc.field})')
            self.compile(doc.then)

    def _compile_if_flag(self, doc: FmtIfFlag) -> None:
        T = self.node_name
        if '.' in doc.field:
            # Flags union field: flags.bitname → read uint8_t, test mask.
            field_base, bit_name = doc.field.split('.', 1)
            mask = self._resolve_flag_mask(field_base, bit_name)
            if doc.else_ is not None:
                self.ops.append(f'FOP_IF_FLAG_ELSE({T}, {field_base}, {mask})')
            else:
                self.ops.append(f'FOP_IF_FLAG({T}, {field_base}, {mask})')
        else:
            # Bool field: use IF_ENUM (Bool is int-sized, IF_FLAG reads uint8_t).
            if doc.else_ is not None:
                self.ops.append(
                    f'FOP_IF_ENUM_ELSE({T}, {doc.field}, SYNTAQLITE_BOOL_TRUE)')
            else:
                self.ops.append(
                    f'FOP_IF_ENUM({T}, {doc.field}, SYNTAQLITE_BOOL_TRUE)')

        self.compile(doc.then)
        if doc.else_ is not None:
            self.compile(doc.else_)

    def _compile_if_enum(self, doc: FmtIfEnum) -> None:
        T = self.node_name
        c_value = self._resolve_enum_value(doc.field, doc.value)
        if doc.else_ is not None:
            self.ops.append(f'FOP_IF_ENUM_ELSE({T}, {doc.field}, {c_value})')
            self.compile(doc.then)
            self.compile(doc.else_)
        else:
            self.ops.append(f'FOP_IF_ENUM({T}, {doc.field}, {c_value})')
            self.compile(doc.then)

    def _compile_if_span(self, doc: FmtIfSpan) -> None:
        T = self.node_name
        if doc.else_ is not None:
            self.ops.append(f'FOP_IF_SPAN_ELSE({T}, {doc.field})')
            self.compile(doc.then)
            self.compile(doc.else_)
        else:
            self.ops.append(f'FOP_IF_SPAN({T}, {doc.field})')
            self.compile(doc.then)

    # ---- Pattern compilers ----

    def _compile_clause(self, doc: FmtClause) -> None:
        T = self.node_name
        if isinstance(doc.body, FmtChild) and doc.body.field != "_item":
            self.ops.append(
                f'FOP_CLAUSE({T}, {doc.body.field}, {_c_str(doc.keyword)})')
        else:
            raise TypeError(
                f"CLAUSE body must be child(field), got: {doc.body}")

    def _compile_enum_display(self, doc: FmtEnumDisplay) -> None:
        T = self.node_name
        arr_name = self._aux_var("entries")
        n = len(doc.mapping)

        # Emit static entries array before the op array.
        self.aux_lines.append(
            f"static const SynqFmtEnumEntry {arr_name}[] = {{")
        for enum_val, display_str in doc.mapping.items():
            c_enum = self._resolve_enum_value(doc.field, enum_val)
            self.aux_lines.append(
                f"    {{ {c_enum}, {_c_str(display_str)} }},")
        self.aux_lines.append("};")
        self.aux_lines.append("")

        self.ops.append(
            f'FOP_ENUM_DISPLAY({T}, {doc.field}, {arr_name}, {n})')

    def _compile_switch(self, doc: FmtSwitch) -> None:
        T = self.node_name
        n = len(doc.cases)
        if doc.default:
            self.ops.append(f'FOP_SWITCH_DEFAULT({T}, {doc.field}, {n})')
        else:
            self.ops.append(f'FOP_SWITCH({T}, {doc.field}, {n})')

        for case_val, case_doc in doc.cases.items():
            c_enum = self._resolve_enum_value(doc.field, case_val)
            self.ops.append(f'FOP_CASE({c_enum})')
            self.compile(case_doc)

        if doc.default:
            self.compile(doc.default)

    def _compile_for_each(self, doc: FmtForEachChild) -> None:
        if doc.separator:
            self.ops.append('FOP_FOR_EACH_SEP')
            self.compile(doc.template)
            self.compile(doc.separator)
        else:
            self.ops.append('FOP_FOR_EACH')
            self.compile(doc.template)

    # ---- Type resolution ----

    def _resolve_enum_value(self, field_name: str, value: str) -> str:
        """Resolve an enum value string to its C constant name."""
        type_name = self._get_field_type_name(field_name)
        prefix = _enum_prefix(type_name)
        return f"{prefix}_{value}"

    def _get_field_type_name(self, field_name: str) -> str:
        """Get the type name for a field from the node definition."""
        if isinstance(self.node_def, NodeDef) and field_name in self.node_def.fields:
            field = self.node_def.fields[field_name]
            if isinstance(field, InlineField):
                return field.type_name
        # Fallback: derive from field name.
        return field_name

    def _resolve_flag_mask(self, field_base: str, bit_name: str) -> str:
        """Resolve flags.bitname to a hex mask string."""
        if isinstance(self.node_def, NodeDef) and field_base in self.node_def.fields:
            field = self.node_def.fields[field_base]
            if isinstance(field, InlineField):
                flags_def = self.flags_lookup.get(field.type_name)
                if flags_def:
                    for name, value in flags_def.flags.items():
                        if name.lower() == bit_name.lower():
                            return f"0x{value:02x}"
        raise ValueError(
            f"Cannot resolve flag mask for "
            f"{self.node_name}.{field_base}.{bit_name}")


def _compile_recipes(
    node_defs: list[AnyNodeDef],
    flags_lookup: dict[str, FlagsDef],
) -> tuple[list[tuple[str, _OpCompiler]], set[str]]:
    """Compile FmtDoc annotations into op arrays.

    Returns (compiled_nodes, fmt_names) where compiled_nodes is a list of
    (snake_name, compiler) pairs and fmt_names is the set of snake names
    that have fmt annotations.
    """
    compiled: list[tuple[str, _OpCompiler]] = []
    for node in node_defs:
        if node.fmt is None:
            continue
        snake = pascal_to_snake(node.name)
        compiler = _OpCompiler(node, flags_lookup)
        compiler.compile(node.fmt)
        compiled.append((snake, compiler))
    fmt_names = {s for s, _ in compiled}
    return compiled, fmt_names


def _emit_recipe_arrays(
    lines: list[str],
    compiled_nodes: list[tuple[str, _OpCompiler]],
) -> None:
    """Emit static const SynqFmtOp arrays for each compiled node."""
    for snake, compiler in compiled_nodes:
        lines.extend(compiler.aux_lines)
        lines.append(f"static const SynqFmtOp fmt_{snake}[] = {{")
        for op in compiler.ops:
            lines.append(f"    {op},")
        lines.append("};")
        lines.append("")


def generate_fmt_c(
    node_defs: list[AnyNodeDef],
    flags_defs: list[FlagsDef],
    output: Path,
) -> None:
    """Generate src/fmt/fmt_gen.c with static format recipes + dispatcher."""
    flags_lookup = {f.name: f for f in flags_defs}
    lines: list[str] = []

    # Header
    lines.append("// Copyright 2025 The syntaqlite Authors. All rights reserved.")
    lines.append("// Licensed under the Apache License, Version 2.0.")
    lines.append("")
    lines.append("// Generated from ast_codegen node definitions - DO NOT EDIT")
    lines.append("// Regenerate with: python3 python/tools/extract_sqlite.py")
    lines.append("")
    lines.append('#include "src/formatter/fmt_ops.h"')
    lines.append("")
    lines.append('#include "src/parser/ast_nodes_gen.h"')
    lines.append("")

    compiled_nodes, fmt_names = _compile_recipes(node_defs, flags_lookup)

    lines.append("// ============ Format Recipes ============")
    lines.append("")
    _emit_recipe_arrays(lines, compiled_nodes)

    # Recipe lookup table (indexed by SynqNodeTag).
    lines.append("// ============ Recipe Table ============")
    lines.append("")
    lines.append("const SynqFmtOp *synq_fmt_recipes[SYNTAQLITE_NODE_COUNT] = {")

    for node in node_defs:
        tag = _tag_name(node.name)
        snake = pascal_to_snake(node.name)

        if node.fmt is not None:
            lines.append(f"    [{tag}] = fmt_{snake},")
        elif isinstance(node, ListDef):
            lines.append(f"    [{tag}] = synq_fmt_default_comma_list,")

    lines.append("};")
    lines.append("")

    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n".join(lines))


def generate_extension_fmt_c(
    node_defs: list[AnyNodeDef],
    flags_defs: list[FlagsDef] | None = None,
) -> str:
    """Generate format recipes for extension nodes (header-safe).

    Returns a C code string with:
    - Static const recipe arrays per annotated node
    - A lookup table synq_fmt_extension_recipes[] indexed by (tag - SYNTAQLITE_NODE_COUNT)
    - A #define SYNTAQLITE_EXTENSION_NODE_COUNT for bounds checking
    """
    if flags_defs is None:
        flags_defs = []
    flags_lookup = {f.name: f for f in flags_defs}
    lines: list[str] = []

    lines.append('#include "src/formatter/fmt_ops.h"')
    lines.append("")

    compiled_nodes, fmt_names = _compile_recipes(node_defs, flags_lookup)

    if not compiled_nodes:
        return ""

    _emit_recipe_arrays(lines, compiled_nodes)

    # Lookup table indexed by (tag - SYNTAQLITE_NODE_COUNT).
    # Extension tags are contiguous #defines starting at SYNTAQLITE_NODE_COUNT+0.
    # We need entries for ALL extension nodes (not just fmt-annotated ones),
    # so unannotated slots get NULL.
    lines.append(f"#define SYNTAQLITE_EXTENSION_NODE_COUNT {len(node_defs)}")
    lines.append("")
    lines.append(
        "static const SynqFmtOp* synq_fmt_extension_recipes"
        "[SYNTAQLITE_EXTENSION_NODE_COUNT] = {")
    for node in node_defs:
        snake = pascal_to_snake(node.name)
        if snake in fmt_names:
            lines.append(f"    fmt_{snake},")
        else:
            lines.append("    NULL,")
    lines.append("};")
    lines.append("")

    return "\n".join(lines)
