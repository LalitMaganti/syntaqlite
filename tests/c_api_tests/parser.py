# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Parser C API scenarios.

Each test method returns a `CApiScenario` with a line-protocol `input`
fed to `parser_driver.c` and the byte-for-byte `expected` stdout.
See `tests/c_api_tests/parser_driver.c` for the supported verbs.
"""

from python.dev.integration_tests.suites.c_api import CApiScenario, CApiTestSuite


class BasicParser(CApiTestSuite):
    def test_simple_select(self):
        return CApiScenario(
            input="""\
create
reset
SELECT 1 FROM t;
.
parse_one
node_count
parser_text
""",
            expected="""\
create ok
reset ok len=16
parse_one ok root=4 recovery=0
node_count 5
parser_text off=0 len=16
SELECT 1 FROM t;
.
""",
        )

    def test_multi_statements(self):
        return CApiScenario(
            input="""\
create
reset
SELECT 1; SELECT 2; SELECT 3;
.
parse_one
parse_one
parse_one
parse_one
""",
            expected="""\
create ok
reset ok len=29
parse_one ok root=3 recovery=0
parse_one ok root=3 recovery=0
parse_one ok root=3 recovery=0
parse_one done root=0 recovery=0
""",
        )

    def test_bare_semicolons_done(self):
        return CApiScenario(
            input="""\
create
reset
;; ;;
.
parse_one
""",
            expected="""\
create ok
reset ok len=5
parse_one done root=0 recovery=0
""",
        )

    def test_empty_input(self):
        return CApiScenario(
            input="""\
create
reset
.
parse_one
""",
            expected="""\
create ok
reset ok len=0
parse_one done root=0 recovery=0
""",
        )


class ErrorParser(CApiTestSuite):
    def test_parse_error(self):
        return CApiScenario(
            input="""\
create
reset
SELECT from where;
.
parse_one
error_info
""",
            expected="""\
create ok
reset ok len=18
parse_one error root=0 recovery=0
error_info msg="syntax error near 'from'" off=7 len=4 recovery=0
""",
        )

    def test_parse_continues_after_error(self):
        return CApiScenario(
            input="""\
create
reset
SELECT from where; SELECT 1;
.
parse_one
error_info
parse_one
""",
            expected="""\
create ok
reset ok len=28
parse_one error root=0 recovery=0
error_info msg="syntax error near 'from'" off=7 len=4 recovery=0
parse_one ok root=3 recovery=0
""",
        )

    def test_no_handle_reset(self):
        return CApiScenario(
            input="""\
reset
SELECT 1;
.
""",
            expected="""\
reset err no_handle
""",
        )


class TokensParser(CApiTestSuite):
    def test_tokens_disabled_empty(self):
        return CApiScenario(
            input="""\
create
reset
SELECT 1 FROM t;
.
parse_one
dump_tokens
""",
            expected="""\
create ok
reset ok len=16
parse_one ok root=4 recovery=0
tokens count=0
.
""",
        )

    def test_tokens_enabled(self):
        return CApiScenario(
            input="""\
create
collect_tokens 1
reset
SELECT 1 FROM t;
.
parse_one
dump_tokens
""",
            expected="""\
create ok
collect_tokens ok
reset ok len=16
parse_one ok root=4 recovery=0
tokens count=5
tok[0] type=161 off=0 len=6 flags=0 layer=0
tok[1] type=110 off=7 len=1 flags=0 layer=0
tok[2] type=127 off=9 len=4 flags=0 layer=0
tok[3] type=40 off=14 len=1 flags=1 layer=0
tok[4] type=112 off=15 len=1 flags=0 layer=0
.
""",
        )

    def test_config_after_reset_sealed(self):
        return CApiScenario(
            input="""\
create
reset
SELECT 1;
.
collect_tokens 1
collect_extents 1
""",
            expected="""\
create ok
reset ok len=9
collect_tokens err already_used
collect_extents err already_used
""",
        )


class CommentsParser(CApiTestSuite):
    def test_leading_and_trailing(self):
        return CApiScenario(
            input="""\
create
collect_tokens 1
reset
-- preamble
SELECT 1; -- after select
.
parse_one
dump_comments
token_comments 0
token_comments 2
""",
            expected="""\
create ok
collect_tokens ok
reset ok len=37
parse_one ok root=3 recovery=0
comments count=2
com[0] side=leading kind=line off=0 len=11 token_idx=0
com[1] side=trailing kind=line off=22 len=15 token_idx=2
.
token_comments idx=0 leading=1 trailing=0
lead[0] side=leading kind=line off=0 len=11 token_idx=0
.
token_comments idx=2 leading=0 trailing=1
trail[0] side=trailing kind=line off=22 len=15 token_idx=2
.
""",
        )

    def test_block_comment(self):
        return CApiScenario(
            input="""\
create
collect_tokens 1
reset
/* block */ SELECT 1;
.
parse_one
dump_comments
""",
            expected="""\
create ok
collect_tokens ok
reset ok len=21
parse_one ok root=3 recovery=0
comments count=1
com[0] side=leading kind=block off=0 len=11 token_idx=0
.
""",
        )


class ExtentsParser(CApiTestSuite):
    def test_extents_disabled_none(self):
        return CApiScenario(
            input="""\
create
reset
SELECT 1 FROM t;
.
parse_one
node_text 4
""",
            expected="""\
create ok
reset ok len=16
parse_one ok root=4 recovery=0
node_text none
""",
        )

    def test_extents_enabled_slice(self):
        return CApiScenario(
            input="""\
create
collect_extents 1
reset
SELECT 1 FROM t;
.
parse_one
node_text 4
""",
            expected="""\
create ok
collect_extents ok
reset ok len=16
parse_one ok root=4 recovery=0
node_text id=4 off=0 len=15
SELECT 1 FROM t
.
""",
        )


class IntrospectionParser(CApiTestSuite):
    def test_full_text_roundtrip(self):
        return CApiScenario(
            input="""\
create
reset
SELECT 1; SELECT 2;
.
parse_one
full_text
""",
            expected="""\
create ok
reset ok len=19
parse_one ok root=3 recovery=0
full_text len=19
SELECT 1; SELECT 2;
.
""",
        )

    def test_parser_text_per_statement(self):
        return CApiScenario(
            input="""\
create
reset
SELECT 1; SELECT 22;
.
parse_one
parser_text
parse_one
parser_text
""",
            expected="""\
create ok
reset ok len=20
parse_one ok root=3 recovery=0
parser_text off=0 len=9
SELECT 1;
.
parse_one ok root=3 recovery=0
parser_text off=10 len=10
SELECT 22;
.
""",
        )

    def test_dump_root(self):
        return CApiScenario(
            input="""\
create
reset
SELECT 1;
.
parse_one
dump_root
""",
            expected="""\
create ok
reset ok len=9
parse_one ok root=3 recovery=0
dump_root ok
SelectStmt
  flags: (none)
  columns:
    ResultColumnList [1 items]
      ResultColumn
        flags: (none)
        alias: (none)
        expr:
          Literal
            literal_type: INTEGER
            source: "1"
  from_clause: (none)
  where_clause: (none)
  groupby: (none)
  having: (none)
  orderby: (none)
  limit_clause: (none)
  window_clause: (none)
.
""",
        )


class MacroRewrites(CApiTestSuite):
    """Macro rewrite API — call-site args and self-resolving buffers.

    These verify the shape C consumers actually see: `args()` populated
    for top-level fallback calls, `is_fallback` flag set, and
    `parent_buffer + offset` giving back the authored text.
    """

    def test_fallback_call_exposes_args(self):
        return CApiScenario(
            input="""\
create
macro_fallback 1
reset
SELECT foo!(a, 1 + 2, 'x');
.
parse_one
dump_macros
""",
            expected="""\
create ok
macro_fallback ok
reset ok len=27
parse_one ok root=3 recovery=0
macros count=1
mac[0] parent=source call_off=7 call_len=19 is_fallback=1 name="foo"
  call_text="foo!(a, 1 + 2, 'x')"
  args count=3
    arg[0] off=12 len=1 text="a"
    arg[1] off=15 len=5 text="1 + 2"
    arg[2] off=22 len=3 text="'x'"
.
""",
        )

    def test_empty_fallback_call_has_no_args(self):
        return CApiScenario(
            input="""\
create
macro_fallback 1
reset
SELECT foo!();
.
parse_one
dump_macros
""",
            expected="""\
create ok
macro_fallback ok
reset ok len=14
parse_one ok root=3 recovery=0
macros count=1
mac[0] parent=source call_off=7 call_len=6 is_fallback=1 name="foo"
  call_text="foo!()"
  args count=0
.
""",
        )
