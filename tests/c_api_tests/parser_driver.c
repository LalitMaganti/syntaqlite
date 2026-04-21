// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Line-protocol driver for the parser C API integration tests.
//
// Unlike test_ast.c (which is compiled against a dialect amalgamation),
// this driver links against libsyntaqlite.a and targets the parser
// surface beyond single-shot AST dumps: configuration flags, explicit
// parse_one stepping, tokens/comments/extents dumps, and error info.
//
// Each verb emits one status line on stdout. Dump verbs follow their
// status line with a block terminated by a `.` line.
//
// Verbs:
//   create                Create a SQLite parser with default mem.
//   destroy               Destroy the active handle.
//   collect_tokens 0|1    Configure (must be before first reset).
//   collect_extents 0|1   Configure (must be before first reset).
//   reset                 Bind multi-line source (until `.`).
//   parse_one             Call next() once; prints status + root.
//   node_count            Prints arena node count.
//   parser_text           Prints statement text range + first N bytes.
//   full_text             Prints full source length + slice.
//   dump_root             dump_node at root with indent 0.
//   dump_tokens           All tokens for current statement.
//   dump_comments         All comments for current statement.
//   token_comments <idx>  Leading + trailing comments for token idx.
//   node_text <id>        Authored text slice for node (needs extents).
//   error_info            error_msg/off/len/recovery_root dump.
//   macro_fallback 0|1    Configure (must be before first reset).
//   dump_macros           All macro rewrites with parent_buffer + args.

// Needed for strtok_r under glibc when compiling with -std=c11.
#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "syntaqlite/parser.h"
#include "syntaqlite/types.h"

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

static int tokenize(char* s, char** argv, int max) {
  int n = 0;
  char* save = NULL;
  for (char* tok = strtok_r(s, " \t", &save); tok && n < max;
       tok = strtok_r(NULL, " \t", &save)) {
    argv[n++] = tok;
  }
  return n;
}

static const char* cfg_rc_str(int32_t rc) {
  switch (rc) {
    case SYNTAQLITE_OK:               return "ok";
    case SYNTAQLITE_ERR_ALREADY_USED: return "err already_used";
    default:                          return "err unknown";
  }
}

static const char* parse_rc_str(int32_t rc) {
  switch (rc) {
    case SYNTAQLITE_PARSE_DONE:  return "done";
    case SYNTAQLITE_PARSE_OK:    return "ok";
    case SYNTAQLITE_PARSE_ERROR: return "error";
    default:                     return "?";
  }
}

static void print_comment(const SyntaqliteComment* c, const char* prefix, uint32_t i) {
  const char* side = c->side == SYNQ_COMMENT_LEADING ? "leading" : "trailing";
  const char* kind = c->kind == 0 ? "line" : "block";
  printf("%s[%u] side=%s kind=%s off=%u len=%u token_idx=%u\n",
         prefix, i, side, kind, c->offset, c->length, c->token_idx);
}

int main(void) {
  SyntaqliteParser* p = NULL;
  int32_t last_parse_rc = SYNTAQLITE_PARSE_DONE;

  while (fgets(g_line, sizeof(g_line), stdin)) {
    chomp(g_line);
    if (g_line[0] == '#' || g_line[0] == '\0') continue;

    char* argv[8];
    int argc = tokenize(g_line, argv, 8);
    if (argc == 0) continue;
    const char* verb = argv[0];

    if (strcmp(verb, "create") == 0) {
      if (p) syntaqlite_parser_destroy(p);
      p = syntaqlite_parser_create(NULL);
      printf("create %s\n", p ? "ok" : "err null");
    } else if (strcmp(verb, "destroy") == 0) {
      syntaqlite_parser_destroy(p);
      p = NULL;
      printf("destroy ok\n");
    } else if (strcmp(verb, "reset") == 0) {
      int n = read_block(g_body, sizeof(g_body));
      if (n < 0) { printf("reset err no_terminator\n"); break; }
      if (!p) { printf("reset err no_handle\n"); continue; }
      syntaqlite_parser_reset(p, g_body, (SyntaqliteDocLen)n);
      printf("reset ok len=%d\n", n);
    } else if (!p) {
      printf("%s err no_handle\n", verb);
    } else if (strcmp(verb, "collect_tokens") == 0) {
      if (argc < 2) { printf("collect_tokens err bad_arg\n"); continue; }
      int32_t rc = syntaqlite_parser_set_collect_tokens(
          p, (uint32_t)strtoul(argv[1], NULL, 10));
      printf("collect_tokens %s\n", cfg_rc_str(rc));
    } else if (strcmp(verb, "collect_extents") == 0) {
      if (argc < 2) { printf("collect_extents err bad_arg\n"); continue; }
      int32_t rc = syntaqlite_parser_set_collect_node_extents(
          p, (uint32_t)strtoul(argv[1], NULL, 10));
      printf("collect_extents %s\n", cfg_rc_str(rc));
    } else if (strcmp(verb, "parse_one") == 0) {
      last_parse_rc = syntaqlite_parser_next(p);
      uint32_t root = syntaqlite_result_root(p);
      uint32_t recovery = syntaqlite_result_recovery_root(p);
      printf("parse_one %s root=%u recovery=%u\n",
             parse_rc_str(last_parse_rc),
             root == SYNTAQLITE_NULL_NODE ? 0 : root,
             recovery == SYNTAQLITE_NULL_NODE ? 0 : recovery);
    } else if (strcmp(verb, "node_count") == 0) {
      printf("node_count %u\n", syntaqlite_parser_node_count(p));
    } else if (strcmp(verb, "parser_text") == 0) {
      SyntaqliteDocOffset off = 0;
      SyntaqliteStmtLen len = 0;
      const char* t = syntaqlite_parser_text(p, &off, &len);
      if (!t) {
        printf("parser_text none\n");
      } else {
        printf("parser_text off=%u len=%u\n", off, len);
        fwrite(t, 1, len, stdout);
        if (len == 0 || t[len - 1] != '\n') fputc('\n', stdout);
        printf(".\n");
      }
    } else if (strcmp(verb, "full_text") == 0) {
      SyntaqliteDocLen len = 0;
      const char* t = syntaqlite_parser_full_text(p, &len);
      if (!t) {
        printf("full_text none\n");
      } else {
        printf("full_text len=%u\n", len);
        fwrite(t, 1, len, stdout);
        if (len == 0 || t[len - 1] != '\n') fputc('\n', stdout);
        printf(".\n");
      }
    } else if (strcmp(verb, "dump_root") == 0) {
      uint32_t root = syntaqlite_result_root(p);
      if (root == SYNTAQLITE_NULL_NODE) {
        printf("dump_root none\n");
      } else {
        char* s = syntaqlite_dump_node(p, root, 0);
        printf("dump_root ok\n");
        if (s) { fputs(s, stdout); free(s); }
        printf(".\n");
      }
    } else if (strcmp(verb, "dump_tokens") == 0) {
      uint32_t count = 0;
      const SyntaqliteParserToken* toks = syntaqlite_result_tokens(p, &count);
      printf("tokens count=%u\n", count);
      for (uint32_t i = 0; i < count; i++) {
        printf("tok[%u] type=%u off=%u len=%u flags=%u layer=%u\n",
               i, toks[i].type, toks[i].offset, toks[i].length,
               toks[i].flags, toks[i]._layer_id);
      }
      printf(".\n");
    } else if (strcmp(verb, "dump_comments") == 0) {
      uint32_t count = 0;
      const SyntaqliteComment* cs = syntaqlite_result_comments(p, &count);
      printf("comments count=%u\n", count);
      for (uint32_t i = 0; i < count; i++) print_comment(&cs[i], "com", i);
      printf(".\n");
    } else if (strcmp(verb, "token_comments") == 0) {
      if (argc < 2) { printf("token_comments err bad_arg\n"); continue; }
      SyntaqliteTokenIdx idx = (SyntaqliteTokenIdx)strtoul(argv[1], NULL, 10);
      uint32_t lead = 0, trail = 0;
      const SyntaqliteComment* lc = syntaqlite_token_leading_comments(p, idx, &lead);
      const SyntaqliteComment* tc = syntaqlite_token_trailing_comments(p, idx, &trail);
      printf("token_comments idx=%u leading=%u trailing=%u\n", idx, lead, trail);
      for (uint32_t i = 0; i < lead; i++) print_comment(&lc[i], "lead", i);
      for (uint32_t i = 0; i < trail; i++) print_comment(&tc[i], "trail", i);
      printf(".\n");
    } else if (strcmp(verb, "node_text") == 0) {
      if (argc < 2) { printf("node_text err bad_arg\n"); continue; }
      uint32_t id = (uint32_t)strtoul(argv[1], NULL, 10);
      uint32_t len = 0, off = 0;
      const char* t = syntaqlite_parser_node_text(p, id, &len, &off);
      if (!t) {
        printf("node_text none\n");
      } else {
        printf("node_text id=%u off=%u len=%u\n", id, off, len);
        fwrite(t, 1, len, stdout);
        if (len == 0 || t[len - 1] != '\n') fputc('\n', stdout);
        printf(".\n");
      }
    } else if (strcmp(verb, "macro_fallback") == 0) {
      if (argc < 2) { printf("macro_fallback err bad_arg\n"); continue; }
      int32_t rc = syntaqlite_parser_set_macro_fallback(
          p, (uint32_t)strtoul(argv[1], NULL, 10));
      printf("macro_fallback %s\n", cfg_rc_str(rc));
    } else if (strcmp(verb, "dump_macros") == 0) {
      uint32_t count = syntaqlite_result_macro_count(p);
      printf("macros count=%u\n", count);
      for (uint32_t i = 0; i < count; i++) {
        SyntaqliteMacroRewrite r = syntaqlite_result_macro_rewrite_at(p, i);
        const char* parent_str =
            r.parent_idx == SYNTAQLITE_MACRO_PARENT_SOURCE ? "source" : "idx";
        if (r.parent_idx == SYNTAQLITE_MACRO_PARENT_SOURCE) {
          printf(
              "mac[%u] parent=%s call_off=%u call_len=%u is_fallback=%u "
              "name=\"%.*s\"\n",
              i, parent_str, r.call_offset, r.call_length, r.is_fallback,
              (int)r.name_len, r.name ? r.name : "");
        } else {
          printf(
              "mac[%u] parent=%s(%u) call_off=%u call_len=%u is_fallback=%u "
              "name=\"%.*s\"\n",
              i, parent_str, r.parent_idx, r.call_offset, r.call_length,
              r.is_fallback, (int)r.name_len, r.name ? r.name : "");
        }
        // Slice call text via parent_buffer to exercise the
        // self-resolving API: a C consumer can get call text without
        // walking the parent chain.
        if (r.parent_buffer && r.call_offset + r.call_length <= r.parent_buffer_len) {
          printf("  call_text=\"%.*s\"\n",
                 (int)r.call_length, r.parent_buffer + r.call_offset);
        }
        uint32_t acount = syntaqlite_macro_rewrite_arg_count(p, i);
        printf("  args count=%u\n", acount);
        for (uint32_t j = 0; j < acount; j++) {
          SyntaqliteMacroCallArg a = syntaqlite_macro_rewrite_arg_at(p, i, j);
          printf("    arg[%u] off=%u len=%u text=\"%.*s\"\n",
                 j, a.offset, a.length, (int)a.length,
                 r.parent_buffer + a.offset);
        }
      }
      printf(".\n");
    } else if (strcmp(verb, "error_info") == 0) {
      const char* msg = syntaqlite_result_error_msg(p);
      SyntaqliteStmtOffset off = syntaqlite_result_error_offset(p);
      SyntaqliteStmtLen len = syntaqlite_result_error_length(p);
      uint32_t recovery = syntaqlite_result_recovery_root(p);
      printf("error_info msg=\"%s\" off=%u len=%u recovery=%u\n",
             msg ? msg : "",
             off == 0xFFFFFFFFu ? 0xFFFFFFFFu : off,
             len,
             recovery == SYNTAQLITE_NULL_NODE ? 0 : recovery);
    } else {
      printf("error unknown_verb %s\n", verb);
    }
  }

  if (p) syntaqlite_parser_destroy(p);
  (void)last_parse_rc;
  return 0;
}
