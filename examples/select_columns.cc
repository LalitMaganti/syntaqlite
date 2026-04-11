// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// select_columns: parse SQL and resolve output columns, expanding * using
// schema knowledge from CREATE TABLE / CREATE VIEW / CTEs.
//
// This is an example / test of the syntaqlite C API used from C++.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "syntaqlite_sqlite.h"

// ── Helpers ─────────────────────────────────────────────────────────────

// Extract a std::string from a SyntaqliteTextSpan.
static std::string SpanText(SyntaqliteParser* p, SyntaqliteTextSpan span) {
  uint32_t len = 0;
  const char* text = syntaqlite_parser_span_expanded_text(p, &span, &len);
  if (!text || len == 0)
    return {};
  return {text, len};
}

// Extract a std::string from an IdentName node ID.
static std::string IdentText(SyntaqliteParser* p, uint32_t ident_id) {
  const auto* ident = SYNTAQLITE_NODE(p, SyntaqliteIdentName, ident_id);
  if (!ident)
    return {};
  return SpanText(p, ident->source);
}

// Case-insensitive string for table name lookups.
static std::string Lower(std::string s) {
  for (auto& c : s)
    c = (char)std::tolower((unsigned char)c);
  return s;
}

// ── Schema ──────────────────────────────────────────────────────────────

// A resolved output column: name + optional source table.
struct Column {
  std::string name;
  std::string source_table;  // which table/view/cte it came from (for *)
};

// Maps lowercase table/view/cte name → list of columns.
using Schema = std::unordered_map<std::string, std::vector<Column>>;

// ── Forward declarations ────────────────────────────────────────────────

static std::vector<Column> ResolveSelectColumns(
    SyntaqliteParser* p,
    const SyntaqliteSelectStmt* select,
    const Schema& schema);

static std::vector<Column> ResolveStmtColumns(SyntaqliteParser* p,
                                              uint32_t stmt_id,
                                              const Schema& schema);

// ── FROM clause: collect table sources ──────────────────────────────────

struct TableSource {
  std::string name;             // the table/view/cte/subquery name
  std::string alias;            // alias if present, else same as name
  std::vector<Column> columns;  // resolved columns for this source
};

static void CollectFromSources(SyntaqliteParser* p,
                               uint32_t from_id,
                               const Schema& schema,
                               std::vector<TableSource>& out) {
  const auto* node = SYNTAQLITE_NODE(p, SyntaqliteTableSource, from_id);
  if (!node)
    return;

  switch (node->tag) {
    case SYNTAQLITE_NODE_TABLE_REF: {
      const auto& ref = node->table_ref;
      std::string name = SpanText(p, ref.table_name);
      std::string alias = syntaqlite_node_is_present(ref.alias)
                              ? IdentText(p, ref.alias)
                              : name;
      std::vector<Column> cols;
      auto it = schema.find(Lower(name));
      if (it != schema.end()) {
        cols = it->second;
        for (auto& c : cols)
          c.source_table = alias;
      }
      out.push_back({name, alias, std::move(cols)});
      break;
    }

    case SYNTAQLITE_NODE_SUBQUERY_TABLE_SOURCE: {
      const auto& sub = node->subquery_table_source;
      std::string alias = IdentText(p, sub.alias);
      auto cols = ResolveStmtColumns(p, sub.select, schema);
      for (auto& c : cols)
        c.source_table = alias;
      out.push_back({"(subquery)", alias, std::move(cols)});
      break;
    }

    case SYNTAQLITE_NODE_JOIN_CLAUSE: {
      const auto& join = node->join_clause;
      CollectFromSources(p, join.left, schema, out);
      CollectFromSources(p, join.right, schema, out);
      break;
    }

    case SYNTAQLITE_NODE_JOIN_PREFIX: {
      const auto& jp = node->join_prefix;
      CollectFromSources(p, jp.source, schema, out);
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
// has a matching column name. For compound expressions, returns empty.
static std::string ExprSource(SyntaqliteParser* p,
                              uint32_t expr_id,
                              const std::vector<TableSource>& sources) {
  const auto* expr = SYNTAQLITE_NODE(p, SyntaqliteExpr, expr_id);
  if (!expr)
    return {};

  if (expr->tag == SYNTAQLITE_NODE_COLUMN_REF) {
    const auto& ref = expr->column_ref;
    if (ref.table.length != 0) {
      // Explicit qualifier: SELECT u.name → source is "u"
      return SpanText(p, ref.table);
    }
    // Bare column: search FROM sources for a match.
    std::string col_name = SpanText(p, ref.column);
    for (const auto& src : sources) {
      for (const auto& c : src.columns) {
        if (strcasecmp(c.name.c_str(), col_name.c_str()) == 0)
          return src.alias;
      }
    }
  }
  return {};
}

// Try to infer a column name from an expression (for unnamed result columns).
static std::string ExprName(SyntaqliteParser* p, uint32_t expr_id) {
  const auto* expr = SYNTAQLITE_NODE(p, SyntaqliteExpr, expr_id);
  if (!expr)
    return "?";

  switch (expr->tag) {
    case SYNTAQLITE_NODE_COLUMN_REF:
      return SpanText(p, expr->column_ref.column);

    case SYNTAQLITE_NODE_LITERAL:
      return SpanText(p, expr->literal.source);

    case SYNTAQLITE_NODE_FUNCTION_CALL:
      return SpanText(p, expr->function_call.func_name) + "(...)";

    case SYNTAQLITE_NODE_AGGREGATE_FUNCTION_CALL:
      return SpanText(p, expr->aggregate_function_call.func_name) + "(...)";

    case SYNTAQLITE_NODE_CAST_EXPR:
      return "CAST(" + ExprName(p, expr->cast_expr.expr) + ")";

    case SYNTAQLITE_NODE_SUBQUERY_EXPR:
      return "(subquery)";

    case SYNTAQLITE_NODE_BINARY_EXPR:
      return "(" + ExprName(p, expr->binary_expr.left) + " op " +
             ExprName(p, expr->binary_expr.right) + ")";

    case SYNTAQLITE_NODE_UNARY_EXPR:
      return "op(" + ExprName(p, expr->unary_expr.operand) + ")";

    default:
      return "<expr>";
  }
}

// ── Resolve SELECT columns ──────────────────────────────────────────────

static std::vector<Column> ResolveSelectColumns(
    SyntaqliteParser* p,
    const SyntaqliteSelectStmt* select,
    const Schema& schema) {
  std::vector<Column> result;

  // Gather FROM sources for * expansion.
  std::vector<TableSource> sources;
  CollectFromSources(p, select->from_clause, schema, sources);

  if (!syntaqlite_node_is_present(select->columns))
    return result;

  SYNTAQLITE_LIST_FOREACH(p, SyntaqliteResultColumn, rc, select->columns) {
    if (rc->flags.bits.star) {
      // table.* or * — alias field holds the table qualifier if present.
      std::string qualifier;
      if (syntaqlite_node_is_present(rc->alias))
        qualifier = IdentText(p, rc->alias);

      if (qualifier.empty()) {
        // SELECT * — expand all tables in FROM order.
        bool expanded = false;
        for (const auto& src : sources) {
          if (!src.columns.empty()) {
            for (const auto& c : src.columns)
              result.push_back(c);
            expanded = true;
          }
        }
        if (!expanded)
          result.push_back({"*", ""});
      } else {
        // SELECT t.* — find matching source.
        bool found = false;
        for (const auto& src : sources) {
          if (strcasecmp(src.alias.c_str(), qualifier.c_str()) == 0 ||
              strcasecmp(src.name.c_str(), qualifier.c_str()) == 0) {
            if (!src.columns.empty()) {
              for (const auto& c : src.columns)
                result.push_back(c);
            } else {
              result.push_back({qualifier + ".*", qualifier});
            }
            found = true;
            break;
          }
        }
        if (!found)
          result.push_back({qualifier + ".*", qualifier});
      }
      continue;
    }

    // Regular column — use alias if present, otherwise infer from expr.
    std::string name;
    if (syntaqlite_node_is_present(rc->alias)) {
      name = IdentText(p, rc->alias);
    } else {
      name = ExprName(p, rc->expr);
    }
    result.push_back({name, ExprSource(p, rc->expr, sources)});
  }

  return result;
}

// ── Resolve any statement's columns ─────────────────────────────────────

static std::vector<Column> ResolveStmtColumns(SyntaqliteParser* p,
                                              uint32_t stmt_id,
                                              const Schema& schema) {
  const auto* stmt = SYNTAQLITE_NODE(p, SyntaqliteStmt, stmt_id);
  if (!stmt)
    return {};

  switch (stmt->tag) {
    case SYNTAQLITE_NODE_SELECT_STMT:
      return ResolveSelectColumns(p, &stmt->select_stmt, schema);

    case SYNTAQLITE_NODE_WITH_CLAUSE: {
      const auto& with = stmt->with_clause;
      Schema local = schema;

      SYNTAQLITE_LIST_FOREACH(p, SyntaqliteCteDefinition, cte, with.ctes) {
        std::string cte_name = Lower(SpanText(p, cte->cte_name));

        if (syntaqlite_node_is_present(cte->columns)) {
          // CTE has explicit column names.
          std::vector<Column> cols;
          SYNTAQLITE_LIST_FOREACH(p, SyntaqliteColumnRef, col_ref,
                                  cte->columns) {
            cols.push_back({SpanText(p, col_ref->column), cte_name});
          }
          local[cte_name] = std::move(cols);
        } else {
          // Resolve from the CTE's SELECT body.
          auto cols = ResolveStmtColumns(p, cte->select, local);
          for (auto& c : cols)
            c.source_table = cte_name;
          local[cte_name] = std::move(cols);
        }
      }

      return ResolveStmtColumns(p, with.select, local);
    }

    case SYNTAQLITE_NODE_COMPOUND_SELECT:
      return ResolveStmtColumns(p, stmt->compound_select.left, schema);

    case SYNTAQLITE_NODE_VALUES_CLAUSE:
      return {};

    default:
      return {};
  }
}

// ── Process a top-level statement ───────────────────────────────────────

static int ProcessStatement(SyntaqliteParser* p,
                            uint32_t root_id,
                            Schema& schema) {
  const auto* stmt = SYNTAQLITE_NODE(p, SyntaqliteStmt, root_id);
  if (!stmt)
    return 0;

  switch (stmt->tag) {
    case SYNTAQLITE_NODE_CREATE_TABLE_STMT: {
      const auto& ct = stmt->create_table_stmt;
      std::string table_name = Lower(SpanText(p, ct.table_name));

      if (syntaqlite_node_is_present(ct.columns)) {
        std::vector<Column> cols;
        SYNTAQLITE_LIST_FOREACH(p, SyntaqliteColumnDef, col_def, ct.columns) {
          cols.push_back({IdentText(p, col_def->column_name), table_name});
        }
        schema[table_name] = std::move(cols);
        printf("registered table '%s'\n", table_name.c_str());
      } else if (syntaqlite_node_is_present(ct.as_select)) {
        auto cols = ResolveStmtColumns(p, ct.as_select, schema);
        for (auto& c : cols)
          c.source_table = table_name;
        schema[table_name] = std::move(cols);
        printf("registered table '%s' (from SELECT)\n", table_name.c_str());
      }
      return 0;
    }

    case SYNTAQLITE_NODE_CREATE_VIEW_STMT: {
      const auto& cv = stmt->create_view_stmt;
      std::string view_name = Lower(SpanText(p, cv.view_name));

      if (syntaqlite_node_is_present(cv.column_names)) {
        std::vector<Column> cols;
        SYNTAQLITE_LIST_FOREACH(p, SyntaqliteColumnRef, col_ref,
                                cv.column_names) {
          cols.push_back({SpanText(p, col_ref->column), view_name});
        }
        schema[view_name] = std::move(cols);
      } else {
        auto cols = ResolveStmtColumns(p, cv.select, schema);
        for (auto& c : cols)
          c.source_table = view_name;
        schema[view_name] = std::move(cols);
      }
      printf("registered view '%s'\n", view_name.c_str());
      return 0;
    }

    case SYNTAQLITE_NODE_SELECT_STMT:
    case SYNTAQLITE_NODE_WITH_CLAUSE:
    case SYNTAQLITE_NODE_COMPOUND_SELECT: {
      auto cols = ResolveStmtColumns(p, root_id, schema);
      printf("output columns (%zu):\n", cols.size());
      for (size_t i = 0; i < cols.size(); i++) {
        printf("  [%zu] %s", i + 1, cols[i].name.c_str());
        if (!cols[i].source_table.empty())
          printf("  (from %s)", cols[i].source_table.c_str());
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
  const char* sql = nullptr;

  if (argc >= 2) {
    sql = argv[1];
  } else {
    static char buf[64 * 1024];
    size_t n = fread(buf, 1, sizeof(buf) - 1, stdin);
    buf[n] = '\0';
    sql = buf;
  }

  SyntaqliteParser* p = syntaqlite_parser_create(nullptr);
  syntaqlite_parser_reset(p, sql, (uint32_t)strlen(sql));

  Schema schema;
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
    ProcessStatement(p, root, schema);
  }

  if (stmt_num == 0) {
    fprintf(stderr, "error: no SQL statement provided\n");
    syntaqlite_parser_destroy(p);
    return 1;
  }

  syntaqlite_parser_destroy(p);
  return 0;
}
