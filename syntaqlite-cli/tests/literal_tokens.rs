// Copyright 2025 The syntaqlite Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0.

//! Generate, compile, and exercise a dialect with custom operator tokens.
#![cfg(feature = "codegen")]

use std::{fs, process::Command};

fn run(command: &mut Command) {
    let output = command.output().expect("run command");
    assert!(
        output.status.success(),
        "{command:?}:\n{}\n{}",
        String::from_utf8_lossy(&output.stdout),
        String::from_utf8_lossy(&output.stderr)
    );
}

#[test]
fn literal_tokens_reach_tokenizer_and_parser() {
    let temp = tempfile::tempdir().expect("tempdir");
    let root = temp.path();
    fs::write(
        root.join("operators.tokens"),
        "# Operators\nPIPE = \"|>\";\nLONGPIPE = \"|>>\";\nCARET = \"^\";\n",
    )
    .expect("tokens");
    fs::write(
        root.join("pipe.y"),
        "cmd(A) ::= PIPE select(B). { A = B; }\n",
    )
    .expect("grammar");
    run(Command::new(env!("CARGO_BIN_EXE_syntaqlite")).args([
        "dialect",
        "generate",
        "--name",
        "pipes",
        "--output-type",
        "full",
        "--output-dir",
        root.to_str().expect("path"),
        "--actions-dir",
        root.to_str().expect("path"),
        "--tokens-file",
        root.join("operators.tokens").to_str().expect("path"),
    ]));
    fs::write(
        root.join("test.c"),
        r#"
#include "syntaqlite_pipes.h"
#include <assert.h>
#include <string.h>

int main(void) {
  SyntaqliteTokenizer* tokenizer = syntaqlite_tokenizer_create_pipes(0);
  struct { const char* text; unsigned type, length; } cases[] = {
    {"|>x", SYNTAQLITE_TK_PIPE, 2}, {"|>>", SYNTAQLITE_TK_LONGPIPE, 3},
    {"|>", SYNTAQLITE_TK_PIPE, 2}, /* EOF before the longer spelling */
    {"|", SYNTAQLITE_TK_BITOR, 1}, {"||", SYNTAQLITE_TK_CONCAT, 2},
    {"| >", SYNTAQLITE_TK_BITOR, 1}, {"^", SYNTAQLITE_TK_CARET, 1},
    {"'|>'", SYNTAQLITE_TK_STRING, 4}, {"\"|>\"", SYNTAQLITE_TK_ID, 4},
    {"-- |>\n", SYNTAQLITE_TK_COMMENT, 5}, {"/* |> */", SYNTAQLITE_TK_COMMENT, 8},
    {"PIPE", SYNTAQLITE_TK_ID, 4}, {"", SYNTAQLITE_TK_ILLEGAL, 0}
  };
  for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
    SyntaqliteToken token;
    syntaqlite_tokenizer_reset(tokenizer, cases[i].text, (unsigned)strlen(cases[i].text));
    unsigned n = syntaqlite_tokenizer_next(tokenizer, &token);
    assert(n == (cases[i].length != 0));
    if (n) assert(token.type == cases[i].type && token.length == cases[i].length);
  }
  syntaqlite_tokenizer_destroy(tokenizer);
  SyntaqliteParser* parser = syntaqlite_parser_create_pipes(0);
  const char* sql = "|> SELECT pipe, '|>' FROM t;";
  syntaqlite_parser_reset(parser, sql, (unsigned)strlen(sql));
  assert(syntaqlite_parser_next(parser) == SYNTAQLITE_PARSE_OK);
  assert(syntaqlite_parser_next(parser) == SYNTAQLITE_PARSE_DONE);
  syntaqlite_parser_destroy(parser);
  return 0;
}
"#,
    )
    .expect("C test");
    run(Command::new("cc").current_dir(root).args([
        "-std=c11",
        "-DSYNTAQLITE_OMIT_SQLITE_API",
        "syntaqlite_pipes.c",
        "test.c",
        "-o",
        "test",
    ]));
    run(&mut Command::new(root.join("test")));
    fs::write(root.join("operators.tokens"), "SELECT = \"|>\";\n").expect("conflict");
    let output = Command::new(env!("CARGO_BIN_EXE_syntaqlite"))
        .args([
            "dialect",
            "generate",
            "--name",
            "pipes",
            "--output-type",
            "full",
            "--output-dir",
            root.to_str().expect("path"),
            "--tokens-file",
            root.join("operators.tokens").to_str().expect("path"),
        ])
        .output()
        .expect("invalid generation");
    assert!(!output.status.success());
    assert!(String::from_utf8_lossy(&output.stderr).contains("conflicts with a base SQLite token"));
}
