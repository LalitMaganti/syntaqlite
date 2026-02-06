# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Generate C code from AST node definitions.

This module produces:
- ast_nodes.h: Node structs, union, arena types, tag enum, size table
- ast_builder.h: Builder function declarations
- ast_builder.c: Builder implementations with arena allocation
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
    IndexField,
    pascal_to_snake,
)


# Mapping from our type names to C types
TYPE_MAP = {
    "u8": "uint8_t",
    "u16": "uint16_t",
    "u32": "uint32_t",
    "u64": "uint64_t",
    "i8": "int8_t",
    "i16": "int16_t",
    "i32": "int32_t",
    "i64": "int64_t",
}


def _flags_type_name(flags_name: str) -> str:
    """Generate C union type name from flags name."""
    return f"Synq{flags_name}"


def _field_c_type(field_type, enum_names: set[str], flags_names: set[str] | None = None) -> str:
    """Get the C type string for an inline or index field."""
    if isinstance(field_type, IndexField):
        return "uint32_t"
    type_name = field_type.type_name
    if type_name in TYPE_MAP:
        return TYPE_MAP[type_name]
    if type_name in enum_names:
        return f"Synq{type_name}"
    if flags_names and type_name in flags_names:
        return _flags_type_name(type_name)
    return type_name


def _struct_name(node_name: str) -> str:
    """Generate C struct name from node name."""
    return f"Synq{node_name}"


def _tag_name(node_name: str) -> str:
    """Generate enum tag name from node name."""
    return f"SYNQ_NODE_{pascal_to_snake(node_name).upper()}"


def _builder_name(node_name: str) -> str:
    """Generate builder function name from node name."""
    return f"synq_ast_{pascal_to_snake(node_name)}"


def _enum_prefix(enum_name: str) -> str:
    """Generate enum value prefix from enum name."""
    return f"SYNQ_{pascal_to_snake(enum_name).upper()}"


def _build_node_params(node: NodeDef, enum_names: set[str], flags_names: set[str] | None = None) -> list[str]:
    """Build C parameter list for a node builder function."""
    params = ["SynqAstContext *ctx"]
    for field_name, field_type in node.fields.items():
        c_type = _field_c_type(field_type, enum_names, flags_names)
        params.append(f"{c_type} {field_name}")
    return params


def _emit_func_signature(lines: list[str], func_name: str, params: list[str], end: str = ";") -> None:
    """Emit a function signature, wrapping long parameter lists."""
    params_str = ", ".join(params)
    if len(params_str) > 80:
        lines.append(f"uint32_t {func_name}(")
        for i, param in enumerate(params):
            comma = "," if i < len(params) - 1 else ""
            lines.append(f"    {param}{comma}")
        lines.append(f"){end}")
    else:
        lines.append(f"uint32_t {func_name}({params_str}){end}")


def _emit_enums(lines: list[str], enum_defs: list[EnumDef]) -> None:
    """Emit enum typedefs."""
    if not enum_defs:
        return
    lines.append("// ============ Value Enums ============")
    lines.append("")
    for enum in enum_defs:
        prefix = _enum_prefix(enum.name)
        lines.append(f"typedef enum {{")
        for i, value in enumerate(enum.values):
            lines.append(f"    {prefix}_{value} = {i},")
        lines.append(f"}} Synq{enum.name};")
        lines.append("")


def _emit_enum_name_arrays(lines: list[str], enum_defs: list[EnumDef]) -> None:
    """Emit static name string arrays for enums (internal only)."""
    if not enum_defs:
        return
    for enum in enum_defs:
        var_name = f"synq_{pascal_to_snake(enum.name)}_names"
        lines.append(f"static const char* const {var_name}[] = {{")
        for value in enum.values:
            lines.append(f'    "{value}",')
        lines.append("};")
        lines.append("")


def _emit_flags(lines: list[str], flags_defs: list[FlagsDef]) -> None:
    """Emit flags union typedefs."""
    if not flags_defs:
        return
    lines.append("// ============ Flags Types ============")
    lines.append("")
    for fdef in flags_defs:
        type_name = _flags_type_name(fdef.name)
        lines.append(f"typedef union {type_name} {{")
        lines.append("    uint8_t raw;")
        lines.append("    struct {")
        sorted_flags = sorted(fdef.flags.items(), key=lambda x: x[1])
        next_bit = 0
        for name, value in sorted_flags:
            bit_pos = value.bit_length() - 1
            assert value == (1 << bit_pos), f"Flag {name}=0x{value:02x} must be a single bit"
            if bit_pos > next_bit:
                lines.append(f"        uint8_t : {bit_pos - next_bit};")
            lines.append(f"        uint8_t {name.lower()} : 1;")
            next_bit = bit_pos + 1
        lines.append("    };")
        lines.append(f"}} {type_name};")
        lines.append("")


def _emit_node_tags(lines: list[str], node_defs: list[AnyNodeDef]) -> None:
    """Emit node tag enum."""
    lines.append("// ============ Node Tags ============")
    lines.append("")
    lines.append("typedef enum {")
    lines.append("    SYNQ_NODE_NULL = 0,")
    for node in node_defs:
        lines.append(f"    {_tag_name(node.name)},")
    lines.append("    SYNQ_NODE_COUNT")
    lines.append("} SynqNodeTag;")
    lines.append("")


def _emit_node_structs(lines: list[str], node_defs: list[AnyNodeDef],
                       enum_names: set[str], flags_names: set[str]) -> None:
    """Emit node struct typedefs."""
    lines.append("// ============ Node Structs (variable sizes) ============")
    lines.append("")
    for node in node_defs:
        if isinstance(node, NodeDef):
            struct_name = _struct_name(node.name)
            lines.append(f"typedef struct {struct_name} {{")
            lines.append("    uint8_t tag;")
            for field_name, field_type in node.fields.items():
                c_type = _field_c_type(field_type, enum_names, flags_names)
                lines.append(f"    {c_type} {field_name};")
            lines.append(f"}} {struct_name};")
            lines.append("")
        elif isinstance(node, ListDef):
            struct_name = _struct_name(node.name)
            lines.append(f"// List of {node.child_type}")
            lines.append(f"typedef struct {struct_name} {{")
            lines.append("    uint8_t tag;")
            lines.append("    uint8_t _pad[3];")
            lines.append("    uint32_t count;")
            lines.append("    uint32_t children[];  // flexible array of indices")
            lines.append(f"}} {struct_name};")
            lines.append("")


def _emit_node_union(lines: list[str], node_defs: list[AnyNodeDef]) -> None:
    """Emit the SynqNode union."""
    lines.append("// ============ Node Union ============")
    lines.append("")
    lines.append("typedef union SynqNode {")
    lines.append("    uint8_t tag;")
    for node in node_defs:
        struct_name = _struct_name(node.name)
        field_name = pascal_to_snake(node.name)
        lines.append(f"    {struct_name} {field_name};")
    lines.append("} SynqNode;")
    lines.append("")


def generate_public_ast_nodes_h(node_defs: list[AnyNodeDef], enum_defs: list[EnumDef],
                                flags_defs: list[FlagsDef], output: Path) -> None:
    """Generate include/syntaqlite/ast_nodes_gen.h (public header).

    Self-contained header with only stdint.h/stddef.h dependencies.
    Contains: SYNQ_NULL_NODE, SynqSourceSpan, enums, flags, node structs, union.
    """
    flags_lookup = {f.name: f for f in flags_defs}
    enum_names = {e.name for e in enum_defs}
    flags_names = set(flags_lookup.keys())
    lines = []

    lines.append("// Copyright 2025 The syntaqlite Authors. All rights reserved.")
    lines.append("// Licensed under the Apache License, Version 2.0.")
    lines.append("")
    lines.append("// Generated from data/ast_nodes.py - DO NOT EDIT")
    lines.append("// Regenerate with: python3 python/tools/extract_sqlite.py")
    lines.append("")
    lines.append("#ifndef SYNTAQLITE_AST_NODES_GEN_H")
    lines.append("#define SYNTAQLITE_AST_NODES_GEN_H")
    lines.append("")
    lines.append("#ifdef SYNTAQLITE_CUSTOM_NODES")
    lines.append("#include SYNTAQLITE_CUSTOM_NODES")
    lines.append("#else")
    lines.append("")
    lines.append("#include <stddef.h>")
    lines.append("#include <stdint.h>")
    lines.append("")
    lines.append("#ifdef __cplusplus")
    lines.append('extern "C" {')
    lines.append("#endif")
    lines.append("")

    # SYNQ_NULL_NODE
    lines.append("// Null node ID (used for nullable fields)")
    lines.append("#define SYNQ_NULL_NODE 0xFFFFFFFFu")
    lines.append("")

    # SynqSourceSpan
    lines.append("// Source location span (offset + length into source text)")
    lines.append("typedef struct SynqSourceSpan {")
    lines.append("    uint32_t offset;")
    lines.append("    uint16_t length;")
    lines.append("} SynqSourceSpan;")
    lines.append("")

    _emit_enums(lines, enum_defs)
    _emit_flags(lines, flags_defs)
    _emit_node_tags(lines, node_defs)
    _emit_node_structs(lines, node_defs, enum_names, flags_names)
    _emit_node_union(lines, node_defs)

    lines.append("#ifdef __cplusplus")
    lines.append("}")
    lines.append("#endif")
    lines.append("")
    lines.append("#endif /* SYNTAQLITE_CUSTOM_NODES */")
    lines.append("")
    lines.append("#endif  // SYNTAQLITE_AST_NODES_GEN_H")

    output.write_text("\n".join(lines) + "\n")


def generate_ast_nodes_h(node_defs: list[AnyNodeDef], enum_defs: list[EnumDef],
                         flags_defs: list[FlagsDef], output: Path) -> None:
    """Generate src/ast/ast_nodes_gen.h (internal header).

    Thin wrapper that re-exports the public header plus internal additions:
    - _names[] string arrays (debug/print)
    - synq_ast_node() inline (needs SynqArena)
    - AST_NODE macro
    - synq_node_base_size() declaration
    """
    lines = []

    lines.append("// Copyright 2025 The syntaqlite Authors. All rights reserved.")
    lines.append("// Licensed under the Apache License, Version 2.0.")
    lines.append("")
    lines.append("// Generated from data/ast_nodes.py - DO NOT EDIT")
    lines.append("// Regenerate with: python3 python/tools/extract_sqlite.py")
    lines.append("")
    lines.append("#ifndef SYNQ_SRC_AST_AST_NODES_GEN_H")
    lines.append("#define SYNQ_SRC_AST_AST_NODES_GEN_H")
    lines.append("")
    lines.append("// Public types (enums, node structs, union)")
    lines.append('#include "syntaqlite/ast_nodes_gen.h"')
    lines.append("")
    lines.append("// Internal dependencies (SynqArena, etc.)")
    lines.append('#include "src/parser/ast_base.h"')
    lines.append("")
    lines.append("#ifdef __cplusplus")
    lines.append('extern "C" {')
    lines.append("#endif")
    lines.append("")

    # Enum name string arrays (internal only - causes static duplication in public header)
    _emit_enum_name_arrays(lines, enum_defs)

    # Node access (needs SynqArena from ast_base.h)
    lines.append("// Access node by ID")
    lines.append("inline SynqNode* synq_ast_node(SynqArena *ast, uint32_t id) {")
    lines.append("    if (id == SYNQ_NULL_NODE) return NULL;")
    lines.append("    return (SynqNode*)(ast->data + ast->offsets[id]);")
    lines.append("}")
    lines.append("")
    lines.append("#define AST_NODE(ast, id) synq_ast_node(ast, id)")
    lines.append("")

    # Node size table declaration
    lines.append("// ============ Node Size Table ============")
    lines.append("")
    lines.append("// Returns the fixed size of a node type (0 for variable-size nodes like lists)")
    lines.append("size_t synq_node_base_size(uint8_t tag);")
    lines.append("")

    lines.append("#ifdef __cplusplus")
    lines.append("}")
    lines.append("#endif")
    lines.append("")
    lines.append("#endif  // SYNQ_SRC_AST_AST_NODES_GEN_H")

    output.write_text("\n".join(lines) + "\n")


def generate_ast_builder_h(node_defs: list[AnyNodeDef], enum_defs: list[EnumDef],
                           flags_defs: list[FlagsDef], output: Path) -> None:
    """Generate src/ast/ast_builder.h with builder function declarations."""
    enum_names = {e.name for e in enum_defs}
    flags_names = {f.name for f in flags_defs}
    lines = []

    # Header
    lines.append("// Copyright 2025 The syntaqlite Authors. All rights reserved.")
    lines.append("// Licensed under the Apache License, Version 2.0.")
    lines.append("")
    lines.append("// Generated from data/ast_nodes.py - DO NOT EDIT")
    lines.append("// Regenerate with: python3 python/tools/generate_ast.py")
    lines.append("")
    lines.append("#ifndef SYNQ_SRC_AST_AST_BUILDER_GEN_H")
    lines.append("#define SYNQ_SRC_AST_AST_BUILDER_GEN_H")
    lines.append("")
    lines.append('#include "src/parser/ast_nodes_gen.h"')
    lines.append("")
    lines.append("#ifdef __cplusplus")
    lines.append('extern "C" {')
    lines.append("#endif")
    lines.append("")

    # Builder function declarations
    lines.append("// ============ Builder Functions ============")
    lines.append("")

    for node in node_defs:
        if isinstance(node, NodeDef):
            func_name = _builder_name(node.name)
            params = _build_node_params(node, enum_names, flags_names)
            _emit_func_signature(lines, func_name, params)
            lines.append("")

        elif isinstance(node, ListDef):
            func_name = _builder_name(node.name)
            lines.append(f"// Create empty {node.name}")
            lines.append(f"uint32_t {func_name}_empty(SynqAstContext *ctx);")
            lines.append("")
            lines.append(f"// Create {node.name} with single child")
            lines.append(f"uint32_t {func_name}(SynqAstContext *ctx, uint32_t first_child);")
            lines.append("")
            lines.append(f"// Append child to {node.name} (may reallocate, returns new list ID)")
            lines.append(f"uint32_t {func_name}_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child);")
            lines.append("")

    lines.append("#ifdef __cplusplus")
    lines.append("}")
    lines.append("#endif")
    lines.append("")
    lines.append("#endif  // SYNQ_SRC_AST_AST_BUILDER_GEN_H")

    output.write_text("\n".join(lines) + "\n")


def generate_ast_builder_c(node_defs: list[AnyNodeDef], enum_defs: list[EnumDef],
                           flags_defs: list[FlagsDef], output: Path) -> None:
    """Generate src/ast/ast_builder.c with builder implementations."""
    enum_names = {e.name for e in enum_defs}
    flags_names = {f.name for f in flags_defs}
    lines = []

    # Header
    lines.append("// Copyright 2025 The syntaqlite Authors. All rights reserved.")
    lines.append("// Licensed under the Apache License, Version 2.0.")
    lines.append("")
    lines.append("// Generated from data/ast_nodes.py - DO NOT EDIT")
    lines.append("// Regenerate with: python3 python/tools/generate_ast.py")
    lines.append("")
    lines.append('#include "src/parser/ast_builder_gen.h"')
    lines.append("")
    lines.append("#include <stdlib.h>")
    lines.append("#include <string.h>")
    lines.append("")
    lines.append("// External definition for inline function (C99/C11).")
    lines.append("extern inline SynqNode* synq_ast_node(SynqArena *ast, uint32_t id);")
    lines.append("")

    # Node size table - collect fixed-size nodes
    lines.append("// ============ Node Size Table ============")
    lines.append("")
    lines.append("static const size_t node_base_sizes[] = {")
    lines.append("    [SYNQ_NODE_NULL] = 0,")
    for node in node_defs:
        tag = _tag_name(node.name)
        struct_name = _struct_name(node.name)
        lines.append(f"    [{tag}] = sizeof({struct_name}),")
    lines.append("};")
    lines.append("")

    lines.append("size_t synq_node_base_size(uint8_t tag) {")
    lines.append("    if (tag >= SYNQ_NODE_COUNT) return 0;")
    lines.append("    return node_base_sizes[tag];")
    lines.append("}")
    lines.append("")

    # Builder functions
    lines.append("// ============ Builder Functions ============")
    lines.append("")

    for node in node_defs:
        if isinstance(node, NodeDef):
            func_name = _builder_name(node.name)
            struct_name = _struct_name(node.name)
            tag = _tag_name(node.name)

            params = _build_node_params(node, enum_names, flags_names)
            _emit_func_signature(lines, func_name, params, " {")

            # Allocate node
            lines.append(f"    uint32_t id = synq_arena_alloc(&ctx->ast, {tag}, sizeof({struct_name}));")
            lines.append("")

            # Get pointer and set fields
            lines.append(f"    {struct_name} *node = ({struct_name}*)")
            lines.append("        (ctx->ast.data + ctx->ast.offsets[id]);")

            for field_name in node.fields:
                lines.append(f"    node->{field_name} = {field_name};")

            # Auto-compute source range from IndexField children and SourceSpan fields
            index_fields = [
                (fn, ft) for fn, ft in node.fields.items()
                if isinstance(ft, IndexField)
            ]
            span_fields = [
                (fn, ft) for fn, ft in node.fields.items()
                if isinstance(ft, InlineField) and ft.type_name == "SynqSourceSpan"
            ]
            if index_fields or span_fields:
                lines.append("")
                lines.append("    synq_ast_ranges_sync(ctx);")
                lines.append("    SynqSourceRange _r = {UINT32_MAX, 0};")
                for fn, _ft in index_fields:
                    lines.append(f"    synq_ast_range_union(ctx, &_r, {fn});")
                for fn, _ft in span_fields:
                    lines.append(f"    synq_ast_range_union_span(&_r, {fn});")
                lines.append("    if (_r.first != UINT32_MAX) ctx->ranges.data[id] = _r;")

            lines.append("    return id;")
            lines.append("}")
            lines.append("")

        elif isinstance(node, ListDef):
            func_name = _builder_name(node.name)
            struct_name = _struct_name(node.name)
            tag = _tag_name(node.name)

            # Empty list creator (direct arena allocation, no accumulator needed)
            lines.append(f"uint32_t {func_name}_empty(SynqAstContext *ctx) {{")
            lines.append(f"    uint32_t id = synq_arena_alloc(&ctx->ast, {tag}, sizeof({struct_name}));")
            lines.append("")
            lines.append(f"    {struct_name} *node = ({struct_name}*)")
            lines.append("        (ctx->ast.data + ctx->ast.offsets[id]);")
            lines.append("    node->count = 0;")
            lines.append("    synq_ast_ranges_sync(ctx);")
            lines.append("    return id;")
            lines.append("}")
            lines.append("")

            # Single-child creator (via accumulator for O(n) amortized appending)
            lines.append(f"uint32_t {func_name}(SynqAstContext *ctx, uint32_t first_child) {{")
            lines.append(f"    return synq_ast_list_start(ctx, {tag}, first_child);")
            lines.append("}")
            lines.append("")

            # Append function (via accumulator)
            lines.append(f"uint32_t {func_name}_append(SynqAstContext *ctx, uint32_t list_id, uint32_t child) {{")
            lines.append("    if (list_id == SYNQ_NULL_NODE) {")
            lines.append(f"        return {func_name}(ctx, child);")
            lines.append("    }")
            lines.append(f"    return synq_ast_list_append(ctx, list_id, child, {tag});")
            lines.append("}")
            lines.append("")

    output.write_text("\n".join(lines) + "\n")


def generate_ast_print_c(node_defs: list[AnyNodeDef], enum_defs: list[EnumDef],
                         flags_defs: list[FlagsDef], output: Path) -> None:
    """Generate src/ast/ast_print.c with printer implementations."""
    # Build name sets for type lookups
    enum_names = {e.name for e in enum_defs}
    flags_lookup = {f.name: f for f in flags_defs}

    lines = []

    lines.append("// Copyright 2025 The syntaqlite Authors. All rights reserved.")
    lines.append("// Licensed under the Apache License, Version 2.0.")
    lines.append("")
    lines.append("// Generated from data/ast_nodes.py - DO NOT EDIT")
    lines.append("// Regenerate with: python3 python/tools/generate_ast.py")
    lines.append("")
    lines.append('#include "src/parser/ast_nodes_gen.h"')
    lines.append('#include "src/parser/ast_print.h"')
    lines.append("")

    # Forward declaration
    lines.append("static void print_node(FILE *out, SynqArena *ast, uint32_t node_id,")
    lines.append("                       const char *source, int depth,")
    lines.append("                       const char *field_name);")
    lines.append("")

    # Main print_node function
    lines.append("static void print_node(FILE *out, SynqArena *ast, uint32_t node_id,")
    lines.append("                       const char *source, int depth,")
    lines.append("                       const char *field_name) {")
    lines.append("  if (node_id == SYNQ_NULL_NODE) {")
    lines.append("    if (field_name) {")
    lines.append("      synq_ast_print_indent(out, depth);")
    lines.append('      fprintf(out, "%s: null\\n", field_name);')
    lines.append("    }")
    lines.append("    return;")
    lines.append("  }")
    lines.append("")
    lines.append("  SynqNode *node = AST_NODE(ast, node_id);")
    lines.append("  if (!node) {")
    lines.append("    return;")
    lines.append("  }")
    lines.append("")
    lines.append("  switch (node->tag) {")

    for node in node_defs:
        tag = _tag_name(node.name)
        snake_name = pascal_to_snake(node.name)

        lines.append(f"    case {tag}: {{")
        lines.append("      synq_ast_print_indent(out, depth);")

        if isinstance(node, NodeDef):
            # Print node name (with field name prefix if provided)
            lines.append("      if (field_name)")
            lines.append(f'        fprintf(out, "%s: {node.name}\\n", field_name);')
            lines.append("      else")
            lines.append(f'        fprintf(out, "{node.name}\\n");')

            # Print each field
            for field_name, field_type in node.fields.items():
                if isinstance(field_type, IndexField):
                    # Recursively print child node with field name
                    lines.append(f'      print_node(out, ast, node->{snake_name}.{field_name}, source, depth + 1, "{field_name}");')
                elif isinstance(field_type, InlineField):
                    if field_type.type_name == "SynqSourceSpan":
                        # Source span - print quoted text or "null"
                        lines.append("      synq_ast_print_indent(out, depth + 1);")
                        lines.append(f'      fprintf(out, "{field_name}: ");')
                        lines.append(f"      synq_ast_print_source_span(out, source, node->{snake_name}.{field_name});")
                        lines.append('      fprintf(out, "\\n");')
                    elif field_type.type_name in enum_names:
                        # Enum field - print as string
                        names_var = f"synq_{pascal_to_snake(field_type.type_name)}_names"
                        lines.append("      synq_ast_print_indent(out, depth + 1);")
                        lines.append(f'      fprintf(out, "{field_name}: %s\\n", {names_var}[node->{snake_name}.{field_name}]);')
                    elif field_type.type_name in flags_lookup:
                        # Flags union - print individual flag names
                        fdef = flags_lookup[field_type.type_name]
                        accessor = f"node->{snake_name}.{field_name}"
                        lines.append("      synq_ast_print_indent(out, depth + 1);")
                        lines.append(f'      fprintf(out, "{field_name}:");')
                        for fname in fdef.flags:
                            lines.append(f'      if ({accessor}.{fname.lower()}) fprintf(out, " {fname}");')
                        lines.append(f'      if (!{accessor}.raw) fprintf(out, " (none)");')
                        lines.append('      fprintf(out, "\\n");')
                    else:
                        # Regular inline field - print name and value
                        lines.append("      synq_ast_print_indent(out, depth + 1);")
                        lines.append(f'      fprintf(out, "{field_name}: %u\\n", node->{snake_name}.{field_name});')

        elif isinstance(node, ListDef):
            # Print list with count and children (with field name prefix if provided)
            lines.append("      if (field_name)")
            lines.append(f'        fprintf(out, "%s: {node.name}[%u]\\n", field_name, node->{snake_name}.count);')
            lines.append("      else")
            lines.append(f'        fprintf(out, "{node.name}[%u]\\n", node->{snake_name}.count);')
            lines.append(f"      for (uint32_t i = 0; i < node->{snake_name}.count; i++) {{")
            lines.append(f"        print_node(out, ast, node->{snake_name}.children[i], source, depth + 1, NULL);")
            lines.append("      }")

        lines.append("      break;")
        lines.append("    }")
        lines.append("")

    lines.append("    default:")
    lines.append("      synq_ast_print_indent(out, depth);")
    lines.append('      fprintf(out, "Unknown(tag=%d)\\n", node->tag);')
    lines.append("      break;")
    lines.append("  }")
    lines.append("}")
    lines.append("")

    # Public function
    lines.append("void synq_ast_print(FILE *out, SynqArena *ast, uint32_t node_id,")
    lines.append("                          const char *source) {")
    lines.append("  print_node(out, ast, node_id, source, 0, NULL);")
    lines.append("}")

    output.write_text("\n".join(lines) + "\n")


def generate_all(node_defs: list[AnyNodeDef], enum_defs: list[EnumDef], output_dir: Path,
                  flags_defs: list[FlagsDef] | None = None,
                  public_header_dir: Path | None = None) -> None:
    """Generate all AST C code files.

    Args:
        node_defs: List of node definitions.
        enum_defs: List of enum definitions.
        output_dir: Directory to write output files (typically src/ast/).
        flags_defs: List of flags definitions for bitfield unions.
        public_header_dir: Directory for public headers (typically include/syntaqlite/).
            If provided, generates the public ast_nodes_gen.h there.
    """
    if flags_defs is None:
        flags_defs = []
    output_dir.mkdir(parents=True, exist_ok=True)

    # Generate public header if directory is provided
    if public_header_dir is not None:
        public_header_dir.mkdir(parents=True, exist_ok=True)
        generate_public_ast_nodes_h(node_defs, enum_defs, flags_defs,
                                    public_header_dir / "ast_nodes_gen.h")

    generate_ast_nodes_h(node_defs, enum_defs, flags_defs, output_dir / "ast_nodes_gen.h")
    generate_ast_builder_h(node_defs, enum_defs, flags_defs, output_dir / "ast_builder_gen.h")
    generate_ast_builder_c(node_defs, enum_defs, flags_defs, output_dir / "ast_builder_gen.c")
    # ast_print.h is manually maintained
    generate_ast_print_c(node_defs, enum_defs, flags_defs, output_dir / "ast_print_gen.c")
