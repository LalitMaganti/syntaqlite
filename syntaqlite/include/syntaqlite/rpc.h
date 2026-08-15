// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

// JSON-RPC C API: parse / format / tokenize / analyze over one in-process
// call. The same protocol the CLI's `serve json` speaks over stdio, for
// embedding. One UTF-8 JSON request in, one {"ok":...} envelope out.
//
//   SyntaqliteRpc* s = syntaqlite_rpc_create_sqlite();
//   const char* req = "{\"op\":\"format\",\"sql\":\"select 1\"}";
//   uint64_t len = 0;
//   uint8_t* resp = syntaqlite_rpc_call(s, (const uint8_t*)req, strlen(req), &len);
//   // resp[0..len]: UTF-8 JSON, e.g. {"ok":true,"result":...}
//   syntaqlite_rpc_free(resp, len);
//   syntaqlite_rpc_destroy(s);
//
// Sessions are reusable but single-threaded. A panic never crosses the
// boundary — it surfaces as an {"ok":false,...} envelope.

#ifndef SYNTAQLITE_RPC_H
#define SYNTAQLITE_RPC_H

#include <stdint.h>
#include "syntaqlite/config.h"

#ifdef __cplusplus
extern "C" {
#endif

// Opaque session; owns the reused parser/tokenizer/analyzer/formatter state.
typedef struct SyntaqliteRpc SyntaqliteRpc;

// Run one JSON request. Writes the response length to *out_len and returns a
// fresh, NUL-terminated UTF-8 envelope (*out_len excludes the NUL); free it
// with syntaqlite_rpc_free(ptr, *out_len). Returns NULL (and *out_len = 0)
// only when handle is NULL.
SYNTAQLITE_API uint8_t* syntaqlite_rpc_call(SyntaqliteRpc* handle,
                                              const uint8_t* request,
                                              uint64_t request_len,
                                              uint64_t* out_len);

// Free a syntaqlite_rpc_call() buffer. Pass its pointer and *out_len.
SYNTAQLITE_API void syntaqlite_rpc_free(uint8_t* ptr, uint64_t len);

// Destroy a session. No-op if NULL.
SYNTAQLITE_API void syntaqlite_rpc_destroy(SyntaqliteRpc* handle);

// Session for a dynamically-loaded dialect (CLI `--dialect`/`--dialect-name`).
// `name` may be NULL to use the default symbol. Returns NULL on load error.
SYNTAQLITE_API SyntaqliteRpc* syntaqlite_rpc_create_dialect(const char* path,
                                                                uint64_t path_len,
                                                                const char* name,
                                                                uint64_t name_len);

#ifndef SYNTAQLITE_OMIT_SQLITE_API

// Session bound to the built-in SQLite dialect. Returns NULL on failure.
SYNTAQLITE_API SyntaqliteRpc* syntaqlite_rpc_create_sqlite(void);

#endif  // SYNTAQLITE_OMIT_SQLITE_API

#ifdef __cplusplus
}
#endif

#endif  // SYNTAQLITE_RPC_H
