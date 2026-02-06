// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

#include "syntaqlite/parser.h"

#include "src/ast/ast_nodes_gen.h"
#include "src/common/synq_sqlite_defs.h"
#include "syntaqlite/sqlite_tokens_gen.h"
#include "src/fmt/token_list.h"

#include <stdlib.h>
#include <string.h>

// External parser functions (defined in sqlite_parse_gen.c)
void *synq_sqlite3ParserAlloc(void *(*mallocProc)(size_t),
                              SynqParseContext *pCtx);
void synq_sqlite3Parser(void *parser, int tokenType, SynqToken token,
                        SynqParseContext *pCtx);
void synq_sqlite3ParserFree(void *parser, void (*freeProc)(void *));

// External tokenizer function (defined in sqlite_tokenize_gen.c)
i64 synq_sqlite3GetToken(const unsigned char *z, int *tokenType);

struct SyntaqliteParser {
    const char *source;
    uint32_t length;

    // Tokenizer cursor
    uint32_t pos;
    int last_token_type;

    // Lemon parser
    void *lemon;
    SynqParseContext parseCtx;
    SynqAstContext astCtx;

    // Optional token collection
    SynqTokenList *token_list;
    SynqTokenList token_list_storage;

    // Error state
    int had_error;
    char error_msg[256];

    // EOF tracking
    int finished;
};

static void on_syntax_error(SynqParseContext *ctx) {
    SyntaqliteParser *p = (SyntaqliteParser *)ctx->userData;
    p->had_error = 1;
}

static void on_stack_overflow(SynqParseContext *ctx) {
    SyntaqliteParser *p = (SyntaqliteParser *)ctx->userData;
    p->had_error = 1;
    snprintf(p->error_msg, sizeof(p->error_msg), "Parser stack overflow");
}

SyntaqliteParser *syntaqlite_parser_create(const char *source, uint32_t len,
                                           const SyntaqliteParserConfig *config) {
    SyntaqliteParser *p = (SyntaqliteParser *)calloc(1, sizeof(SyntaqliteParser));
    if (!p) return NULL;

    p->source = source;
    p->length = len;
    p->pos = 0;
    p->last_token_type = 0;
    p->had_error = 0;
    p->finished = 0;
    p->error_msg[0] = '\0';

    // Initialize AST context
    synq_ast_context_init(&p->astCtx, source, len);

    // Token collection
    if (config && config->collect_tokens) {
        synq_vec_init(&p->token_list_storage);
        p->token_list = &p->token_list_storage;
    } else {
        p->token_list = NULL;
    }

    // Initialize parse context
    p->parseCtx.userData = p;
    p->parseCtx.onSyntaxError = on_syntax_error;
    p->parseCtx.onStackOverflow = on_stack_overflow;
    p->parseCtx.astCtx = &p->astCtx;
    p->parseCtx.zSql = source;
    p->parseCtx.root = SYNQ_NULL_NODE;
    p->parseCtx.token_list = p->token_list;
    p->parseCtx.stmt_completed = 0;

    // Create Lemon parser
    p->lemon = synq_sqlite3ParserAlloc(malloc, &p->parseCtx);
    if (!p->lemon) {
        synq_ast_context_cleanup(&p->astCtx);
        if (p->token_list) synq_vec_free(&p->token_list_storage);
        free(p);
        return NULL;
    }

    return p;
}

SyntaqliteParseResult syntaqlite_parser_next(SyntaqliteParser *p) {
    SyntaqliteParseResult result = {SYNQ_NULL_NODE, 0, NULL};

    if (p->finished || p->had_error) {
        result.error = p->had_error;
        result.error_msg = p->had_error ? p->error_msg : NULL;
        return result;
    }

    // Reset statement state
    p->parseCtx.stmt_completed = 0;
    p->parseCtx.root = SYNQ_NULL_NODE;

    const unsigned char *z = (const unsigned char *)p->source;

    while (p->pos < p->length && z[p->pos] != '\0') {
        int tokenType;
        i64 tokenLen = synq_sqlite3GetToken(z + p->pos, &tokenType);
        if (tokenLen <= 0) break;

        // Collect token if enabled
        if (p->token_list) {
            synq_token_list_append(p->token_list, p->pos,
                                   (uint16_t)tokenLen, (uint16_t)tokenType);
        }

        // Skip whitespace and comments for parser
        if (tokenType == TK_SPACE || tokenType == TK_COMMENT) {
            p->pos += (uint32_t)tokenLen;
            continue;
        }

        // Feed token to parser
        SynqToken token;
        token.z = (const char *)(z + p->pos);
        token.n = (int)tokenLen;
        token.type = tokenType;

        synq_sqlite3Parser(p->lemon, tokenType, token, &p->parseCtx);
        p->last_token_type = tokenType;
        p->pos += (uint32_t)tokenLen;

        if (p->had_error) {
            if (p->error_msg[0] == '\0') {
                snprintf(p->error_msg, sizeof(p->error_msg),
                         "Syntax error near '%.*s'", (int)tokenLen,
                         token.z);
            }
            result.error = 1;
            result.error_msg = p->error_msg;
            return result;
        }

        // Check if a statement was completed by the grammar
        if (p->parseCtx.stmt_completed) {
            p->parseCtx.stmt_completed = 0;
            uint32_t root = p->parseCtx.root;
            if (root != SYNQ_NULL_NODE) {
                result.root = root;
                return result;
            }
            // Bare semicolon — skip and continue
            p->parseCtx.root = SYNQ_NULL_NODE;
        }
    }

    // Reached end of input. Synthesize SEMI if last token wasn't one.
    if (p->last_token_type != 0 && p->last_token_type != TK_SEMI) {
        SynqToken semiToken = {NULL, 0, TK_SEMI};
        synq_sqlite3Parser(p->lemon, TK_SEMI, semiToken, &p->parseCtx);

        if (p->had_error) {
            if (p->error_msg[0] == '\0') {
                snprintf(p->error_msg, sizeof(p->error_msg),
                         "Incomplete statement");
            }
            result.error = 1;
            result.error_msg = p->error_msg;
            return result;
        }

        if (p->parseCtx.stmt_completed && p->parseCtx.root != SYNQ_NULL_NODE) {
            result.root = p->parseCtx.root;
            p->finished = 1;
            return result;
        }
    }

    // Send end-of-input to finalize parser
    SynqToken endToken = {NULL, 0, 0};
    synq_sqlite3Parser(p->lemon, 0, endToken, &p->parseCtx);
    p->finished = 1;

    if (p->had_error) {
        if (p->error_msg[0] == '\0') {
            snprintf(p->error_msg, sizeof(p->error_msg),
                     "Incomplete statement");
        }
        result.error = 1;
        result.error_msg = p->error_msg;
        return result;
    }

    // Check if the final reduction produced a root
    if (p->parseCtx.root != SYNQ_NULL_NODE) {
        result.root = p->parseCtx.root;
    }

    return result;
}

// ============ Public API ============

const SynqNode *syntaqlite_parser_node(SyntaqliteParser *p, uint32_t node_id) {
    return synq_ast_node(&p->astCtx.ast, node_id);
}

const char *syntaqlite_parser_source(SyntaqliteParser *p) {
    return p->source;
}

uint32_t syntaqlite_parser_source_length(SyntaqliteParser *p) {
    return p->length;
}

// ============ Internal API (used by test binaries / formatter) ============

SynqAstContext *syntaqlite_parser_ast_context(SyntaqliteParser *p) {
    return &p->astCtx;
}

SynqArena *syntaqlite_parser_arena(SyntaqliteParser *p) {
    return &p->astCtx.ast;
}

struct SynqTokenList *syntaqlite_parser_token_list(SyntaqliteParser *p) {
    return p->token_list;
}

void syntaqlite_parser_destroy(SyntaqliteParser *p) {
    if (!p) return;
    synq_sqlite3ParserFree(p->lemon, free);
    synq_ast_context_cleanup(&p->astCtx);
    if (p->token_list) synq_vec_free(&p->token_list_storage);
    free(p);
}
