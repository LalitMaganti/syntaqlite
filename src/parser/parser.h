// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Internal parser struct definition. Include this header to access
// SyntaqliteParser fields directly instead of through accessor functions.

#ifndef SYNTAQLITE_SRC_PARSER_PARSER_H
#define SYNTAQLITE_SRC_PARSER_PARSER_H

#include "src/common/synq_sqlite_defs.h"
#include "src/parser/ast_base.h"
#include "src/parser/token_list.h"

struct SyntaqliteParser {
  const char* source;
  uint32_t length;

  // Tokenizer cursor
  uint32_t pos;
  int last_token_type;

  // Lemon parser
  void* lemon;
  SynqParseContext parseCtx;
  SynqAstContext astCtx;

  // Optional token collection
  SynqTokenList* token_list;
  SynqTokenList token_list_storage;

  // Error state
  int had_error;
  char error_msg[256];

  // EOF tracking
  int finished;
};

#endif  // SYNTAQLITE_SRC_PARSER_PARSER_H
