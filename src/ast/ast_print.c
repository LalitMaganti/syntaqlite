// Generated from data/ast_nodes.py - DO NOT EDIT
// Regenerate with: python3 python/tools/generate_ast.py

#include "src/ast/ast_print.h"

#include <stdio.h>

static void print_indent(FILE *out, int depth) {
  for (int i = 0; i < depth; i++) {
    fprintf(out, "  ");
  }
}

static void print_source_span(FILE *out, const char *source, SyntaqliteSourceSpan span) {
  if (source && span.length > 0) {
    fprintf(out, "\"");
    for (uint16_t i = 0; i < span.length; i++) {
      char c = source[span.offset + i];
      if (c == '"') {
        fprintf(out, "\\\"");
      } else if (c == '\\') {
        fprintf(out, "\\\\");
      } else if (c == '\n') {
        fprintf(out, "\\n");
      } else {
        fprintf(out, "%c", c);
      }
    }
    fprintf(out, "\"");
  } else {
    fprintf(out, "null");
  }
}

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
      print_indent(out, depth);
      fprintf(out, "BinaryExpr\n");
      print_indent(out, depth + 1);
      fprintf(out, "op: %u\n", node->binary_expr.op);
      print_node(out, ast, node->binary_expr.left, source, depth + 1);
      print_node(out, ast, node->binary_expr.right, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_UNARY_EXPR: {
      print_indent(out, depth);
      fprintf(out, "UnaryExpr\n");
      print_indent(out, depth + 1);
      fprintf(out, "op: %u\n", node->unary_expr.op);
      print_node(out, ast, node->unary_expr.operand, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_LITERAL: {
      print_indent(out, depth);
      fprintf(out, "Literal\n");
      print_indent(out, depth + 1);
      fprintf(out, "literal_type: %u\n", node->literal.literal_type);
      print_indent(out, depth + 1);
      fprintf(out, "source: ");
      print_source_span(out, source, node->literal.source);
      fprintf(out, "\n");
      break;
    }

    case SYNTAQLITE_NODE_RESULT_COLUMN: {
      print_indent(out, depth);
      fprintf(out, "ResultColumn\n");
      print_indent(out, depth + 1);
      fprintf(out, "flags: %u\n", node->result_column.flags);
      print_indent(out, depth + 1);
      fprintf(out, "alias: ");
      print_source_span(out, source, node->result_column.alias);
      fprintf(out, "\n");
      print_node(out, ast, node->result_column.expr, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_RESULT_COLUMN_LIST: {
      print_indent(out, depth);
      fprintf(out, "ResultColumnList[%u]\n", node->result_column_list.count);
      for (uint32_t i = 0; i < node->result_column_list.count; i++) {
        print_node(out, ast, node->result_column_list.children[i], source, depth + 1);
      }
      break;
    }

    case SYNTAQLITE_NODE_SELECT_STMT: {
      print_indent(out, depth);
      fprintf(out, "SelectStmt\n");
      print_indent(out, depth + 1);
      fprintf(out, "flags: %u\n", node->select_stmt.flags);
      print_node(out, ast, node->select_stmt.columns, source, depth + 1);
      break;
    }

    default:
      print_indent(out, depth);
      fprintf(out, "Unknown(tag=%d)\n", node->tag);
      break;
  }
}

void syntaqlite_ast_print(FILE *out, SyntaqliteAst *ast, uint32_t node_id,
                          const char *source) {
  print_node(out, ast, node_id, source, 0);
}

void syntaqlite_ast_dump(SyntaqliteAst *ast, uint32_t root_id,
                         const char *source) {
  syntaqlite_ast_print(stdout, ast, root_id, source);
}
