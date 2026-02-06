// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// Internal parser accessors for test binaries and formatter.
// Not part of the public API - use syntaqlite/parser.h instead.

#ifndef SYNQ_SRC_PARSER_INTERNAL_H
#define SYNQ_SRC_PARSER_INTERNAL_H

#include "syntaqlite/parser.h"
#include "src/ast/ast_base.h"

#ifdef __cplusplus
extern "C" {
#endif

struct SynqTokenList;

// Access the shared AST context (valid for lifetime of parser).
SynqAstContext *syntaqlite_parser_ast_context(SyntaqliteParser *p);

// Convenience: access the arena directly.
SynqArena *syntaqlite_parser_arena(SyntaqliteParser *p);

// Access the token list (NULL if collect_tokens was not set).
struct SynqTokenList *syntaqlite_parser_token_list(SyntaqliteParser *p);

#ifdef __cplusplus
}
#endif

#endif // SYNQ_SRC_PARSER_INTERNAL_H
