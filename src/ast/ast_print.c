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
      print_node(out, ast, node->select_stmt.from_clause, source, depth + 1);
      print_node(out, ast, node->select_stmt.where, source, depth + 1);
      print_node(out, ast, node->select_stmt.groupby, source, depth + 1);
      print_node(out, ast, node->select_stmt.having, source, depth + 1);
      print_node(out, ast, node->select_stmt.orderby, source, depth + 1);
      print_node(out, ast, node->select_stmt.limit_clause, source, depth + 1);
      print_node(out, ast, node->select_stmt.window_clause, source, depth + 1);
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
      print_node(out, ast, node->function_call.filter_clause, source, depth + 1);
      print_node(out, ast, node->function_call.over_clause, source, depth + 1);
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

    case SYNTAQLITE_NODE_CAST_EXPR: {
      ast_print_indent(out, depth);
      fprintf(out, "CastExpr\n");
      print_node(out, ast, node->cast_expr.expr, source, depth + 1);
      ast_print_indent(out, depth + 1);
      fprintf(out, "type_name: ");
      ast_print_source_span(out, source, node->cast_expr.type_name);
      fprintf(out, "\n");
      break;
    }

    case SYNTAQLITE_NODE_VALUES_ROW_LIST: {
      ast_print_indent(out, depth);
      fprintf(out, "ValuesRowList[%u]\n", node->values_row_list.count);
      for (uint32_t i = 0; i < node->values_row_list.count; i++) {
        print_node(out, ast, node->values_row_list.children[i], source, depth + 1);
      }
      break;
    }

    case SYNTAQLITE_NODE_VALUES_CLAUSE: {
      ast_print_indent(out, depth);
      fprintf(out, "ValuesClause\n");
      print_node(out, ast, node->values_clause.rows, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_CTE_DEFINITION: {
      ast_print_indent(out, depth);
      fprintf(out, "CteDefinition\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "cte_name: ");
      ast_print_source_span(out, source, node->cte_definition.cte_name);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "materialized: %u\n", node->cte_definition.materialized);
      print_node(out, ast, node->cte_definition.columns, source, depth + 1);
      print_node(out, ast, node->cte_definition.select, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_CTE_LIST: {
      ast_print_indent(out, depth);
      fprintf(out, "CteList[%u]\n", node->cte_list.count);
      for (uint32_t i = 0; i < node->cte_list.count; i++) {
        print_node(out, ast, node->cte_list.children[i], source, depth + 1);
      }
      break;
    }

    case SYNTAQLITE_NODE_WITH_CLAUSE: {
      ast_print_indent(out, depth);
      fprintf(out, "WithClause\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "recursive: %u\n", node->with_clause.recursive);
      print_node(out, ast, node->with_clause.ctes, source, depth + 1);
      print_node(out, ast, node->with_clause.select, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_AGGREGATE_FUNCTION_CALL: {
      ast_print_indent(out, depth);
      fprintf(out, "AggregateFunctionCall\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "func_name: ");
      ast_print_source_span(out, source, node->aggregate_function_call.func_name);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "flags: %u\n", node->aggregate_function_call.flags);
      print_node(out, ast, node->aggregate_function_call.args, source, depth + 1);
      print_node(out, ast, node->aggregate_function_call.orderby, source, depth + 1);
      print_node(out, ast, node->aggregate_function_call.filter_clause, source, depth + 1);
      print_node(out, ast, node->aggregate_function_call.over_clause, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_RAISE_EXPR: {
      ast_print_indent(out, depth);
      fprintf(out, "RaiseExpr\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "raise_type: %s\n", syntaqlite_raise_type_names[node->raise_expr.raise_type]);
      print_node(out, ast, node->raise_expr.error_message, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_TABLE_REF: {
      ast_print_indent(out, depth);
      fprintf(out, "TableRef\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "table_name: ");
      ast_print_source_span(out, source, node->table_ref.table_name);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "schema: ");
      ast_print_source_span(out, source, node->table_ref.schema);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "alias: ");
      ast_print_source_span(out, source, node->table_ref.alias);
      fprintf(out, "\n");
      break;
    }

    case SYNTAQLITE_NODE_SUBQUERY_TABLE_SOURCE: {
      ast_print_indent(out, depth);
      fprintf(out, "SubqueryTableSource\n");
      print_node(out, ast, node->subquery_table_source.select, source, depth + 1);
      ast_print_indent(out, depth + 1);
      fprintf(out, "alias: ");
      ast_print_source_span(out, source, node->subquery_table_source.alias);
      fprintf(out, "\n");
      break;
    }

    case SYNTAQLITE_NODE_JOIN_CLAUSE: {
      ast_print_indent(out, depth);
      fprintf(out, "JoinClause\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "join_type: %s\n", syntaqlite_join_type_names[node->join_clause.join_type]);
      print_node(out, ast, node->join_clause.left, source, depth + 1);
      print_node(out, ast, node->join_clause.right, source, depth + 1);
      print_node(out, ast, node->join_clause.on_expr, source, depth + 1);
      print_node(out, ast, node->join_clause.using_columns, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_JOIN_PREFIX: {
      ast_print_indent(out, depth);
      fprintf(out, "JoinPrefix\n");
      print_node(out, ast, node->join_prefix.source, source, depth + 1);
      ast_print_indent(out, depth + 1);
      fprintf(out, "join_type: %s\n", syntaqlite_join_type_names[node->join_prefix.join_type]);
      break;
    }

    case SYNTAQLITE_NODE_DELETE_STMT: {
      ast_print_indent(out, depth);
      fprintf(out, "DeleteStmt\n");
      print_node(out, ast, node->delete_stmt.table, source, depth + 1);
      print_node(out, ast, node->delete_stmt.where, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_SET_CLAUSE: {
      ast_print_indent(out, depth);
      fprintf(out, "SetClause\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "column: ");
      ast_print_source_span(out, source, node->set_clause.column);
      fprintf(out, "\n");
      print_node(out, ast, node->set_clause.columns, source, depth + 1);
      print_node(out, ast, node->set_clause.value, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_SET_CLAUSE_LIST: {
      ast_print_indent(out, depth);
      fprintf(out, "SetClauseList[%u]\n", node->set_clause_list.count);
      for (uint32_t i = 0; i < node->set_clause_list.count; i++) {
        print_node(out, ast, node->set_clause_list.children[i], source, depth + 1);
      }
      break;
    }

    case SYNTAQLITE_NODE_UPDATE_STMT: {
      ast_print_indent(out, depth);
      fprintf(out, "UpdateStmt\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "conflict_action: %s\n", syntaqlite_conflict_action_names[node->update_stmt.conflict_action]);
      print_node(out, ast, node->update_stmt.table, source, depth + 1);
      print_node(out, ast, node->update_stmt.setlist, source, depth + 1);
      print_node(out, ast, node->update_stmt.from_clause, source, depth + 1);
      print_node(out, ast, node->update_stmt.where, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_INSERT_STMT: {
      ast_print_indent(out, depth);
      fprintf(out, "InsertStmt\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "conflict_action: %s\n", syntaqlite_conflict_action_names[node->insert_stmt.conflict_action]);
      print_node(out, ast, node->insert_stmt.table, source, depth + 1);
      print_node(out, ast, node->insert_stmt.columns, source, depth + 1);
      print_node(out, ast, node->insert_stmt.source, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_QUALIFIED_NAME: {
      ast_print_indent(out, depth);
      fprintf(out, "QualifiedName\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "object_name: ");
      ast_print_source_span(out, source, node->qualified_name.object_name);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "schema: ");
      ast_print_source_span(out, source, node->qualified_name.schema);
      fprintf(out, "\n");
      break;
    }

    case SYNTAQLITE_NODE_DROP_STMT: {
      ast_print_indent(out, depth);
      fprintf(out, "DropStmt\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "object_type: %s\n", syntaqlite_drop_object_type_names[node->drop_stmt.object_type]);
      ast_print_indent(out, depth + 1);
      fprintf(out, "if_exists: %u\n", node->drop_stmt.if_exists);
      print_node(out, ast, node->drop_stmt.target, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_ALTER_TABLE_STMT: {
      ast_print_indent(out, depth);
      fprintf(out, "AlterTableStmt\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "op: %s\n", syntaqlite_alter_op_names[node->alter_table_stmt.op]);
      print_node(out, ast, node->alter_table_stmt.target, source, depth + 1);
      ast_print_indent(out, depth + 1);
      fprintf(out, "new_name: ");
      ast_print_source_span(out, source, node->alter_table_stmt.new_name);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "old_name: ");
      ast_print_source_span(out, source, node->alter_table_stmt.old_name);
      fprintf(out, "\n");
      break;
    }

    case SYNTAQLITE_NODE_TRANSACTION_STMT: {
      ast_print_indent(out, depth);
      fprintf(out, "TransactionStmt\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "op: %s\n", syntaqlite_transaction_op_names[node->transaction_stmt.op]);
      ast_print_indent(out, depth + 1);
      fprintf(out, "trans_type: %s\n", syntaqlite_transaction_type_names[node->transaction_stmt.trans_type]);
      break;
    }

    case SYNTAQLITE_NODE_SAVEPOINT_STMT: {
      ast_print_indent(out, depth);
      fprintf(out, "SavepointStmt\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "op: %s\n", syntaqlite_savepoint_op_names[node->savepoint_stmt.op]);
      ast_print_indent(out, depth + 1);
      fprintf(out, "savepoint_name: ");
      ast_print_source_span(out, source, node->savepoint_stmt.savepoint_name);
      fprintf(out, "\n");
      break;
    }

    case SYNTAQLITE_NODE_PRAGMA_STMT: {
      ast_print_indent(out, depth);
      fprintf(out, "PragmaStmt\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "pragma_name: ");
      ast_print_source_span(out, source, node->pragma_stmt.pragma_name);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "schema: ");
      ast_print_source_span(out, source, node->pragma_stmt.schema);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "value: ");
      ast_print_source_span(out, source, node->pragma_stmt.value);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "pragma_form: %u\n", node->pragma_stmt.pragma_form);
      break;
    }

    case SYNTAQLITE_NODE_ANALYZE_STMT: {
      ast_print_indent(out, depth);
      fprintf(out, "AnalyzeStmt\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "target_name: ");
      ast_print_source_span(out, source, node->analyze_stmt.target_name);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "schema: ");
      ast_print_source_span(out, source, node->analyze_stmt.schema);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "is_reindex: %u\n", node->analyze_stmt.is_reindex);
      break;
    }

    case SYNTAQLITE_NODE_ATTACH_STMT: {
      ast_print_indent(out, depth);
      fprintf(out, "AttachStmt\n");
      print_node(out, ast, node->attach_stmt.filename, source, depth + 1);
      print_node(out, ast, node->attach_stmt.db_name, source, depth + 1);
      print_node(out, ast, node->attach_stmt.key, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_DETACH_STMT: {
      ast_print_indent(out, depth);
      fprintf(out, "DetachStmt\n");
      print_node(out, ast, node->detach_stmt.db_name, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_VACUUM_STMT: {
      ast_print_indent(out, depth);
      fprintf(out, "VacuumStmt\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "schema: ");
      ast_print_source_span(out, source, node->vacuum_stmt.schema);
      fprintf(out, "\n");
      print_node(out, ast, node->vacuum_stmt.into_expr, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_EXPLAIN_STMT: {
      ast_print_indent(out, depth);
      fprintf(out, "ExplainStmt\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "explain_mode: %s\n", syntaqlite_explain_mode_names[node->explain_stmt.explain_mode]);
      print_node(out, ast, node->explain_stmt.stmt, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_CREATE_INDEX_STMT: {
      ast_print_indent(out, depth);
      fprintf(out, "CreateIndexStmt\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "index_name: ");
      ast_print_source_span(out, source, node->create_index_stmt.index_name);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "schema: ");
      ast_print_source_span(out, source, node->create_index_stmt.schema);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "table_name: ");
      ast_print_source_span(out, source, node->create_index_stmt.table_name);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "is_unique: %u\n", node->create_index_stmt.is_unique);
      ast_print_indent(out, depth + 1);
      fprintf(out, "if_not_exists: %u\n", node->create_index_stmt.if_not_exists);
      print_node(out, ast, node->create_index_stmt.columns, source, depth + 1);
      print_node(out, ast, node->create_index_stmt.where, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_CREATE_VIEW_STMT: {
      ast_print_indent(out, depth);
      fprintf(out, "CreateViewStmt\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "view_name: ");
      ast_print_source_span(out, source, node->create_view_stmt.view_name);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "schema: ");
      ast_print_source_span(out, source, node->create_view_stmt.schema);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "is_temp: %u\n", node->create_view_stmt.is_temp);
      ast_print_indent(out, depth + 1);
      fprintf(out, "if_not_exists: %u\n", node->create_view_stmt.if_not_exists);
      print_node(out, ast, node->create_view_stmt.column_names, source, depth + 1);
      print_node(out, ast, node->create_view_stmt.select, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_FOREIGN_KEY_CLAUSE: {
      ast_print_indent(out, depth);
      fprintf(out, "ForeignKeyClause\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "ref_table: ");
      ast_print_source_span(out, source, node->foreign_key_clause.ref_table);
      fprintf(out, "\n");
      print_node(out, ast, node->foreign_key_clause.ref_columns, source, depth + 1);
      ast_print_indent(out, depth + 1);
      fprintf(out, "on_delete: %s\n", syntaqlite_foreign_key_action_names[node->foreign_key_clause.on_delete]);
      ast_print_indent(out, depth + 1);
      fprintf(out, "on_update: %s\n", syntaqlite_foreign_key_action_names[node->foreign_key_clause.on_update]);
      ast_print_indent(out, depth + 1);
      fprintf(out, "is_deferred: %u\n", node->foreign_key_clause.is_deferred);
      break;
    }

    case SYNTAQLITE_NODE_COLUMN_CONSTRAINT: {
      ast_print_indent(out, depth);
      fprintf(out, "ColumnConstraint\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "kind: %s\n", syntaqlite_column_constraint_kind_names[node->column_constraint.kind]);
      ast_print_indent(out, depth + 1);
      fprintf(out, "constraint_name: ");
      ast_print_source_span(out, source, node->column_constraint.constraint_name);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "onconf: %u\n", node->column_constraint.onconf);
      ast_print_indent(out, depth + 1);
      fprintf(out, "sort_order: %u\n", node->column_constraint.sort_order);
      ast_print_indent(out, depth + 1);
      fprintf(out, "is_autoincrement: %u\n", node->column_constraint.is_autoincrement);
      ast_print_indent(out, depth + 1);
      fprintf(out, "collation_name: ");
      ast_print_source_span(out, source, node->column_constraint.collation_name);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "generated_storage: %s\n", syntaqlite_generated_column_storage_names[node->column_constraint.generated_storage]);
      print_node(out, ast, node->column_constraint.default_expr, source, depth + 1);
      print_node(out, ast, node->column_constraint.check_expr, source, depth + 1);
      print_node(out, ast, node->column_constraint.generated_expr, source, depth + 1);
      print_node(out, ast, node->column_constraint.fk_clause, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_COLUMN_CONSTRAINT_LIST: {
      ast_print_indent(out, depth);
      fprintf(out, "ColumnConstraintList[%u]\n", node->column_constraint_list.count);
      for (uint32_t i = 0; i < node->column_constraint_list.count; i++) {
        print_node(out, ast, node->column_constraint_list.children[i], source, depth + 1);
      }
      break;
    }

    case SYNTAQLITE_NODE_COLUMN_DEF: {
      ast_print_indent(out, depth);
      fprintf(out, "ColumnDef\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "column_name: ");
      ast_print_source_span(out, source, node->column_def.column_name);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "type_name: ");
      ast_print_source_span(out, source, node->column_def.type_name);
      fprintf(out, "\n");
      print_node(out, ast, node->column_def.constraints, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_COLUMN_DEF_LIST: {
      ast_print_indent(out, depth);
      fprintf(out, "ColumnDefList[%u]\n", node->column_def_list.count);
      for (uint32_t i = 0; i < node->column_def_list.count; i++) {
        print_node(out, ast, node->column_def_list.children[i], source, depth + 1);
      }
      break;
    }

    case SYNTAQLITE_NODE_TABLE_CONSTRAINT: {
      ast_print_indent(out, depth);
      fprintf(out, "TableConstraint\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "kind: %s\n", syntaqlite_table_constraint_kind_names[node->table_constraint.kind]);
      ast_print_indent(out, depth + 1);
      fprintf(out, "constraint_name: ");
      ast_print_source_span(out, source, node->table_constraint.constraint_name);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "onconf: %u\n", node->table_constraint.onconf);
      ast_print_indent(out, depth + 1);
      fprintf(out, "is_autoincrement: %u\n", node->table_constraint.is_autoincrement);
      print_node(out, ast, node->table_constraint.columns, source, depth + 1);
      print_node(out, ast, node->table_constraint.check_expr, source, depth + 1);
      print_node(out, ast, node->table_constraint.fk_clause, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_TABLE_CONSTRAINT_LIST: {
      ast_print_indent(out, depth);
      fprintf(out, "TableConstraintList[%u]\n", node->table_constraint_list.count);
      for (uint32_t i = 0; i < node->table_constraint_list.count; i++) {
        print_node(out, ast, node->table_constraint_list.children[i], source, depth + 1);
      }
      break;
    }

    case SYNTAQLITE_NODE_CREATE_TABLE_STMT: {
      ast_print_indent(out, depth);
      fprintf(out, "CreateTableStmt\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "table_name: ");
      ast_print_source_span(out, source, node->create_table_stmt.table_name);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "schema: ");
      ast_print_source_span(out, source, node->create_table_stmt.schema);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "is_temp: %u\n", node->create_table_stmt.is_temp);
      ast_print_indent(out, depth + 1);
      fprintf(out, "if_not_exists: %u\n", node->create_table_stmt.if_not_exists);
      ast_print_indent(out, depth + 1);
      fprintf(out, "table_options: %u\n", node->create_table_stmt.table_options);
      print_node(out, ast, node->create_table_stmt.columns, source, depth + 1);
      print_node(out, ast, node->create_table_stmt.table_constraints, source, depth + 1);
      print_node(out, ast, node->create_table_stmt.as_select, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_FRAME_BOUND: {
      ast_print_indent(out, depth);
      fprintf(out, "FrameBound\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "bound_type: %s\n", syntaqlite_frame_bound_type_names[node->frame_bound.bound_type]);
      print_node(out, ast, node->frame_bound.expr, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_FRAME_SPEC: {
      ast_print_indent(out, depth);
      fprintf(out, "FrameSpec\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "frame_type: %s\n", syntaqlite_frame_type_names[node->frame_spec.frame_type]);
      ast_print_indent(out, depth + 1);
      fprintf(out, "exclude: %s\n", syntaqlite_frame_exclude_names[node->frame_spec.exclude]);
      print_node(out, ast, node->frame_spec.start_bound, source, depth + 1);
      print_node(out, ast, node->frame_spec.end_bound, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_WINDOW_DEF: {
      ast_print_indent(out, depth);
      fprintf(out, "WindowDef\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "base_window_name: ");
      ast_print_source_span(out, source, node->window_def.base_window_name);
      fprintf(out, "\n");
      print_node(out, ast, node->window_def.partition_by, source, depth + 1);
      print_node(out, ast, node->window_def.orderby, source, depth + 1);
      print_node(out, ast, node->window_def.frame, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_WINDOW_DEF_LIST: {
      ast_print_indent(out, depth);
      fprintf(out, "WindowDefList[%u]\n", node->window_def_list.count);
      for (uint32_t i = 0; i < node->window_def_list.count; i++) {
        print_node(out, ast, node->window_def_list.children[i], source, depth + 1);
      }
      break;
    }

    case SYNTAQLITE_NODE_NAMED_WINDOW_DEF: {
      ast_print_indent(out, depth);
      fprintf(out, "NamedWindowDef\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "window_name: ");
      ast_print_source_span(out, source, node->named_window_def.window_name);
      fprintf(out, "\n");
      print_node(out, ast, node->named_window_def.window_def, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_NAMED_WINDOW_DEF_LIST: {
      ast_print_indent(out, depth);
      fprintf(out, "NamedWindowDefList[%u]\n", node->named_window_def_list.count);
      for (uint32_t i = 0; i < node->named_window_def_list.count; i++) {
        print_node(out, ast, node->named_window_def_list.children[i], source, depth + 1);
      }
      break;
    }

    case SYNTAQLITE_NODE_FILTER_OVER: {
      ast_print_indent(out, depth);
      fprintf(out, "FilterOver\n");
      print_node(out, ast, node->filter_over.filter_expr, source, depth + 1);
      print_node(out, ast, node->filter_over.over_def, source, depth + 1);
      ast_print_indent(out, depth + 1);
      fprintf(out, "over_name: ");
      ast_print_source_span(out, source, node->filter_over.over_name);
      fprintf(out, "\n");
      break;
    }

    case SYNTAQLITE_NODE_TRIGGER_EVENT: {
      ast_print_indent(out, depth);
      fprintf(out, "TriggerEvent\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "event_type: %s\n", syntaqlite_trigger_event_type_names[node->trigger_event.event_type]);
      print_node(out, ast, node->trigger_event.columns, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_TRIGGER_CMD_LIST: {
      ast_print_indent(out, depth);
      fprintf(out, "TriggerCmdList[%u]\n", node->trigger_cmd_list.count);
      for (uint32_t i = 0; i < node->trigger_cmd_list.count; i++) {
        print_node(out, ast, node->trigger_cmd_list.children[i], source, depth + 1);
      }
      break;
    }

    case SYNTAQLITE_NODE_CREATE_TRIGGER_STMT: {
      ast_print_indent(out, depth);
      fprintf(out, "CreateTriggerStmt\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "trigger_name: ");
      ast_print_source_span(out, source, node->create_trigger_stmt.trigger_name);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "schema: ");
      ast_print_source_span(out, source, node->create_trigger_stmt.schema);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "is_temp: %u\n", node->create_trigger_stmt.is_temp);
      ast_print_indent(out, depth + 1);
      fprintf(out, "if_not_exists: %u\n", node->create_trigger_stmt.if_not_exists);
      ast_print_indent(out, depth + 1);
      fprintf(out, "timing: %s\n", syntaqlite_trigger_timing_names[node->create_trigger_stmt.timing]);
      print_node(out, ast, node->create_trigger_stmt.event, source, depth + 1);
      print_node(out, ast, node->create_trigger_stmt.table, source, depth + 1);
      print_node(out, ast, node->create_trigger_stmt.when_expr, source, depth + 1);
      print_node(out, ast, node->create_trigger_stmt.body, source, depth + 1);
      break;
    }

    case SYNTAQLITE_NODE_CREATE_VIRTUAL_TABLE_STMT: {
      ast_print_indent(out, depth);
      fprintf(out, "CreateVirtualTableStmt\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "table_name: ");
      ast_print_source_span(out, source, node->create_virtual_table_stmt.table_name);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "schema: ");
      ast_print_source_span(out, source, node->create_virtual_table_stmt.schema);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "module_name: ");
      ast_print_source_span(out, source, node->create_virtual_table_stmt.module_name);
      fprintf(out, "\n");
      ast_print_indent(out, depth + 1);
      fprintf(out, "if_not_exists: %u\n", node->create_virtual_table_stmt.if_not_exists);
      ast_print_indent(out, depth + 1);
      fprintf(out, "module_args: ");
      ast_print_source_span(out, source, node->create_virtual_table_stmt.module_args);
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
