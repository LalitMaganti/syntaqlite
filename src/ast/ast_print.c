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

    case SYNTAQLITE_NODE_EXPR_LIST: {
      ast_print_indent(out, depth);
      fprintf(out, "ExprList[%u]\n", node->expr_list.count);
      for (uint32_t i = 0; i < node->expr_list.count; i++) {
        print_node(out, ast, node->expr_list.children[i], source, depth + 1);
      }
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
      print_node(out, ast, node->select_stmt.where, source, depth + 1);
      print_node(out, ast, node->select_stmt.groupby, source, depth + 1);
      print_node(out, ast, node->select_stmt.having, source, depth + 1);
      print_node(out, ast, node->select_stmt.orderby, source, depth + 1);
      print_node(out, ast, node->select_stmt.limit_clause, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_ORDERING_TERM: {
      ast_print_indent(out, depth);
      fprintf(out, "OrderingTerm\n");
      print_node(out, ast, node->ordering_term.expr, source, depth + 1);
      ast_print_indent(out, depth + 1);
      fprintf(out, "sort_order: %s\n", syntaqlite_sort_order_names[node->ordering_term.sort_order]);
      ast_print_indent(out, depth + 1);
      fprintf(out, "nulls_order: %s\n", syntaqlite_nulls_order_names[node->ordering_term.nulls_order]);
      break;
    }

    case SYNTAQLITE_NODE_ORDER_BY_LIST: {
      ast_print_indent(out, depth);
      fprintf(out, "OrderByList[%u]\n", node->order_by_list.count);
      for (uint32_t i = 0; i < node->order_by_list.count; i++) {
        print_node(out, ast, node->order_by_list.children[i], source, depth + 1);
      }
      break;
    }

    case SYNTAQLITE_NODE_LIMIT_CLAUSE: {
      ast_print_indent(out, depth);
      fprintf(out, "LimitClause\n");
      print_node(out, ast, node->limit_clause.limit, source, depth + 1);
      print_node(out, ast, node->limit_clause.offset, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_COLUMN_REF: {
      ast_print_indent(out, depth);
      fprintf(out, "ColumnRef\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "column: ");
      ast_print_source_span(out, source, node->column_ref.column);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "table: ");
      ast_print_source_span(out, source, node->column_ref.table);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "schema: ");
      ast_print_source_span(out, source, node->column_ref.schema);
      fprintf(out, "\n");
      break;
    }

    case SYNTAQLITE_NODE_FUNCTION_CALL: {
      ast_print_indent(out, depth);
      fprintf(out, "FunctionCall\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "func_name: ");
      ast_print_source_span(out, source, node->function_call.func_name);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "flags: %u\n", node->function_call.flags);
      print_node(out, ast, node->function_call.args, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_IS_EXPR: {
      ast_print_indent(out, depth);
      fprintf(out, "IsExpr\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "op: %s\n", syntaqlite_is_op_names[node->is_expr.op]);
      print_node(out, ast, node->is_expr.left, source, depth + 1);
      print_node(out, ast, node->is_expr.right, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_BETWEEN_EXPR: {
      ast_print_indent(out, depth);
      fprintf(out, "BetweenExpr\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "negated: %u\n", node->between_expr.negated);
      print_node(out, ast, node->between_expr.operand, source, depth + 1);
      print_node(out, ast, node->between_expr.low, source, depth + 1);
      print_node(out, ast, node->between_expr.high, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_LIKE_EXPR: {
      ast_print_indent(out, depth);
      fprintf(out, "LikeExpr\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "negated: %u\n", node->like_expr.negated);
      print_node(out, ast, node->like_expr.operand, source, depth + 1);
      print_node(out, ast, node->like_expr.pattern, source, depth + 1);
      print_node(out, ast, node->like_expr.escape, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_CASE_EXPR: {
      ast_print_indent(out, depth);
      fprintf(out, "CaseExpr\n");
      print_node(out, ast, node->case_expr.operand, source, depth + 1);
      print_node(out, ast, node->case_expr.else_expr, source, depth + 1);
      print_node(out, ast, node->case_expr.whens, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_CASE_WHEN: {
      ast_print_indent(out, depth);
      fprintf(out, "CaseWhen\n");
      print_node(out, ast, node->case_when.when_expr, source, depth + 1);
      print_node(out, ast, node->case_when.then_expr, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_CASE_WHEN_LIST: {
      ast_print_indent(out, depth);
      fprintf(out, "CaseWhenList[%u]\n", node->case_when_list.count);
      for (uint32_t i = 0; i < node->case_when_list.count; i++) {
        print_node(out, ast, node->case_when_list.children[i], source, depth + 1);
      }
      break;
    }

    case SYNTAQLITE_NODE_COMPOUND_SELECT: {
      ast_print_indent(out, depth);
      fprintf(out, "CompoundSelect\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "op: %s\n", syntaqlite_compound_op_names[node->compound_select.op]);
      print_node(out, ast, node->compound_select.left, source, depth + 1);
      print_node(out, ast, node->compound_select.right, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_SUBQUERY_EXPR: {
      ast_print_indent(out, depth);
      fprintf(out, "SubqueryExpr\n");
      print_node(out, ast, node->subquery_expr.select, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_EXISTS_EXPR: {
      ast_print_indent(out, depth);
      fprintf(out, "ExistsExpr\n");
      print_node(out, ast, node->exists_expr.select, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_IN_EXPR: {
      ast_print_indent(out, depth);
      fprintf(out, "InExpr\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "negated: %u\n", node->in_expr.negated);
      print_node(out, ast, node->in_expr.operand, source, depth + 1);
      print_node(out, ast, node->in_expr.source, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_VARIABLE: {
      ast_print_indent(out, depth);
      fprintf(out, "Variable\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "source: ");
      ast_print_source_span(out, source, node->variable.source);
      fprintf(out, "\n");
      break;
    }

    case SYNTAQLITE_NODE_COLLATE_EXPR: {
      ast_print_indent(out, depth);
      fprintf(out, "CollateExpr\n");
      print_node(out, ast, node->collate_expr.expr, source, depth + 1);
      ast_print_indent(out, depth + 1);
      fprintf(out, "collation: ");
      ast_print_source_span(out, source, node->collate_expr.collation);
      fprintf(out, "\n");
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
