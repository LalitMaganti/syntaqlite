// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// select_columns: parse SQL and resolve output columns, expanding * using
// schema knowledge from CREATE TABLE / CREATE VIEW / CTEs.
//
// This is an example / test of the syntaqlite C API ergonomics.

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "syntaqlite_sqlite.h"

// ── Schema ──────────────────────────────────────────────────────────────

typedef struct Column {
  char name[128];
  char source_table[128];
} Column;

typedef struct TableSchema {
  char name[128];
  Column columns[64];
  int column_count;
} TableSchema;

typedef struct Schema {
  TableSchema tables[64];
  int table_count;
} Schema;

static void schema_put(Schema* s,
                       const char* name,
                       const Column* cols,
                       int count) {
  for (int i = 0; i < s->table_count; i++) {
    if (strcasecmp(s->tables[i].name, name) == 0) {
      memcpy(s->tables[i].columns, cols, count * sizeof(Column));
      s->tables[i].column_count = count;
      return;
    }
  }
  TableSchema* ts = &s->tables[s->table_count++];
  snprintf(ts->name, sizeof(ts->name), "%s", name);
  memcpy(ts->columns, cols, count * sizeof(Column));
  ts->column_count = count;
}

static const TableSchema* schema_get(const Schema* s, const char* name) {
  for (int i = 0; i < s->table_count; i++) {
    if (strcasecmp(s->tables[i].name, name) == 0)
      return &s->tables[i];
  }
  return NULL;
}

// ── Helpers ─────────────────────────────────────────────────────────────

static void span_to_str(SyntaqliteParser* p,
                        SyntaqliteSourceSpan span,
                        char* buf,
                        int buf_size) {
  SyntaqliteResolvedSpan resolved = syntaqlite_parser_resolve_span(p, &span);
  if (!resolved.text || resolved.text_len == 0) {
    buf[0] = '\0';
    return;
  }
  int n = (int)resolved.text_len < buf_size - 1 ? (int)resolved.text_len
                                                : buf_size - 1;
  memcpy(buf, resolved.text, n);
  buf[n] = '\0';
}

// Extract text from an IdentName node (used for alias, column_name, etc.).
static void ident_to_str(SyntaqliteParser* p,
                         uint32_t ident_id,
                         char* buf,
                         int buf_size) {
  const SyntaqliteIdentName* ident =
      SYNTAQLITE_NODE(p, SyntaqliteIdentName, ident_id);
  if (ident)
    span_to_str(p, ident->source, buf, buf_size);
  else
    buf[0] = '\0';
}

static void str_lower(char* s) {
  for (; *s; s++)
    *s = (char)tolower((unsigned char)*s);
}

// ── Column list (growable result) ───────────────────────────────────────

typedef struct ColumnList {
  Column items[256];
  int count;
} ColumnList;

static void col_list_push(ColumnList* cl,
                          const char* name,
                          const char* source) {
  Column* c = &cl->items[cl->count++];
  snprintf(c->name, sizeof(c->name), "%s", name);
  snprintf(c->source_table, sizeof(c->source_table), "%s", source);
}

static void col_list_push_col(ColumnList* cl, const Column* col) {
  cl->items[cl->count++] = *col;
}

// ── Forward declarations ────────────────────────────────────────────────

static void resolve_select_columns(SyntaqliteParser* p,
                                   const SyntaqliteSelectStmt* select,
                                   const Schema* schema,
                                   ColumnList* out);

static void resolve_stmt_columns(SyntaqliteParser* p,
                                 uint32_t stmt_id,
                                 const Schema* schema,
                                 ColumnList* out);

// ── FROM clause: collect table sources ──────────────────────────────────

typedef struct TableSource {
  char name[128];
  char alias[128];
  Column columns[64];
  int column_count;
} TableSource;

typedef struct TableSourceList {
  TableSource items[32];
  int count;
} TableSourceList;

static void collect_from_sources(SyntaqliteParser* p,
                                 uint32_t from_id,
                                 const Schema* schema,
                                 TableSourceList* out) {
  const SyntaqliteTableSource* node =
      SYNTAQLITE_NODE(p, SyntaqliteTableSource, from_id);
  if (!node)
    return;

  switch (node->tag) {
    case SYNTAQLITE_NODE_TABLE_REF: {
      const SyntaqliteTableRef* ref = &node->table_ref;
      TableSource* ts = &out->items[out->count++];
      span_to_str(p, ref->table_name, ts->name, sizeof(ts->name));
      if (syntaqlite_node_is_present(ref->alias))
        ident_to_str(p, ref->alias, ts->alias, sizeof(ts->alias));
      else
        snprintf(ts->alias, sizeof(ts->alias), "%s", ts->name);

      char lower_name[128];
      snprintf(lower_name, sizeof(lower_name), "%s", ts->name);
      str_lower(lower_name);
      const TableSchema* tbl = schema_get(schema, lower_name);
      if (tbl) {
        ts->column_count = tbl->column_count;
        for (int i = 0; i < tbl->column_count; i++) {
          ts->columns[i] = tbl->columns[i];
          snprintf(ts->columns[i].source_table,
                   sizeof(ts->columns[i].source_table), "%s", ts->alias);
        }
      } else {
        ts->column_count = 0;
      }
      break;
    }

    case SYNTAQLITE_NODE_SUBQUERY_TABLE_SOURCE: {
      const SyntaqliteSubqueryTableSource* sub = &node->subquery_table_source;
      TableSource* ts = &out->items[out->count++];
      snprintf(ts->name, sizeof(ts->name), "(subquery)");
      ident_to_str(p, sub->alias, ts->alias, sizeof(ts->alias));
      ColumnList cols = {0};
      resolve_stmt_columns(p, sub->select, schema, &cols);
      ts->column_count = cols.count;
      for (int i = 0; i < cols.count; i++) {
        ts->columns[i] = cols.items[i];
        snprintf(ts->columns[i].source_table,
                 sizeof(ts->columns[i].source_table), "%s", ts->alias);
      }
      break;
    }

    case SYNTAQLITE_NODE_JOIN_CLAUSE: {
      const SyntaqliteJoinClause* join = &node->join_clause;
      collect_from_sources(p, join->left, schema, out);
      collect_from_sources(p, join->right, schema, out);
      break;
    }

    case SYNTAQLITE_NODE_JOIN_PREFIX: {
      const SyntaqliteJoinPrefix* jp = &node->join_prefix;
      collect_from_sources(p, jp->source, schema, out);
      break;
    }

    default:
      break;
  }
}

// ── Expression → column name + source table ─────────────────────────────

// Resolve the source table for an expression. If the expression is a column
// reference with an explicit table qualifier (e.g. u.name), use that. If it's
// a bare column reference, search the FROM sources for the first table that
// has a matching column name. For compound expressions, leave source empty.
static void expr_source(SyntaqliteParser* p,
                        uint32_t expr_id,
                        const TableSourceList* sources,
                        char* buf,
                        int buf_size) {
  buf[0] = '\0';
  const SyntaqliteExpr* expr = SYNTAQLITE_NODE(p, SyntaqliteExpr, expr_id);
  if (!expr)
    return;

  if (expr->tag == SYNTAQLITE_NODE_COLUMN_REF) {
    const SyntaqliteColumnRef* ref = &expr->column_ref;
    if (ref->table.length != 0) {
      // Explicit qualifier: SELECT u.name → source is "u"
      span_to_str(p, ref->table, buf, buf_size);
      return;
    }
    // Bare column: search FROM sources for a match.
    char col_name[128];
    span_to_str(p, ref->column, col_name, sizeof(col_name));
    for (int s = 0; s < sources->count; s++) {
      for (int c = 0; c < sources->items[s].column_count; c++) {
        if (strcasecmp(sources->items[s].columns[c].name, col_name) == 0) {
          snprintf(buf, buf_size, "%s", sources->items[s].alias);
          return;
        }
      }
    }
  }
}

static void expr_name(SyntaqliteParser* p,
                      uint32_t expr_id,
                      char* buf,
                      int buf_size) {
  const SyntaqliteExpr* expr = SYNTAQLITE_NODE(p, SyntaqliteExpr, expr_id);
  if (!expr) {
    snprintf(buf, buf_size, "?");
    return;
  }

  switch (expr->tag) {
    case SYNTAQLITE_NODE_COLUMN_REF:
      span_to_str(p, expr->column_ref.column, buf, buf_size);
      return;

    case SYNTAQLITE_NODE_LITERAL:
      span_to_str(p, expr->literal.source, buf, buf_size);
      return;

    case SYNTAQLITE_NODE_FUNCTION_CALL:
      span_to_str(p, expr->function_call.func_name, buf, buf_size);
      strncat(buf, "(...)", buf_size - strlen(buf) - 1);
      return;

    case SYNTAQLITE_NODE_AGGREGATE_FUNCTION_CALL:
      span_to_str(p, expr->aggregate_function_call.func_name, buf, buf_size);
      strncat(buf, "(...)", buf_size - strlen(buf) - 1);
      return;

    case SYNTAQLITE_NODE_CAST_EXPR: {
      char inner[128];
      expr_name(p, expr->cast_expr.expr, inner, sizeof(inner));
      snprintf(buf, buf_size, "CAST(%s)", inner);
      return;
    }

    case SYNTAQLITE_NODE_SUBQUERY_EXPR:
      snprintf(buf, buf_size, "(subquery)");
      return;

    case SYNTAQLITE_NODE_BINARY_EXPR: {
      char left[128], right[128];
      expr_name(p, expr->binary_expr.left, left, sizeof(left));
      expr_name(p, expr->binary_expr.right, right, sizeof(right));
      snprintf(buf, buf_size, "(%s op %s)", left, right);
      return;
    }

    case SYNTAQLITE_NODE_UNARY_EXPR: {
      char operand[128];
      expr_name(p, expr->unary_expr.operand, operand, sizeof(operand));
      snprintf(buf, buf_size, "op(%s)", operand);
      return;
    }

    default:
      snprintf(buf, buf_size, "<expr>");
      return;
  }
}

// ── Resolve SELECT columns ──────────────────────────────────────────────

static void resolve_select_columns(SyntaqliteParser* p,
                                   const SyntaqliteSelectStmt* select,
                                   const Schema* schema,
                                   ColumnList* out) {
  TableSourceList sources = {0};
  collect_from_sources(p, select->from_clause, schema, &sources);

  if (!syntaqlite_node_is_present(select->columns))
    return;

  SYNTAQLITE_LIST_FOREACH(p, SyntaqliteResultColumn, rc, select->columns) {
    if (rc->flags.bits.star) {
      char qualifier[128] = {0};
      if (syntaqlite_node_is_present(rc->alias))
        ident_to_str(p, rc->alias, qualifier, sizeof(qualifier));

      if (qualifier[0] == '\0') {
        int expanded = 0;
        for (int s = 0; s < sources.count; s++) {
          if (sources.items[s].column_count > 0) {
            for (int c = 0; c < sources.items[s].column_count; c++)
              col_list_push_col(out, &sources.items[s].columns[c]);
            expanded = 1;
          }
        }
        if (!expanded)
          col_list_push(out, "*", "");
      } else {
        int found = 0;
        for (int s = 0; s < sources.count; s++) {
          if (strcasecmp(sources.items[s].alias, qualifier) == 0 ||
              strcasecmp(sources.items[s].name, qualifier) == 0) {
            if (sources.items[s].column_count > 0) {
              for (int c = 0; c < sources.items[s].column_count; c++)
                col_list_push_col(out, &sources.items[s].columns[c]);
            } else {
              char star_name[128];
              snprintf(star_name, sizeof(star_name), "%s.*", qualifier);
              col_list_push(out, star_name, qualifier);
            }
            found = 1;
            break;
          }
        }
        if (!found) {
          char star_name[128];
          snprintf(star_name, sizeof(star_name), "%s.*", qualifier);
          col_list_push(out, star_name, qualifier);
        }
      }
      continue;
    }

    char name[128];
    if (syntaqlite_node_is_present(rc->alias)) {
      ident_to_str(p, rc->alias, name, sizeof(name));
    } else {
      expr_name(p, rc->expr, name, sizeof(name));
    }
    char source[128];
    expr_source(p, rc->expr, &sources, source, sizeof(source));
    col_list_push(out, name, source);
  }
}

// ── Resolve any statement's columns ─────────────────────────────────────

static void resolve_stmt_columns(SyntaqliteParser* p,
                                 uint32_t stmt_id,
                                 const Schema* schema,
                                 ColumnList* out) {
  const SyntaqliteStmt* stmt = SYNTAQLITE_NODE(p, SyntaqliteStmt, stmt_id);
  if (!stmt)
    return;

  switch (stmt->tag) {
    case SYNTAQLITE_NODE_SELECT_STMT:
      resolve_select_columns(p, &stmt->select_stmt, schema, out);
      return;

    case SYNTAQLITE_NODE_WITH_CLAUSE: {
      const SyntaqliteWithClause* with = &stmt->with_clause;
      Schema local = *schema;

      SYNTAQLITE_LIST_FOREACH(p, SyntaqliteCteDefinition, cte, with->ctes) {
        char cte_name[128];
        span_to_str(p, cte->cte_name, cte_name, sizeof(cte_name));
        str_lower(cte_name);

        if (syntaqlite_node_is_present(cte->columns)) {
          Column cols[64];
          int nc = 0;
          SYNTAQLITE_LIST_FOREACH(p, SyntaqliteColumnRef, col_ref,
                                  cte->columns) {
            span_to_str(p, col_ref->column, cols[nc].name,
                        sizeof(cols[nc].name));
            snprintf(cols[nc].source_table, sizeof(cols[nc].source_table), "%s",
                     cte_name);
            nc++;
          }
          schema_put(&local, cte_name, cols, nc);
        } else {
          ColumnList cols = {0};
          resolve_stmt_columns(p, cte->select, &local, &cols);
          for (int c = 0; c < cols.count; c++)
            snprintf(cols.items[c].source_table,
                     sizeof(cols.items[c].source_table), "%s", cte_name);
          schema_put(&local, cte_name, cols.items, cols.count);
        }
      }

      resolve_stmt_columns(p, with->select, &local, out);
      return;
    }

    case SYNTAQLITE_NODE_COMPOUND_SELECT:
      resolve_stmt_columns(p, stmt->compound_select.left, schema, out);
      return;

    default:
      return;
  }
}

// ── Process a top-level statement ───────────────────────────────────────

static int process_statement(SyntaqliteParser* p,
                             uint32_t root_id,
                             Schema* schema) {
  const SyntaqliteStmt* stmt = SYNTAQLITE_NODE(p, SyntaqliteStmt, root_id);

  switch (stmt->tag) {
    case SYNTAQLITE_NODE_CREATE_TABLE_STMT: {
      const SyntaqliteCreateTableStmt* ct = &stmt->create_table_stmt;
      char table_name[128];
      span_to_str(p, ct->table_name, table_name, sizeof(table_name));
      str_lower(table_name);

      if (syntaqlite_node_is_present(ct->columns)) {
        Column cols[64];
        int count = 0;
        SYNTAQLITE_LIST_FOREACH(p, SyntaqliteColumnDef, col_def, ct->columns) {
          ident_to_str(p, col_def->column_name, cols[count].name,
                       sizeof(cols[count].name));
          snprintf(cols[count].source_table, sizeof(cols[count].source_table),
                   "%s", table_name);
          count++;
        }
        schema_put(schema, table_name, cols, count);
        printf("registered table '%s'\n", table_name);
      } else if (syntaqlite_node_is_present(ct->as_select)) {
        ColumnList cols = {0};
        resolve_stmt_columns(p, ct->as_select, schema, &cols);
        for (int c = 0; c < cols.count; c++)
          snprintf(cols.items[c].source_table,
                   sizeof(cols.items[c].source_table), "%s", table_name);
        schema_put(schema, table_name, cols.items, cols.count);
        printf("registered table '%s' (from SELECT)\n", table_name);
      }
      return 0;
    }

    case SYNTAQLITE_NODE_CREATE_VIEW_STMT: {
      const SyntaqliteCreateViewStmt* cv = &stmt->create_view_stmt;
      char view_name[128];
      span_to_str(p, cv->view_name, view_name, sizeof(view_name));
      str_lower(view_name);

      if (syntaqlite_node_is_present(cv->column_names)) {
        Column cols[64];
        int count = 0;
        SYNTAQLITE_LIST_FOREACH(p, SyntaqliteColumnRef, col_ref,
                                cv->column_names) {
          span_to_str(p, col_ref->column, cols[count].name,
                      sizeof(cols[count].name));
          snprintf(cols[count].source_table, sizeof(cols[count].source_table),
                   "%s", view_name);
          count++;
        }
        schema_put(schema, view_name, cols, count);
      } else {
        ColumnList cols = {0};
        resolve_stmt_columns(p, cv->select, schema, &cols);
        for (int c = 0; c < cols.count; c++)
          snprintf(cols.items[c].source_table,
                   sizeof(cols.items[c].source_table), "%s", view_name);
        schema_put(schema, view_name, cols.items, cols.count);
      }
      printf("registered view '%s'\n", view_name);
      return 0;
    }

    case SYNTAQLITE_NODE_SELECT_STMT:
    case SYNTAQLITE_NODE_WITH_CLAUSE:
    case SYNTAQLITE_NODE_COMPOUND_SELECT: {
      ColumnList cols = {0};
      resolve_stmt_columns(p, root_id, schema, &cols);
      printf("output columns (%d):\n", cols.count);
      for (int i = 0; i < cols.count; i++) {
        printf("  [%d] %s", i + 1, cols.items[i].name);
        if (cols.items[i].source_table[0])
          printf("  (from %s)", cols.items[i].source_table);
        printf("\n");
      }
      return 0;
    }

    case SYNTAQLITE_NODE_EXPLAIN_STMT:
      printf("EXPLAIN statement — not a data query\n");
      return 0;

    default:
      fprintf(stderr, "skipping non-query statement (tag=%d)\n", stmt->tag);
      return 0;
  }
}

// ── Main ────────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
  const char* sql = NULL;

  if (argc >= 2) {
    sql = argv[1];
  } else {
    static char buf[64 * 1024];
    size_t n = fread(buf, 1, sizeof(buf) - 1, stdin);
    buf[n] = '\0';
    sql = buf;
  }

  SyntaqliteParser* p = syntaqlite_parser_create(NULL);
  syntaqlite_parser_reset(p, sql, (uint32_t)strlen(sql));

  Schema schema = {0};
  int stmt_num = 0;

  for (;;) {
    int32_t rc = syntaqlite_parser_next(p);
    if (rc == SYNTAQLITE_PARSE_DONE)
      break;
    if (rc == SYNTAQLITE_PARSE_ERROR) {
      const char* msg = syntaqlite_result_error_msg(p);
      fprintf(stderr, "parse error: %s\n", msg ? msg : "unknown");
      syntaqlite_parser_destroy(p);
      return 1;
    }

    uint32_t root = syntaqlite_result_root(p);
    stmt_num++;
    if (stmt_num > 1)
      printf("\n");
    process_statement(p, root, &schema);
  }

  if (stmt_num == 0) {
    fprintf(stderr, "error: no SQL statement provided\n");
    syntaqlite_parser_destroy(p);
    return 1;
  }

  syntaqlite_parser_destroy(p);
  return 0;
}
