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

from .defs import AnyNodeDef, NodeDef, ListDef, EnumDef, InlineField, IndexField


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


def _c_type(type_name: str) -> str:
    """Convert our type name to C type."""
    return TYPE_MAP.get(type_name, type_name)


def _pascal_to_snake(name: str) -> str:
    """Convert PascalCase to snake_case."""
    result = []
    for i, c in enumerate(name):
        if c.isupper() and i > 0:
            result.append("_")
        result.append(c.lower())
    return "".join(result)


def _struct_name(node_name: str) -> str:
    """Generate C struct name from node name."""
    return f"Syntaqlite{node_name}"


def _tag_name(node_name: str) -> str:
    """Generate enum tag name from node name."""
    return f"SYNTAQLITE_NODE_{_pascal_to_snake(node_name).upper()}"


def _builder_name(node_name: str) -> str:
    """Generate builder function name from node name."""
    return f"ast_{_pascal_to_snake(node_name)}"


def _calculate_struct_size(node: NodeDef) -> tuple[int, list[tuple[str, str, int, str]]]:
    """Calculate struct size and field layout.

    Returns:
        Tuple of (total_size, fields) where fields is a list of
        (field_name, c_type, size, storage_type).
    """
    # Start with tag byte
    fields = [("tag", "uint8_t", 1, "tag")]
    offset = 1

    for field_name, field_type in node.fields.items():
        if isinstance(field_type, InlineField):
            c_type = _c_type(field_type.type_name)
            if field_type.type_name == "u8":
                size = 1
            elif field_type.type_name == "u16":
                size = 2
            elif field_type.type_name in ("u32", "i32"):
                size = 4
            elif field_type.type_name in ("u64", "i64"):
                size = 8
            else:
                size = 4  # Default for custom types
            storage = "inline"
        else:  # IndexField
            c_type = "uint32_t"
            size = 4
            storage = "index"

        # Add padding for alignment
        alignment = min(size, 4)  # Max alignment is 4 bytes for our types
        if offset % alignment != 0:
            padding = alignment - (offset % alignment)
            offset += padding

        fields.append((field_name, c_type, size, storage))
        offset += size

    return offset, fields


def _enum_prefix(enum_name: str) -> str:
    """Generate enum value prefix from enum name."""
    return f"SYNTAQLITE_{_pascal_to_snake(enum_name).upper()}"


def generate_ast_nodes_h(node_defs: list[AnyNodeDef], enum_defs: list[EnumDef], output: Path) -> None:
    """Generate src/ast/ast_nodes.h with structs, union, arena types."""
    lines = []

    # Header
    lines.append("// Generated from data/ast_nodes.py - DO NOT EDIT")
    lines.append("// Regenerate with: python3 python/tools/generate_ast.py")
    lines.append("")
    lines.append("#ifndef SYNTAQLITE_AST_NODES_H")
    lines.append("#define SYNTAQLITE_AST_NODES_H")
    lines.append("")
    lines.append("#include <stddef.h>")
    lines.append("#include <stdint.h>")
    lines.append("")
    lines.append("#ifdef __cplusplus")
    lines.append('extern "C" {')
    lines.append("#endif")
    lines.append("")

    # Null node constant
    lines.append("// Null node ID (used for nullable fields)")
    lines.append("#define SYNTAQLITE_NULL_NODE 0xFFFFFFFFu")
    lines.append("")

    # Helper structs
    lines.append("// ============ Helper Types ============")
    lines.append("")
    lines.append("// Source location span (offset + length into source text)")
    lines.append("typedef struct SyntaqliteSourceSpan {")
    lines.append("    uint32_t offset;")
    lines.append("    uint16_t length;")
    lines.append("} SyntaqliteSourceSpan;")
    lines.append("")
    lines.append("// Create a source span from offset and length")
    lines.append("static inline SyntaqliteSourceSpan syntaqlite_span(uint32_t offset, uint16_t length) {")
    lines.append("    SyntaqliteSourceSpan span = {offset, length};")
    lines.append("    return span;")
    lines.append("}")
    lines.append("")
    lines.append("// Empty source span (no location)")
    lines.append("#define SYNTAQLITE_NO_SPAN ((SyntaqliteSourceSpan){0, 0})")
    lines.append("")

    # User-defined enums
    if enum_defs:
        lines.append("// ============ Value Enums ============")
        lines.append("")
        for enum in enum_defs:
            prefix = _enum_prefix(enum.name)
            lines.append(f"typedef enum {{")
            for i, value in enumerate(enum.values):
                lines.append(f"    {prefix}_{value} = {i},")
            lines.append(f"}} Syntaqlite{enum.name};")
            lines.append("")

    # Node tag enum
    lines.append("// ============ Node Tags ============")
    lines.append("")
    lines.append("typedef enum {")
    lines.append("    SYNTAQLITE_NODE_NULL = 0,")
    for node in node_defs:
        lines.append(f"    {_tag_name(node.name)},")
    lines.append("    SYNTAQLITE_NODE_COUNT")
    lines.append("} SyntaqliteNodeTag;")
    lines.append("")

    # Node structs
    lines.append("// ============ Node Structs (variable sizes) ============")
    lines.append("")

    for node in node_defs:
        if isinstance(node, NodeDef):
            struct_name = _struct_name(node.name)
            lines.append(f"typedef struct {struct_name} {{")
            lines.append("    uint8_t tag;")

            # Track offset for padding comments
            offset = 1
            for field_name, field_type in node.fields.items():
                if isinstance(field_type, InlineField):
                    c_type = _c_type(field_type.type_name)
                    comment = "inline"
                else:
                    c_type = "uint32_t"
                    comment = f"index -> {field_type.type_name}"

                lines.append(f"    {c_type} {field_name};  // {comment}")

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

    # Union for ergonomic access
    lines.append("// ============ Union for Ergonomic Access ============")
    lines.append("// Note: This is for type-punning convenience, NOT for storage sizing")
    lines.append("")
    lines.append("typedef union SyntaqliteNode {")
    lines.append("    uint8_t tag;")
    for node in node_defs:
        struct_name = _struct_name(node.name)
        field_name = _pascal_to_snake(node.name)
        lines.append(f"    {struct_name} {field_name};")
    lines.append("} SyntaqliteNode;")
    lines.append("")

    # Arena storage - uses named struct to match forward declaration
    lines.append("// ============ Arena Storage ============")
    lines.append("")
    lines.append("typedef struct SyntaqliteAst {")
    lines.append("    uint8_t *arena;           // Packed nodes of varying sizes")
    lines.append("    uint32_t arena_size;")
    lines.append("    uint32_t arena_capacity;")
    lines.append("    uint32_t *offsets;        // offsets[node_id] = byte position in arena")
    lines.append("    uint32_t node_count;")
    lines.append("    uint32_t node_capacity;")
    lines.append("} SyntaqliteAst;")
    lines.append("")

    # Node access macro
    lines.append("// Access node by ID")
    lines.append("static inline SyntaqliteNode* syntaqlite_ast_node(SyntaqliteAst *ast, uint32_t id) {")
    lines.append("    if (id == SYNTAQLITE_NULL_NODE) return NULL;")
    lines.append("    return (SyntaqliteNode*)(ast->arena + ast->offsets[id]);")
    lines.append("}")
    lines.append("")
    lines.append("#define AST_NODE(ast, id) syntaqlite_ast_node(ast, id)")
    lines.append("")

    # Node size table declaration
    lines.append("// ============ Node Size Table ============")
    lines.append("")
    lines.append("// Returns the fixed size of a node type (0 for variable-size nodes like lists)")
    lines.append("size_t syntaqlite_node_base_size(uint8_t tag);")
    lines.append("")

    lines.append("#ifdef __cplusplus")
    lines.append("}")
    lines.append("#endif")
    lines.append("")
    lines.append("#endif  // SYNTAQLITE_AST_NODES_H")

    output.write_text("\n".join(lines) + "\n")


def generate_ast_builder_h(node_defs: list[AnyNodeDef], output: Path) -> None:
    """Generate src/ast/ast_builder.h with builder function declarations."""
    lines = []

    # Header
    lines.append("// Generated from data/ast_nodes.py - DO NOT EDIT")
    lines.append("// Regenerate with: python3 python/tools/generate_ast.py")
    lines.append("")
    lines.append("#ifndef SYNTAQLITE_AST_BUILDER_H")
    lines.append("#define SYNTAQLITE_AST_BUILDER_H")
    lines.append("")
    lines.append('#include "src/ast/ast_nodes.h"')
    lines.append("")
    lines.append("#ifdef __cplusplus")
    lines.append('extern "C" {')
    lines.append("#endif")
    lines.append("")

    # Build context - uses named struct to match forward declaration in syntaqlite_sqlite_defs.h
    lines.append("// Build context passed through parser")
    lines.append("typedef struct SyntaqliteAstContext {")
    lines.append("    SyntaqliteAst *ast;")
    lines.append("    const char *source;       // Source text for offset references")
    lines.append("    uint32_t source_length;   // Length of source text")
    lines.append("    int error_code;           // Error code if any")
    lines.append("    const char *error_msg;    // Error message if any")
    lines.append("} SyntaqliteAstContext;")
    lines.append("")

    # Initialize/free AST
    lines.append("// Initialize/free AST")
    lines.append("void syntaqlite_ast_init(SyntaqliteAst *ast);")
    lines.append("void syntaqlite_ast_free(SyntaqliteAst *ast);")
    lines.append("")

    # Generic allocator
    lines.append("// ============ Generic Allocator ============")
    lines.append("")
    lines.append("// Allocate node of given size, return node ID")
    lines.append("uint32_t ast_alloc(SyntaqliteAstContext *ctx, uint8_t tag, size_t size);")
    lines.append("")

    # Builder function declarations
    lines.append("// ============ Builder Functions ============")
    lines.append("")

    for node in node_defs:
        if isinstance(node, NodeDef):
            func_name = _builder_name(node.name)

            # Build parameter list
            params = ["SyntaqliteAstContext *ctx"]
            for field_name, field_type in node.fields.items():
                if isinstance(field_type, InlineField):
                    c_type = _c_type(field_type.type_name)
                else:
                    c_type = "uint32_t"
                params.append(f"{c_type} {field_name}")

            params_str = ", ".join(params)

            # If too long, split across lines
            if len(params_str) > 80:
                lines.append(f"uint32_t {func_name}(")
                for i, param in enumerate(params):
                    comma = "," if i < len(params) - 1 else ""
                    lines.append(f"    {param}{comma}")
                lines.append(");")
            else:
                lines.append(f"uint32_t {func_name}({params_str});")
            lines.append("")

        elif isinstance(node, ListDef):
            func_name = _builder_name(node.name)
            lines.append(f"// Create empty {node.name}")
            lines.append(f"uint32_t {func_name}_empty(SyntaqliteAstContext *ctx);")
            lines.append("")
            lines.append(f"// Create {node.name} with single child")
            lines.append(f"uint32_t {func_name}(SyntaqliteAstContext *ctx, uint32_t first_child);")
            lines.append("")
            lines.append(f"// Append child to {node.name} (may reallocate, returns new list ID)")
            lines.append(f"uint32_t {func_name}_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child);")
            lines.append("")

    lines.append("#ifdef __cplusplus")
    lines.append("}")
    lines.append("#endif")
    lines.append("")
    lines.append("#endif  // SYNTAQLITE_AST_BUILDER_H")

    output.write_text("\n".join(lines) + "\n")


def generate_ast_builder_c(node_defs: list[AnyNodeDef], output: Path) -> None:
    """Generate src/ast/ast_builder.c with builder implementations."""
    lines = []

    # Header
    lines.append("// Generated from data/ast_nodes.py - DO NOT EDIT")
    lines.append("// Regenerate with: python3 python/tools/generate_ast.py")
    lines.append("")
    lines.append('#include "src/ast/ast_builder.h"')
    lines.append("")
    lines.append("#include <stdlib.h>")
    lines.append("#include <string.h>")
    lines.append("")

    # Node size table - collect fixed-size nodes
    lines.append("// ============ Node Size Table ============")
    lines.append("")
    lines.append("static const size_t node_base_sizes[] = {")
    lines.append("    [SYNTAQLITE_NODE_NULL] = 0,")
    for node in node_defs:
        tag = _tag_name(node.name)
        struct_name = _struct_name(node.name)
        if isinstance(node, ListDef):
            # Lists have variable size, base size is header only
            lines.append(f"    [{tag}] = sizeof({struct_name}),")
        else:
            lines.append(f"    [{tag}] = sizeof({struct_name}),")
    lines.append("};")
    lines.append("")

    lines.append("size_t syntaqlite_node_base_size(uint8_t tag) {")
    lines.append("    if (tag >= SYNTAQLITE_NODE_COUNT) return 0;")
    lines.append("    return node_base_sizes[tag];")
    lines.append("}")
    lines.append("")

    # Initialize/free
    lines.append("// ============ Initialize/Free ============")
    lines.append("")
    lines.append("void syntaqlite_ast_init(SyntaqliteAst *ast) {")
    lines.append("    ast->arena = NULL;")
    lines.append("    ast->arena_size = 0;")
    lines.append("    ast->arena_capacity = 0;")
    lines.append("    ast->offsets = NULL;")
    lines.append("    ast->node_count = 0;")
    lines.append("    ast->node_capacity = 0;")
    lines.append("}")
    lines.append("")
    lines.append("void syntaqlite_ast_free(SyntaqliteAst *ast) {")
    lines.append("    free(ast->arena);")
    lines.append("    free(ast->offsets);")
    lines.append("    ast->arena = NULL;")
    lines.append("    ast->offsets = NULL;")
    lines.append("    ast->arena_size = 0;")
    lines.append("    ast->arena_capacity = 0;")
    lines.append("    ast->node_count = 0;")
    lines.append("    ast->node_capacity = 0;")
    lines.append("}")
    lines.append("")

    # Generic allocator
    lines.append("// ============ Generic Allocator ============")
    lines.append("")
    lines.append("uint32_t ast_alloc(SyntaqliteAstContext *ctx, uint8_t tag, size_t size) {")
    lines.append("    SyntaqliteAst *ast = ctx->ast;")
    lines.append("")
    lines.append("    // Grow arena if needed")
    lines.append("    if (ast->arena_size + size > ast->arena_capacity) {")
    lines.append("        size_t new_capacity = ast->arena_capacity * 2 + size + 1024;")
    lines.append("        uint8_t *new_arena = (uint8_t*)realloc(ast->arena, new_capacity);")
    lines.append("        if (!new_arena) {")
    lines.append("            ctx->error_code = 1;")
    lines.append('            ctx->error_msg = "Out of memory (arena)";')
    lines.append("            return SYNTAQLITE_NULL_NODE;")
    lines.append("        }")
    lines.append("        ast->arena = new_arena;")
    lines.append("        ast->arena_capacity = (uint32_t)new_capacity;")
    lines.append("    }")
    lines.append("")
    lines.append("    // Grow offset table if needed")
    lines.append("    if (ast->node_count >= ast->node_capacity) {")
    lines.append("        size_t new_capacity = ast->node_capacity * 2 + 64;")
    lines.append("        uint32_t *new_offsets = (uint32_t*)realloc(ast->offsets, new_capacity * sizeof(uint32_t));")
    lines.append("        if (!new_offsets) {")
    lines.append("            ctx->error_code = 1;")
    lines.append('            ctx->error_msg = "Out of memory (offsets)";')
    lines.append("            return SYNTAQLITE_NULL_NODE;")
    lines.append("        }")
    lines.append("        ast->offsets = new_offsets;")
    lines.append("        ast->node_capacity = (uint32_t)new_capacity;")
    lines.append("    }")
    lines.append("")
    lines.append("    uint32_t node_id = ast->node_count++;")
    lines.append("    ast->offsets[node_id] = ast->arena_size;")
    lines.append("    ast->arena[ast->arena_size] = tag;  // Set tag byte")
    lines.append("    ast->arena_size += (uint32_t)size;")
    lines.append("")
    lines.append("    return node_id;")
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

            # Build parameter list
            params = ["SyntaqliteAstContext *ctx"]
            for field_name, field_type in node.fields.items():
                if isinstance(field_type, InlineField):
                    c_type = _c_type(field_type.type_name)
                else:
                    c_type = "uint32_t"
                params.append(f"{c_type} {field_name}")

            params_str = ", ".join(params)

            # Function signature
            if len(params_str) > 80:
                lines.append(f"uint32_t {func_name}(")
                for i, param in enumerate(params):
                    comma = "," if i < len(params) - 1 else ""
                    lines.append(f"    {param}{comma}")
                lines.append(") {")
            else:
                lines.append(f"uint32_t {func_name}({params_str}) {{")

            # Allocate node
            lines.append(f"    uint32_t id = ast_alloc(ctx, {tag}, sizeof({struct_name}));")
            lines.append("    if (id == SYNTAQLITE_NULL_NODE) return id;")
            lines.append("")

            # Get pointer and set fields
            lines.append(f"    {struct_name} *node = ({struct_name}*)")
            lines.append("        (ctx->ast->arena + ctx->ast->offsets[id]);")

            for field_name in node.fields:
                lines.append(f"    node->{field_name} = {field_name};")

            lines.append("    return id;")
            lines.append("}")
            lines.append("")

        elif isinstance(node, ListDef):
            func_name = _builder_name(node.name)
            struct_name = _struct_name(node.name)
            tag = _tag_name(node.name)

            # Empty list creator
            lines.append(f"uint32_t {func_name}_empty(SyntaqliteAstContext *ctx) {{")
            lines.append(f"    uint32_t id = ast_alloc(ctx, {tag}, sizeof({struct_name}));")
            lines.append("    if (id == SYNTAQLITE_NULL_NODE) return id;")
            lines.append("")
            lines.append(f"    {struct_name} *node = ({struct_name}*)")
            lines.append("        (ctx->ast->arena + ctx->ast->offsets[id]);")
            lines.append("    node->count = 0;")
            lines.append("    return id;")
            lines.append("}")
            lines.append("")

            # Single-child creator
            lines.append(f"uint32_t {func_name}(SyntaqliteAstContext *ctx, uint32_t first_child) {{")
            lines.append(f"    uint32_t id = ast_alloc(ctx, {tag}, sizeof({struct_name}) + sizeof(uint32_t));")
            lines.append("    if (id == SYNTAQLITE_NULL_NODE) return id;")
            lines.append("")
            lines.append(f"    {struct_name} *node = ({struct_name}*)")
            lines.append("        (ctx->ast->arena + ctx->ast->offsets[id]);")
            lines.append("    node->count = 1;")
            lines.append("    node->children[0] = first_child;")
            lines.append("    return id;")
            lines.append("}")
            lines.append("")

            # Append function
            lines.append(f"uint32_t {func_name}_append(SyntaqliteAstContext *ctx, uint32_t list_id, uint32_t child) {{")
            lines.append("    if (list_id == SYNTAQLITE_NULL_NODE) {")
            lines.append(f"        return {func_name}(ctx, child);")
            lines.append("    }")
            lines.append("")
            lines.append("    // Get current list")
            lines.append(f"    {struct_name} *old_node = ({struct_name}*)")
            lines.append("        (ctx->ast->arena + ctx->ast->offsets[list_id]);")
            lines.append("    uint32_t old_count = old_node->count;")
            lines.append("")
            lines.append("    // Allocate new list with space for one more child")
            lines.append(f"    size_t new_size = sizeof({struct_name}) + (old_count + 1) * sizeof(uint32_t);")
            lines.append(f"    uint32_t new_id = ast_alloc(ctx, {tag}, new_size);")
            lines.append("    if (new_id == SYNTAQLITE_NULL_NODE) return new_id;")
            lines.append("")
            lines.append("    // Re-fetch old node pointer (may have moved due to realloc)")
            lines.append(f"    old_node = ({struct_name}*)")
            lines.append("        (ctx->ast->arena + ctx->ast->offsets[list_id]);")
            lines.append("")
            lines.append(f"    {struct_name} *new_node = ({struct_name}*)")
            lines.append("        (ctx->ast->arena + ctx->ast->offsets[new_id]);")
            lines.append("")
            lines.append("    // Copy old children and add new one")
            lines.append("    new_node->count = old_count + 1;")
            lines.append("    memcpy(new_node->children, old_node->children, old_count * sizeof(uint32_t));")
            lines.append("    new_node->children[old_count] = child;")
            lines.append("")
            lines.append("    return new_id;")
            lines.append("}")
            lines.append("")

    output.write_text("\n".join(lines) + "\n")


def generate_ast_print_h(node_defs: list[AnyNodeDef], output: Path) -> None:
    """Generate src/ast/ast_print.h with printer declarations."""
    lines = []

    lines.append("// Generated from data/ast_nodes.py - DO NOT EDIT")
    lines.append("// Regenerate with: python3 python/tools/generate_ast.py")
    lines.append("")
    lines.append("#ifndef SYNTAQLITE_AST_PRINT_H")
    lines.append("#define SYNTAQLITE_AST_PRINT_H")
    lines.append("")
    lines.append('#include "src/ast/ast_nodes.h"')
    lines.append("")
    lines.append("#include <stdio.h>")
    lines.append("")
    lines.append("#ifdef __cplusplus")
    lines.append('extern "C" {')
    lines.append("#endif")
    lines.append("")
    lines.append("// Print AST node and its children to a file stream.")
    lines.append("// Uses a simple indented text format suitable for diff tests.")
    lines.append("void syntaqlite_ast_print(FILE *out, SyntaqliteAst *ast, uint32_t node_id,")
    lines.append("                          const char *source);")
    lines.append("")
    lines.append("// Print AST to stdout")
    lines.append("void syntaqlite_ast_dump(SyntaqliteAst *ast, uint32_t root_id,")
    lines.append("                         const char *source);")
    lines.append("")
    lines.append("#ifdef __cplusplus")
    lines.append("}")
    lines.append("#endif")
    lines.append("")
    lines.append("#endif  // SYNTAQLITE_AST_PRINT_H")

    output.write_text("\n".join(lines) + "\n")


def generate_ast_print_c(node_defs: list[AnyNodeDef], output: Path) -> None:
    """Generate src/ast/ast_print.c with printer implementations."""
    lines = []

    lines.append("// Generated from data/ast_nodes.py - DO NOT EDIT")
    lines.append("// Regenerate with: python3 python/tools/generate_ast.py")
    lines.append("")
    lines.append('#include "src/ast/ast_print.h"')
    lines.append("")
    lines.append("#include <stdio.h>")
    lines.append("")

    # Helper functions
    lines.append("static void print_indent(FILE *out, int depth) {")
    lines.append("  for (int i = 0; i < depth; i++) {")
    lines.append('    fprintf(out, "  ");')
    lines.append("  }")
    lines.append("}")
    lines.append("")

    lines.append("static void print_source_span(FILE *out, const char *source, SyntaqliteSourceSpan span) {")
    lines.append("  if (source && span.length > 0) {")
    lines.append('    fprintf(out, "\\"");')
    lines.append("    for (uint16_t i = 0; i < span.length; i++) {")
    lines.append("      char c = source[span.offset + i];")
    lines.append("      if (c == '\"') {")
    lines.append('        fprintf(out, "\\\\\\"");')
    lines.append("      } else if (c == '\\\\') {")
    lines.append('        fprintf(out, "\\\\\\\\");')
    lines.append("      } else if (c == '\\n') {")
    lines.append('        fprintf(out, "\\\\n");')
    lines.append("      } else {")
    lines.append('        fprintf(out, "%c", c);')
    lines.append("      }")
    lines.append("    }")
    lines.append('    fprintf(out, "\\"");')
    lines.append("  } else {")
    lines.append('    fprintf(out, "null");')
    lines.append("  }")
    lines.append("}")
    lines.append("")

    # Forward declaration
    lines.append("static void print_node(FILE *out, SyntaqliteAst *ast, uint32_t node_id,")
    lines.append("                       const char *source, int depth);")
    lines.append("")

    # Main print_node function
    lines.append("static void print_node(FILE *out, SyntaqliteAst *ast, uint32_t node_id,")
    lines.append("                       const char *source, int depth) {")
    lines.append("  if (node_id == SYNTAQLITE_NULL_NODE) {")
    lines.append("    return;")
    lines.append("  }")
    lines.append("")
    lines.append("  SyntaqliteNode *node = AST_NODE(ast, node_id);")
    lines.append("  if (!node) {")
    lines.append("    return;")
    lines.append("  }")
    lines.append("")
    lines.append("  switch (node->tag) {")

    for node in node_defs:
        tag = _tag_name(node.name)
        snake_name = _pascal_to_snake(node.name)

        lines.append(f"    case {tag}: {{")
        lines.append("      print_indent(out, depth);")

        if isinstance(node, NodeDef):
            # Print node name
            lines.append(f'      fprintf(out, "{node.name}\\n");')

            # Print each field
            for field_name, field_type in node.fields.items():
                if isinstance(field_type, IndexField):
                    # Recursively print child node
                    lines.append(f"      print_node(out, ast, node->{snake_name}.{field_name}, source, depth + 1);")
                elif isinstance(field_type, InlineField):
                    if field_type.type_name == "SyntaqliteSourceSpan":
                        # Source span - print quoted text or "null"
                        lines.append("      print_indent(out, depth + 1);")
                        lines.append(f'      fprintf(out, "{field_name}: ");')
                        lines.append(f"      print_source_span(out, source, node->{snake_name}.{field_name});")
                        lines.append('      fprintf(out, "\\n");')
                    else:
                        # Regular inline field - print name and value
                        lines.append("      print_indent(out, depth + 1);")
                        if field_type.type_name == "u8":
                            lines.append(f'      fprintf(out, "{field_name}: %u\\n", node->{snake_name}.{field_name});')
                        elif field_type.type_name == "u16":
                            lines.append(f'      fprintf(out, "{field_name}: %u\\n", node->{snake_name}.{field_name});')
                        elif field_type.type_name == "u32":
                            lines.append(f'      fprintf(out, "{field_name}: %u\\n", node->{snake_name}.{field_name});')
                        else:
                            lines.append(f'      fprintf(out, "{field_name}: %d\\n", (int)node->{snake_name}.{field_name});')

        elif isinstance(node, ListDef):
            # Print list with count and children
            lines.append(f'      fprintf(out, "{node.name}[%u]\\n", node->{snake_name}.count);')
            lines.append(f"      for (uint32_t i = 0; i < node->{snake_name}.count; i++) {{")
            lines.append(f"        print_node(out, ast, node->{snake_name}.children[i], source, depth + 1);")
            lines.append("      }")

        lines.append("      break;")
        lines.append("    }")
        lines.append("")

    lines.append("    default:")
    lines.append("      print_indent(out, depth);")
    lines.append('      fprintf(out, "Unknown(tag=%d)\\n", node->tag);')
    lines.append("      break;")
    lines.append("  }")
    lines.append("}")
    lines.append("")

    # Public functions
    lines.append("void syntaqlite_ast_print(FILE *out, SyntaqliteAst *ast, uint32_t node_id,")
    lines.append("                          const char *source) {")
    lines.append("  print_node(out, ast, node_id, source, 0);")
    lines.append("}")
    lines.append("")

    lines.append("void syntaqlite_ast_dump(SyntaqliteAst *ast, uint32_t root_id,")
    lines.append("                         const char *source) {")
    lines.append("  syntaqlite_ast_print(stdout, ast, root_id, source);")
    lines.append("}")

    output.write_text("\n".join(lines) + "\n")


def generate_all(node_defs: list[AnyNodeDef], enum_defs: list[EnumDef], output_dir: Path) -> None:
    """Generate all AST C code files.

    Args:
        node_defs: List of node definitions.
        enum_defs: List of enum definitions.
        output_dir: Directory to write output files (typically src/ast/).
    """
    output_dir.mkdir(parents=True, exist_ok=True)

    generate_ast_nodes_h(node_defs, enum_defs, output_dir / "ast_nodes.h")
    generate_ast_builder_h(node_defs, output_dir / "ast_builder.h")
    generate_ast_builder_c(node_defs, output_dir / "ast_builder.c")
    generate_ast_print_h(node_defs, output_dir / "ast_print.h")
    generate_ast_print_c(node_defs, output_dir / "ast_print.c")
