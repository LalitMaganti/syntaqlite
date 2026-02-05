// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Generated from data/ast_nodes.py - DO NOT EDIT
// Regenerate with: python3 python/tools/generate_ast.py

#include "src/ast/ast_nodes.h"
#include "src/ast/ast_print.h"

static void print_node(FILE *out, SyntaqliteAst *ast, uint32_t node_id,
                       const char *source, int depth);

static void print_node(FILE *out, SyntaqliteAst *ast, uint32_t node_id,
                       const char *source, int depth) {
  if (node_id == SYNTAQLITE_NULL_NODE) {
    return;
  }

  SyntaqliteNode *node = AST_NODE(ast, node_id);
  if (!node) {
    return;
  }

  switch (node->tag) {
    case SYNTAQLITE_NODE_BINARY_EXPR: {
      ast_print_indent(out, depth);
      fprintf(out, "BinaryExpr\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "op: %s\n", syntaqlite_binary_op_names[node->binary_expr.op]);
      print_node(out, ast, node->binary_expr.left, source, depth + 1);
      print_node(out, ast, node->binary_expr.right, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_UNARY_EXPR: {
      ast_print_indent(out, depth);
      fprintf(out, "UnaryExpr\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "op: %s\n", syntaqlite_unary_op_names[node->unary_expr.op]);
      print_node(out, ast, node->unary_expr.operand, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_LITERAL: {
      ast_print_indent(out, depth);
      fprintf(out, "Literal\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "literal_type: %s\n", syntaqlite_literal_type_names[node->literal.literal_type]);
      ast_print_indent(out, depth + 1);
      fprintf(out, "source: ");
      ast_print_source_span(out, source, node->literal.source);
      fprintf(out, "\n");
      break;
    }

    case SYNTAQLITE_NODE_RESULT_COLUMN: {
      ast_print_indent(out, depth);
      fprintf(out, "ResultColumn\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "flags: %u\n", node->result_column.flags);
      ast_print_indent(out, depth + 1);
      fprintf(out, "alias: ");
      ast_print_source_span(out, source, node->result_column.alias);
      fprintf(out, "\n");
      print_node(out, ast, node->result_column.expr, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_RESULT_COLUMN_LIST: {
      ast_print_indent(out, depth);
      fprintf(out, "ResultColumnList[%u]\n", node->result_column_list.count);
      for (uint32_t i = 0; i < node->result_column_list.count; i++) {
        print_node(out, ast, node->result_column_list.children[i], source, depth + 1);
      }
      break;
    }

    case SYNTAQLITE_NODE_SELECT_STMT: {
      ast_print_indent(out, depth);
      fprintf(out, "SelectStmt\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "flags: %u\n", node->select_stmt.flags);
      print_node(out, ast, node->select_stmt.columns, source, depth + 1);
      break;
    }

    default:
      ast_print_indent(out, depth);
      fprintf(out, "Unknown(tag=%d)\n", node->tag);
      break;
  }
}

void syntaqlite_ast_print(FILE *out, SyntaqliteAst *ast, uint32_t node_id,
                          const char *source) {
  print_node(out, ast, node_id, source, 0);
}
