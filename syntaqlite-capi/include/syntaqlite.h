/*
** Copyright 2026 The syntaqlite Authors. All rights reserved.
** Licensed under the Apache License, Version 2.0.
**
** syntaqlite.h: the whole libsyntaqlite C API.
**
** The declarations are in the per-module headers below. The parser, tokenizer,
** and SQLite dialect headers are in syntaqlite-syntax/include, so builds from
** source need both that directory and this one on the include path. Release
** archives ship this header as a single generated file instead.
*/
#ifndef SYNTAQLITE_H
#define SYNTAQLITE_H

#include "syntaqlite/parser.h"
#include "syntaqlite/tokenizer.h"
#include "syntaqlite/incremental.h"
#include "syntaqlite_sqlite/sqlite.h"
#include "syntaqlite_sqlite/sqlite_node.h"
#include "syntaqlite_sqlite/sqlite_tokens.h"
#include "syntaqlite/analysis.h"
#include "syntaqlite/formatter.h"
#include "syntaqlite/rpc.h"

#endif  // SYNTAQLITE_H
