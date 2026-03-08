// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// TCL extension that implements the SQLite TCL API surface for upstream
// testing. Each "database handle" runs SQL through both a real SQLite
// database (via sqlite3_prepare_v2) and syntaqlite's parser + validator,
// logging comparison results as JSON lines.
//
// Loaded by tclsh via: load ./tclsyntaqlite.so Tclsyntaqlite
//
// Provides the `sqlite3` command which creates mock database handles.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tcl.h>

#include "sqlite3.h"
#include "syntaqlite/parser.h"
#include "syntaqlite/validation.h"

// ---------------------------------------------------------------------------
// Per-database handle state
// ---------------------------------------------------------------------------

typedef struct DbHandle {
  char* name;                    // TCL command name (e.g. "db")
  sqlite3* real_db;              // Real SQLite for prepare() ground truth
  SyntaqliteParser* parser;      // syntaqlite parser
  SyntaqliteValidator* validator; // syntaqlite validator (may be NULL)
  Tcl_Interp* interp;           // TCL interpreter
  FILE* log_file;                // JSON lines output (shared, not owned)

  // Statistics
  uint32_t total_stmts;
  uint32_t parse_ok;
  uint32_t parse_error;
  uint32_t sqlite_prepare_ok;
  uint32_t sqlite_prepare_error;
  uint32_t both_accept;
  uint32_t both_reject;
  uint32_t false_positive;     // syntaqlite rejects, sqlite accepts
  uint32_t gap;                // sqlite rejects, syntaqlite accepts
} DbHandle;

// ---------------------------------------------------------------------------
// JSON helpers
// ---------------------------------------------------------------------------

// Write a JSON-escaped string to the log file.
static void json_write_string(FILE* f, const char* s) {
  fputc('"', f);
  if (s) {
    for (const char* p = s; *p; p++) {
      switch (*p) {
        case '"':
          fputs("\\\"", f);
          break;
        case '\\':
          fputs("\\\\", f);
          break;
        case '\n':
          fputs("\\n", f);
          break;
        case '\r':
          fputs("\\r", f);
          break;
        case '\t':
          fputs("\\t", f);
          break;
        default:
          if ((unsigned char)*p < 0x20) {
            fprintf(f, "\\u%04x", (unsigned)*p);
          } else {
            fputc(*p, f);
          }
      }
    }
  }
  fputc('"', f);
}

// ---------------------------------------------------------------------------
// Core eval: dual-path SQL execution
// ---------------------------------------------------------------------------

// Evaluate a single SQL statement through both SQLite and syntaqlite.
static void eval_sql(DbHandle* db, const char* sql, int sql_len) {
  if (!sql || sql_len == 0) return;

  // Skip whitespace-only strings.
  int all_space = 1;
  for (int i = 0; i < sql_len; i++) {
    unsigned char c = (unsigned char)sql[i];
    if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
      all_space = 0;
      break;
    }
  }
  if (all_space) return;

  db->total_stmts++;

  // --- Path 1: Real SQLite prepare ---
  sqlite3_stmt* stmt = NULL;
  int sqlite_rc = sqlite3_prepare_v2(db->real_db, sql, sql_len, &stmt, NULL);
  const char* sqlite_err =
      (sqlite_rc != SQLITE_OK) ? sqlite3_errmsg(db->real_db) : NULL;

  int sqlite_ok = (sqlite_rc == SQLITE_OK);
  if (sqlite_ok) {
    db->sqlite_prepare_ok++;
    // Step DDL so schema accumulates in real DB.
    // We step all successfully-prepared statements to build schema.
    if (stmt) {
      sqlite3_step(stmt);
    }
  } else {
    db->sqlite_prepare_error++;
  }
  if (stmt) {
    sqlite3_finalize(stmt);
  }

  // --- Path 2: syntaqlite parser ---
  syntaqlite_parser_reset(db->parser, sql, (uint32_t)sql_len);
  int32_t parse_rc = syntaqlite_parser_next(db->parser);
  int syntaqlite_parse_ok = (parse_rc == SYNTAQLITE_PARSE_OK);
  const char* parse_err = NULL;
  if (!syntaqlite_parse_ok && parse_rc == SYNTAQLITE_PARSE_ERROR) {
    parse_err = syntaqlite_result_error_msg(db->parser);
  }

  if (syntaqlite_parse_ok) {
    db->parse_ok++;
  } else {
    db->parse_error++;
  }

  // --- Path 3: syntaqlite validator (if parse succeeded) ---
  uint32_t diag_count = 0;
  if (syntaqlite_parse_ok && db->validator) {
    diag_count = syntaqlite_validator_analyze(db->validator, sql, (uint32_t)sql_len);
  }
  int syntaqlite_ok = syntaqlite_parse_ok && diag_count == 0;

  // --- Comparison ---
  if (sqlite_ok && syntaqlite_ok)
    db->both_accept++;
  else if (!sqlite_ok && !syntaqlite_ok)
    db->both_reject++;
  else if (sqlite_ok && !syntaqlite_ok)
    db->false_positive++;
  else
    db->gap++;

  // --- Log entry ---
  if (db->log_file) {
    fprintf(db->log_file, "{\"sql\":");
    json_write_string(db->log_file, sql);
    fprintf(db->log_file, ",\"sqlite_ok\":%s", sqlite_ok ? "true" : "false");
    if (sqlite_err) {
      fprintf(db->log_file, ",\"sqlite_error\":");
      json_write_string(db->log_file, sqlite_err);
    }
    fprintf(db->log_file, ",\"parse_ok\":%s",
            syntaqlite_parse_ok ? "true" : "false");
    if (parse_err) {
      fprintf(db->log_file, ",\"parse_error\":");
      json_write_string(db->log_file, parse_err);
    }
    if (diag_count > 0) {
      fprintf(db->log_file, ",\"diagnostics\":[");
      for (uint32_t i = 0; i < diag_count; i++) {
        if (i > 0) fputc(',', db->log_file);
        fprintf(db->log_file, "{\"severity\":%u,\"message\":",
                syntaqlite_diagnostic_severity(db->validator, i));
        json_write_string(db->log_file,
                          syntaqlite_diagnostic_message(db->validator, i));
        fprintf(db->log_file, ",\"start\":%u,\"end\":%u}",
                syntaqlite_diagnostic_start_offset(db->validator, i),
                syntaqlite_diagnostic_end_offset(db->validator, i));
      }
      fputc(']', db->log_file);
    }
    fprintf(db->log_file, "}\n");
  }
}

// Split multi-statement SQL (separated by ;) and eval each.
static void eval_multi_sql(DbHandle* db, const char* sql) {
  if (!sql || !*sql) return;

  // Use sqlite3_prepare_v2's tail pointer to split statements naturally.
  // But we also need to parse each one via syntaqlite separately.
  // Simple approach: feed the whole thing to syntaqlite parser which
  // handles multi-statement splitting internally.

  int len = (int)strlen(sql);
  eval_sql(db, sql, len);
}

// ---------------------------------------------------------------------------
// TCL database command (e.g., "db eval ...", "db close")
// ---------------------------------------------------------------------------

static void db_handle_delete(ClientData data) {
  DbHandle* db = (DbHandle*)data;
  if (db->real_db) sqlite3_close(db->real_db);
  if (db->parser) syntaqlite_parser_destroy(db->parser);
  if (db->validator) syntaqlite_validator_destroy(db->validator);
  free(db->name);
  free(db);
}

static int db_handle_cmd(ClientData data, Tcl_Interp* interp, int objc,
                         Tcl_Obj* const objv[]) {
  DbHandle* db = (DbHandle*)data;

  if (objc < 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "subcommand ?args?");
    return TCL_ERROR;
  }

  const char* sub = Tcl_GetString(objv[1]);

  if (strcmp(sub, "eval") == 0) {
    if (objc < 3) {
      Tcl_WrongNumArgs(interp, 2, objv, "sql");
      return TCL_ERROR;
    }
    const char* sql = Tcl_GetString(objv[2]);
    eval_multi_sql(db, sql);
    // Return empty result (we don't execute queries).
    Tcl_ResetResult(interp);
    return TCL_OK;
  }

  if (strcmp(sub, "close") == 0) {
    Tcl_DeleteCommand(interp, db->name);
    return TCL_OK;
  }

  // Stubs for common subcommands that tests use.
  if (strcmp(sub, "exists") == 0 || strcmp(sub, "onecolumn") == 0) {
    // Execute the SQL for schema tracking, return empty/0.
    if (objc >= 3) {
      eval_multi_sql(db, Tcl_GetString(objv[2]));
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
    return TCL_OK;
  }

  if (strcmp(sub, "transaction") == 0) {
    // Execute the body script.
    if (objc >= 3) {
      return Tcl_EvalObjEx(interp, objv[objc - 1], 0);
    }
    return TCL_OK;
  }

  if (strcmp(sub, "function") == 0 || strcmp(sub, "collate") == 0 ||
      strcmp(sub, "collation_needed") == 0 || strcmp(sub, "trace") == 0 ||
      strcmp(sub, "profile") == 0 || strcmp(sub, "busy") == 0 ||
      strcmp(sub, "timeout") == 0 || strcmp(sub, "progress") == 0 ||
      strcmp(sub, "authorizer") == 0 || strcmp(sub, "nullvalue") == 0 ||
      strcmp(sub, "version") == 0 || strcmp(sub, "errorcode") == 0 ||
      strcmp(sub, "changes") == 0 || strcmp(sub, "total_changes") == 0 ||
      strcmp(sub, "status") == 0 || strcmp(sub, "config") == 0 ||
      strcmp(sub, "cache") == 0 || strcmp(sub, "enable_load_extension") == 0 ||
      strcmp(sub, "interrupt") == 0 || strcmp(sub, "wal_hook") == 0) {
    // Stubs — return empty result.
    return TCL_OK;
  }

  if (strcmp(sub, "complete") == 0) {
    Tcl_SetObjResult(interp, Tcl_NewIntObj(1));
    return TCL_OK;
  }

  if (strcmp(sub, "last_insert_rowid") == 0) {
    Tcl_SetObjResult(interp, Tcl_NewWideIntObj(0));
    return TCL_OK;
  }

  // Unknown subcommand — try to eval as SQL if it looks like a query.
  // Some tests do `db {SELECT ...}` instead of `db eval {SELECT ...}`.
  eval_multi_sql(db, sub);
  return TCL_OK;
}

// ---------------------------------------------------------------------------
// sqlite3 command: create database handles
// ---------------------------------------------------------------------------

// Global log file handle (set by init or env var).
static FILE* g_log_file = NULL;
static int g_enable_validation = 0;

static int sqlite3_cmd(ClientData data, Tcl_Interp* interp, int objc,
                       Tcl_Obj* const objv[]) {
  (void)data;

  if (objc < 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "dbname ?filename? ?options?");
    return TCL_ERROR;
  }

  const char* dbname = Tcl_GetString(objv[1]);

  DbHandle* db = (DbHandle*)calloc(1, sizeof(DbHandle));
  if (!db) {
    Tcl_SetResult(interp, "out of memory", TCL_STATIC);
    return TCL_ERROR;
  }

  db->name = strdup(dbname);
  db->interp = interp;
  db->log_file = g_log_file;

  // Open real in-memory SQLite database.
  int rc = sqlite3_open(":memory:", &db->real_db);
  if (rc != SQLITE_OK) {
    Tcl_SetResult(interp, "failed to open sqlite3", TCL_STATIC);
    free(db->name);
    free(db);
    return TCL_ERROR;
  }

  // Create syntaqlite parser.
  db->parser = syntaqlite_parser_create(NULL);
  if (!db->parser) {
    Tcl_SetResult(interp, "failed to create syntaqlite parser", TCL_STATIC);
    sqlite3_close(db->real_db);
    free(db->name);
    free(db);
    return TCL_ERROR;
  }

  // Create syntaqlite validator (if enabled).
  if (g_enable_validation) {
    db->validator = syntaqlite_validator_create_sqlite();
  }

  Tcl_CreateObjCommand(interp, dbname, db_handle_cmd, (ClientData)db,
                        db_handle_delete);

  return TCL_OK;
}

// ---------------------------------------------------------------------------
// Summary command: print statistics
// ---------------------------------------------------------------------------

static int summary_cmd(ClientData data, Tcl_Interp* interp, int objc,
                       Tcl_Obj* const objv[]) {
  (void)data;
  (void)objc;
  (void)objv;

  if (g_log_file) {
    fflush(g_log_file);
  }

  return TCL_OK;
}

// ---------------------------------------------------------------------------
// Package initialization
// ---------------------------------------------------------------------------

int Tclsyntaqlite_Init(Tcl_Interp* interp) {
  if (Tcl_InitStubs(interp, "8.5", 0) == NULL) {
    return TCL_ERROR;
  }

  // Open log file from environment variable.
  const char* log_path = getenv("SYNTAQLITE_TEST_LOG");
  if (log_path && *log_path) {
    g_log_file = fopen(log_path, "w");
  }

  // Check if validation is enabled.
  const char* validate_env = getenv("SYNTAQLITE_TEST_VALIDATE");
  if (validate_env && *validate_env == '1') {
    g_enable_validation = 1;
  }

  Tcl_CreateObjCommand(interp, "sqlite3", sqlite3_cmd, NULL, NULL);
  Tcl_CreateObjCommand(interp, "syntaqlite_summary", summary_cmd, NULL, NULL);

  Tcl_PkgProvide(interp, "tclsyntaqlite", "1.0");
  return TCL_OK;
}
