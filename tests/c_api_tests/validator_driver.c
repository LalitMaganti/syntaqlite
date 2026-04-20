// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Line-protocol driver for the validator C API integration tests.
//
// Reads verbs from stdin, one per line. SQL-ish bodies for `analyze`
// and `load_ddl` follow the verb line and are terminated by a line
// containing only `.`. Each verb produces one status line on stdout,
// followed for "dump" verbs by a multi-line block terminated by a `.`
// line. Blank lines and lines starting with `#` are ignored.
//
// Verbs:
//   create                        Create a SQLite validator.
//   destroy                       Destroy the active handle.
//   mode document|execute         Set analysis mode.
//   strict_schema 0|1             Toggle strict-schema.
//   check_level <name> <level>    level = allow|warn|deny.
//   suggestion_threshold <N>
//   add_table <name> [cols]       cols = "c1,c2,..." or "-" (unknown).
//   add_view  <name> [cols]
//   add_function <name> <scalar|aggregate|window> <exact|at_least|any> <N>
//   add_table_function <name> <exact|at_least|any> <N> [cols|-]
//   load_ddl                      Multi-line DDL block until `.`.
//   reset_catalog
//   analyze                       Multi-line SQL block until `.`.
//   dump_diagnostics              All aggregate diagnostics.
//   render [file]                 Render diagnostics as text.
//   stmt_count
//   stmt_diagnostics <idx>
//   dump_relations                Per-statement relations for all stmts.
//   dump_physical_tables
//   dump_lineage
//   dump_defined_relations
//
// See python/dev/integration_tests/suites/c_api.py for the harness.

// Needed for strtok_r under glibc when compiling with -std=c11.
#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "syntaqlite/validation.h"

static char g_line[64 * 1024];
static char g_body[256 * 1024];

static void chomp(char* s) {
  size_t n = strlen(s);
  while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) s[--n] = '\0';
}

static int read_block(char* out, size_t cap) {
  size_t used = 0;
  while (fgets(g_line, sizeof(g_line), stdin)) {
    chomp(g_line);
    if (strcmp(g_line, ".") == 0) {
      if (used > 0) used--;  // drop trailing newline
      out[used] = '\0';
      return (int)used;
    }
    size_t len = strlen(g_line);
    if (used + len + 2 > cap) return -1;
    memcpy(out + used, g_line, len);
    used += len;
    out[used++] = '\n';
  }
  return -1;
}

static const char* sev_str(uint32_t sev) {
  switch (sev) {
    case SYNTAQLITE_SEVERITY_ERROR:   return "ERROR";
    case SYNTAQLITE_SEVERITY_WARNING: return "WARNING";
    case SYNTAQLITE_SEVERITY_INFO:    return "INFO";
    case SYNTAQLITE_SEVERITY_HINT:    return "HINT";
    default:                          return "?";
  }
}

static const char* code_str(uint32_t code) {
  switch (code) {
    case SYNTAQLITE_DIAG_PARSE_ERROR:               return "PARSE_ERROR";
    case SYNTAQLITE_DIAG_UNKNOWN_TABLE:             return "UNKNOWN_TABLE";
    case SYNTAQLITE_DIAG_UNKNOWN_COLUMN:            return "UNKNOWN_COLUMN";
    case SYNTAQLITE_DIAG_UNKNOWN_FUNCTION:          return "UNKNOWN_FUNCTION";
    case SYNTAQLITE_DIAG_UNKNOWN_MODULE:            return "UNKNOWN_MODULE";
    case SYNTAQLITE_DIAG_FUNCTION_ARITY:            return "FUNCTION_ARITY";
    case SYNTAQLITE_DIAG_CTE_COLUMN_COUNT_MISMATCH: return "CTE_COLUMN_COUNT_MISMATCH";
    default:                                         return "?";
  }
}

// Parse "a,b,c" into an owned char*[]. Returns count; stores into *out_argv
// (a malloc'd array of malloc'd strings). "-" or "" → count=0, argv=NULL.
static uint32_t split_csv(const char* s, char*** out_argv) {
  *out_argv = NULL;
  if (!s || s[0] == '\0' || (s[0] == '-' && s[1] == '\0')) return 0;
  uint32_t count = 1;
  for (const char* p = s; *p; p++) if (*p == ',') count++;
  char** argv = (char**)malloc(count * sizeof(char*));
  char* buf = strdup(s);
  uint32_t i = 0;
  char* save = NULL;
  for (char* tok = strtok_r(buf, ",", &save); tok; tok = strtok_r(NULL, ",", &save)) {
    argv[i++] = strdup(tok);
  }
  free(buf);
  *out_argv = argv;
  return i;
}

static void free_argv(char** argv, uint32_t count) {
  if (!argv) return;
  for (uint32_t i = 0; i < count; i++) free(argv[i]);
  free(argv);
}

// Tokenize on whitespace. Returns argv (static-local). Safe only until next call.
static int tokenize(char* s, char** argv, int max) {
  int n = 0;
  char* save = NULL;
  for (char* tok = strtok_r(s, " \t", &save); tok && n < max;
       tok = strtok_r(NULL, " \t", &save)) {
    argv[n++] = tok;
  }
  return n;
}

static int parse_mode(const char* s, SyntaqliteAnalysisMode* out) {
  if (strcmp(s, "document") == 0) { *out = SYNTAQLITE_MODE_DOCUMENT; return 0; }
  if (strcmp(s, "execute")  == 0) { *out = SYNTAQLITE_MODE_EXECUTE;  return 0; }
  return -1;
}

static int parse_level(const char* s, SyntaqliteCheckLevel* out) {
  if (strcmp(s, "allow") == 0) { *out = SYNTAQLITE_CHECK_ALLOW; return 0; }
  if (strcmp(s, "warn")  == 0) { *out = SYNTAQLITE_CHECK_WARN;  return 0; }
  if (strcmp(s, "deny")  == 0) { *out = SYNTAQLITE_CHECK_DENY;  return 0; }
  return -1;
}

static int parse_category(const char* s, SyntaqliteFunctionCategory* out) {
  if (strcmp(s, "scalar")    == 0) { *out = SYNTAQLITE_FUNCTION_SCALAR;    return 0; }
  if (strcmp(s, "aggregate") == 0) { *out = SYNTAQLITE_FUNCTION_AGGREGATE; return 0; }
  if (strcmp(s, "window")    == 0) { *out = SYNTAQLITE_FUNCTION_WINDOW;    return 0; }
  return -1;
}

static int parse_arity(const char* s, SyntaqliteAritySpecKind* out) {
  if (strcmp(s, "exact")    == 0) { *out = SYNTAQLITE_ARITY_EXACT;    return 0; }
  if (strcmp(s, "at_least") == 0) { *out = SYNTAQLITE_ARITY_AT_LEAST; return 0; }
  if (strcmp(s, "any")      == 0) { *out = SYNTAQLITE_ARITY_ANY;      return 0; }
  return -1;
}

static void print_diag(const SyntaqliteDiagnostic* d, const char* prefix, uint32_t i) {
  printf("%s[%u] sev=%s code=%s off=%u end=%u msg=\"%s\"\n",
         prefix, i, sev_str(d->severity), code_str(d->kind_code),
         d->start_offset, d->end_offset,
         d->message ? d->message : "");
}

static void add_relation_one(SyntaqliteValidator* v, int is_view,
                              const char* name, const char* cols_csv) {
  char** cols = NULL;
  uint32_t n = split_csv(cols_csv, &cols);
  SyntaqliteRelationDef def;
  def.name = name;
  def.columns = (const char* const*)cols;
  def.column_count = n;
  if (cols_csv && cols_csv[0] == '\0') def.columns = NULL;  // treat "" as unknown cols
  if (is_view) syntaqlite_validator_add_views(v, &def, 1);
  else         syntaqlite_validator_add_tables(v, &def, 1);
  free_argv(cols, n);
}

int main(void) {
  SyntaqliteValidator* v = NULL;

  while (fgets(g_line, sizeof(g_line), stdin)) {
    chomp(g_line);
    if (g_line[0] == '#' || g_line[0] == '\0') continue;

    char* argv[8];
    int argc = tokenize(g_line, argv, 8);
    if (argc == 0) continue;
    const char* verb = argv[0];

    if (strcmp(verb, "create") == 0) {
      if (v) syntaqlite_validator_destroy(v);
      v = syntaqlite_validator_create_sqlite();
      printf("create %s\n", v ? "ok" : "err null");
    } else if (strcmp(verb, "destroy") == 0) {
      syntaqlite_validator_destroy(v);
      v = NULL;
      printf("destroy ok\n");
    } else if (strcmp(verb, "load_ddl") == 0) {
      int n = read_block(g_body, sizeof(g_body));
      if (n < 0) { printf("load_ddl err no_terminator\n"); break; }
      if (!v) { printf("load_ddl err no_handle\n"); continue; }
      uint32_t errs = syntaqlite_validator_load_schema_ddl(v, g_body, (uint32_t)n);
      printf("load_ddl ok errs=%u\n", errs);
    } else if (strcmp(verb, "analyze") == 0) {
      int n = read_block(g_body, sizeof(g_body));
      if (n < 0) { printf("analyze err no_terminator\n"); break; }
      if (!v) { printf("analyze err no_handle\n"); continue; }
      uint32_t diags = syntaqlite_validator_analyze(v, g_body, (uint32_t)n);
      uint32_t stmts = syntaqlite_validator_statement_count(v);
      printf("analyze ok stmts=%u diags=%u\n", stmts, diags);
    } else if (!v) {
      printf("%s err no_handle\n", verb);
    } else if (strcmp(verb, "mode") == 0) {
      SyntaqliteAnalysisMode m;
      if (argc < 2 || parse_mode(argv[1], &m) != 0) {
        printf("mode err bad_arg\n");
      } else {
        syntaqlite_validator_set_mode(v, m);
        printf("mode ok\n");
      }
    } else if (strcmp(verb, "strict_schema") == 0) {
      if (argc < 2) { printf("strict_schema err bad_arg\n"); continue; }
      syntaqlite_validator_set_strict_schema(v, (uint32_t)strtoul(argv[1], NULL, 10));
      printf("strict_schema ok\n");
    } else if (strcmp(verb, "suggestion_threshold") == 0) {
      if (argc < 2) { printf("suggestion_threshold err bad_arg\n"); continue; }
      syntaqlite_validator_set_suggestion_threshold(v, (uint32_t)strtoul(argv[1], NULL, 10));
      printf("suggestion_threshold ok\n");
    } else if (strcmp(verb, "check_level") == 0) {
      SyntaqliteCheckLevel lv;
      if (argc < 3 || parse_level(argv[2], &lv) != 0) {
        printf("check_level err bad_arg\n");
        continue;
      }
      int32_t rc = syntaqlite_validator_set_check_level(v, argv[1], lv);
      printf("check_level %s\n", rc == 0 ? "ok" : "err unknown");
    } else if (strcmp(verb, "add_table") == 0 || strcmp(verb, "add_view") == 0) {
      if (argc < 2) { printf("%s err bad_arg\n", verb); continue; }
      const char* name = argv[1];
      const char* cols = argc >= 3 ? argv[2] : "-";
      add_relation_one(v, strcmp(verb, "add_view") == 0, name, cols);
      printf("%s ok\n", verb);
    } else if (strcmp(verb, "add_function") == 0) {
      SyntaqliteFunctionCategory cat;
      SyntaqliteAritySpecKind ar;
      if (argc < 5 || parse_category(argv[2], &cat) != 0 || parse_arity(argv[3], &ar) != 0) {
        printf("add_function err bad_arg\n");
        continue;
      }
      syntaqlite_validator_add_function_overload(
          v, argv[1], cat, ar, (uint32_t)strtoul(argv[4], NULL, 10));
      printf("add_function ok\n");
    } else if (strcmp(verb, "add_table_function") == 0) {
      SyntaqliteAritySpecKind ar;
      if (argc < 4 || parse_arity(argv[2], &ar) != 0) {
        printf("add_table_function err bad_arg\n");
        continue;
      }
      const char* cols = argc >= 5 ? argv[4] : "-";
      char** out_cols = NULL;
      uint32_t out_n = split_csv(cols, &out_cols);
      syntaqlite_validator_add_table_function(
          v, argv[1], ar, (uint32_t)strtoul(argv[3], NULL, 10),
          (const char* const*)out_cols, out_n);
      free_argv(out_cols, out_n);
      printf("add_table_function ok\n");
    } else if (strcmp(verb, "reset_catalog") == 0) {
      syntaqlite_validator_reset_catalog(v);
      printf("reset_catalog ok\n");
    } else if (strcmp(verb, "dump_diagnostics") == 0) {
      uint32_t n = syntaqlite_validator_diagnostic_count(v);
      printf("diagnostics count=%u\n", n);
      const SyntaqliteDiagnostic* d = syntaqlite_validator_diagnostics(v);
      for (uint32_t i = 0; i < n; i++) print_diag(&d[i], "diag", i);
      printf(".\n");
    } else if (strcmp(verb, "render") == 0) {
      const char* file = argc >= 2 ? argv[1] : NULL;
      const char* out = syntaqlite_validator_render_diagnostics(v, file);
      printf("render ok\n");
      if (out && out[0]) fputs(out, stdout);
      printf(".\n");
    } else if (strcmp(verb, "stmt_count") == 0) {
      printf("stmt_count %u\n", syntaqlite_validator_statement_count(v));
    } else if (strcmp(verb, "stmt_diagnostics") == 0) {
      if (argc < 2) { printf("stmt_diagnostics err bad_arg\n"); continue; }
      uint32_t idx = (uint32_t)strtoul(argv[1], NULL, 10);
      uint32_t n = syntaqlite_validator_statement_diagnostic_count(v, idx);
      printf("stmt_diagnostics idx=%u count=%u\n", idx, n);
      const SyntaqliteDiagnostic* d = syntaqlite_validator_statement_diagnostics(v, idx);
      for (uint32_t i = 0; i < n; i++) print_diag(&d[i], "diag", i);
      printf(".\n");
    } else if (strcmp(verb, "dump_relations") == 0) {
      uint32_t stmts = syntaqlite_validator_statement_count(v);
      printf("relations stmts=%u\n", stmts);
      for (uint32_t s = 0; s < stmts; s++) {
        uint32_t n = syntaqlite_validator_statement_relation_count(v, s);
        const SyntaqliteRelationAccess* r = syntaqlite_validator_statement_relations(v, s);
        printf("stmt[%u] count=%u\n", s, n);
        for (uint32_t i = 0; i < n; i++) {
          const char* kind = r[i].kind == SYNTAQLITE_RELATION_VIEW ? "view" : "table";
          printf("  rel[%u] name=%s kind=%s\n", i, r[i].name ? r[i].name : "", kind);
        }
      }
      printf(".\n");
    } else if (strcmp(verb, "dump_physical_tables") == 0) {
      uint32_t stmts = syntaqlite_validator_statement_count(v);
      printf("physical_tables stmts=%u\n", stmts);
      for (uint32_t s = 0; s < stmts; s++) {
        uint32_t n = syntaqlite_validator_statement_physical_table_count(v, s);
        const SyntaqlitePhysicalTableAccess* t = syntaqlite_validator_statement_physical_tables(v, s);
        printf("stmt[%u] count=%u\n", s, n);
        for (uint32_t i = 0; i < n; i++) {
          printf("  tbl[%u] name=%s\n", i, t[i].name ? t[i].name : "");
        }
      }
      printf(".\n");
    } else if (strcmp(verb, "dump_lineage") == 0) {
      uint32_t stmts = syntaqlite_validator_statement_count(v);
      printf("lineage stmts=%u\n", stmts);
      for (uint32_t s = 0; s < stmts; s++) {
        uint32_t n = syntaqlite_validator_statement_column_lineage_count(v, s);
        const SyntaqliteColumnLineage* c = syntaqlite_validator_statement_column_lineage(v, s);
        printf("stmt[%u] count=%u\n", s, n);
        for (uint32_t i = 0; i < n; i++) {
          const char* t = c[i].origin.table;
          const char* col = c[i].origin.column;
          printf("  col[%u] name=%s idx=%u origin=%s.%s\n", i,
                 c[i].name ? c[i].name : "",
                 c[i].index,
                 t ? t : "-", col ? col : "-");
        }
      }
      printf(".\n");
    } else if (strcmp(verb, "dump_defined_relations") == 0) {
      uint32_t stmts = syntaqlite_validator_statement_count(v);
      printf("defined_relations stmts=%u\n", stmts);
      for (uint32_t s = 0; s < stmts; s++) {
        uint32_t n = syntaqlite_validator_statement_defined_relation_count(v, s);
        const SyntaqliteDefinedRelation* r = syntaqlite_validator_statement_defined_relations(v, s);
        printf("stmt[%u] count=%u\n", s, n);
        for (uint32_t i = 0; i < n; i++) {
          const char* kind = r[i].is_view ? "view" : "table";
          printf("  def[%u] name=%s kind=%s\n", i, r[i].name ? r[i].name : "", kind);
        }
      }
      printf(".\n");
    } else {
      printf("error unknown_verb %s\n", verb);
    }
  }

  if (v) syntaqlite_validator_destroy(v);
  return 0;
}
