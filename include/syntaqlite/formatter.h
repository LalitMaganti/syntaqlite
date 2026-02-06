// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// SQL formatter: pretty-prints parsed SQL statements.
//
// Usage:
//   SyntaqliteParserConfig config = {.collect_tokens = 1};
//   SyntaqliteParser *p = syntaqlite_parser_create(&config);
//   syntaqlite_parser_reset(p, sql, len);
//   SyntaqliteParseResult result = syntaqlite_parser_next(p);
//   SyntaqliteFormatOptions opts = SYNTAQLITE_FORMAT_OPTIONS_DEFAULT;
//   char *formatted = syntaqlite_format(p, result.root, &opts);
//   // use formatted...
//   free(formatted);

#ifndef SYNTAQLITE_FORMATTER_H
#define SYNTAQLITE_FORMATTER_H

#include <stdint.h>

#include "syntaqlite/parser.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SyntaqliteFormatOptions {
    uint32_t target_width;   // default 80
    uint32_t indent_width;   // default 2
} SyntaqliteFormatOptions;

#define SYNTAQLITE_FORMAT_OPTIONS_DEFAULT {80, 2}

// Format a parsed statement to pretty-printed SQL.
// Parser must have been created with collect_tokens=1 for comment preservation.
// Returns malloc'd string (caller frees), or NULL on error.
char *syntaqlite_format(SyntaqliteParser *parser, uint32_t root_id,
                        const SyntaqliteFormatOptions *options);

// Debug: print document IR tree instead of formatted output.
// Returns malloc'd string (caller frees), or NULL on error.
char *syntaqlite_format_debug_ir(SyntaqliteParser *parser, uint32_t root_id,
                                 const SyntaqliteFormatOptions *options);

#ifdef __cplusplus
}
#endif

#endif // SYNTAQLITE_FORMATTER_H
