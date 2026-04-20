// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Line-protocol driver for the formatter C API integration tests.
//
// Reads verbs from stdin, one per line. SQL bodies for the `format` verb
// follow the verb line and are terminated by a line containing only `.`.
// Each verb produces one status line on stdout, optionally followed by an
// output body terminated by a `.` line. Blank lines and lines starting
// with `#` are ignored.
//
// Verbs:
//   create [k=v ...]   Create a SQLite formatter. Options:
//                        line_width, indent_width, semicolons (uint)
//                        keyword_case = upper|lower
//   format             Format the SQL block that follows (until `.`).
//   destroy            Destroy the active handle.
//
// See python/dev/integration_tests/suites/c_api.py for the harness.

// Needed for strtok_r under glibc when compiling with -std=c11.
#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "syntaqlite/formatter.h"

static char g_line[64 * 1024];
static char g_sql[256 * 1024];

static void chomp(char* s) {
  size_t n = strlen(s);
  while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) s[--n] = '\0';
}

static int apply_kv(SyntaqliteFormatConfig* cfg, char* tok) {
  char* eq = strchr(tok, '=');
  if (!eq) return -1;
  *eq = '\0';
  const char* k = tok;
  const char* v = eq + 1;
  if (strcmp(k, "line_width") == 0) {
    cfg->line_width = (uint32_t)strtoul(v, NULL, 10);
  } else if (strcmp(k, "indent_width") == 0) {
    cfg->indent_width = (uint32_t)strtoul(v, NULL, 10);
  } else if (strcmp(k, "semicolons") == 0) {
    cfg->semicolons = (uint32_t)strtoul(v, NULL, 10);
  } else if (strcmp(k, "keyword_case") == 0) {
    if (strcmp(v, "upper") == 0) cfg->keyword_case = SYNTAQLITE_KEYWORD_UPPER;
    else if (strcmp(v, "lower") == 0) cfg->keyword_case = SYNTAQLITE_KEYWORD_LOWER;
    else return -1;
  } else {
    return -1;
  }
  return 0;
}

static int apply_kvs(SyntaqliteFormatConfig* cfg, char* args) {
  char* save = NULL;
  for (char* tok = strtok_r(args, " \t", &save); tok;
       tok = strtok_r(NULL, " \t", &save)) {
    if (apply_kv(cfg, tok) != 0) return -1;
  }
  return 0;
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

int main(void) {
  SyntaqliteFormatter* f = NULL;

  while (fgets(g_line, sizeof(g_line), stdin)) {
    chomp(g_line);
    if (g_line[0] == '#' || g_line[0] == '\0') continue;

    char* verb = g_line;
    char* args = strchr(g_line, ' ');
    if (args) { *args = '\0'; args++; }

    if (strcmp(verb, "create") == 0) {
      SyntaqliteFormatConfig cfg = {
          .line_width = 80,
          .indent_width = 2,
          .keyword_case = SYNTAQLITE_KEYWORD_UPPER,
          .semicolons = 1,
      };
      if (args && apply_kvs(&cfg, args) != 0) {
        printf("create err bad_kv\n");
        continue;
      }
      if (f) syntaqlite_formatter_destroy(f);
      f = syntaqlite_formatter_create_sqlite_with_config(&cfg);
      printf("create %s\n", f ? "ok" : "err null");
    } else if (strcmp(verb, "destroy") == 0) {
      syntaqlite_formatter_destroy(f);
      f = NULL;
      printf("destroy ok\n");
    } else if (strcmp(verb, "format") == 0) {
      int n = read_block(g_sql, sizeof(g_sql));
      if (n < 0) { printf("format err no_terminator\n"); break; }
      if (!f) { printf("format err no_handle\n"); continue; }
      int32_t rc = syntaqlite_formatter_format(f, g_sql, (uint32_t)n);
      if (rc == SYNTAQLITE_FORMAT_OK) {
        const char* out = syntaqlite_formatter_output(f);
        uint32_t out_len = syntaqlite_formatter_output_len(f);
        printf("format ok len=%u\n", (unsigned)out_len);
        if (out && out_len > 0) fwrite(out, 1, out_len, stdout);
        if (out_len == 0 || out[out_len - 1] != '\n') fputc('\n', stdout);
        printf(".\n");
      } else {
        const char* err = syntaqlite_formatter_error_msg(f);
        printf("format err %s\n", err ? err : "unknown");
      }
    } else {
      printf("error unknown_verb %s\n", verb);
    }
  }
  if (f) syntaqlite_formatter_destroy(f);
  return 0;
}
