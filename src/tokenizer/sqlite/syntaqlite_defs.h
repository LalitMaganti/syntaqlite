/*
** Type definitions and macros for syntaqlite tokenizer.
** DO NOT EDIT - regenerate with: python3 python/tools/extract_tokenizer.py
*/
#ifndef SYNTAQLITE_SRC_TOKENIZER_SQLITE_SYNTAQLITE_DEFS_H
#define SYNTAQLITE_SRC_TOKENIZER_SQLITE_SYNTAQLITE_DEFS_H

#include <assert.h>
#include <stdint.h>

/* SQLite type aliases */
typedef int64_t i64;
typedef uint8_t u8;
typedef uint32_t u32;

/* SQLite result codes */
#define SQLITE_OK    0
#define SQLITE_ERROR 1

/* ASCII mode - disables EBCDIC support */
#define SQLITE_ASCII 1

/* Digit separator for numeric literals (e.g., 1_000_000) */
#define SQLITE_DIGIT_SEPARATOR '_'

/* No-op test macro */
#define syntaqlite_sqlite3Testcase(x)

/* C++17 fallthrough, C no-op */
#ifdef __cplusplus
#define deliberate_fall_through [[fallthrough]]
#else
#define deliberate_fall_through
#endif

#endif /* SYNTAQLITE_SRC_TOKENIZER_SQLITE_SYNTAQLITE_DEFS_H */
