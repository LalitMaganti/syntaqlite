/*
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
** Type definitions and macros for syntaqlite tokenizer.
*/
#ifndef SYNTAQLITE_SRC_TOKENIZER_SYNTAQLITE_DEFS_H
#define SYNTAQLITE_SRC_TOKENIZER_SYNTAQLITE_DEFS_H

#include <assert.h>
#include <stdint.h>

/* SQLite type aliases */
typedef int64_t i64;
typedef uint8_t u8;
typedef uint32_t u32;

/* ASCII mode - disables EBCDIC support */
#define SQLITE_ASCII 1

/* Digit separator for numeric literals (e.g., 1_000_000) */
#define SQLITE_DIGIT_SEPARATOR '_'

/* C++17 fallthrough, C no-op */
#ifdef __cplusplus
#define deliberate_fall_through [[fallthrough]]
#else
#define deliberate_fall_through
#endif

#endif /* SYNTAQLITE_SRC_TOKENIZER_SYNTAQLITE_DEFS_H */
