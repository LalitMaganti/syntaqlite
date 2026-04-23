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
com[0] side=leading kind=line off=0 len=11 token_idx=0 layer=0
com[1] side=trailing kind=line off=22 len=15 token_idx=2 layer=0
.
token_comments idx=0 leading=1 trailing=0
lead[0] side=leading kind=line off=0 len=11 token_idx=0 layer=0
.
token_comments idx=2 leading=0 trailing=1
trail[0] side=trailing kind=line off=22 len=15 token_idx=2 layer=0
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
com[0] side=leading kind=block off=0 len=11 token_idx=0 layer=0
.
""",
        )

    def test_multiple_leading_comments_on_same_token(self):
        # Two consecutive line comments before a token: both recorded as
        # leading, both returned in order by token_comments.
        return CApiScenario(
            input="""\
create
collect_tokens 1
reset
-- comment one
-- comment two
SELECT 1;
.
parse_one
dump_comments
token_comments 0
""",
            expected="""\
create ok
collect_tokens ok
reset ok len=39
parse_one ok root=3 recovery=0
comments count=2
com[0] side=leading kind=line off=0 len=14 token_idx=0 layer=0
com[1] side=leading kind=line off=15 len=14 token_idx=0 layer=0
.
token_comments idx=0 leading=2 trailing=0
lead[0] side=leading kind=line off=0 len=14 token_idx=0 layer=0
lead[1] side=leading kind=line off=15 len=14 token_idx=0 layer=0
.
""",
        )

    def test_trailing_on_semicolon_visible_in_first_parse(self):
        # A comment on the same line as `;` is trailing on that semicolon
        # token.  It belongs to the first statement's comment slice and is
        # visible as long as the caller queries before calling parse_one again.
        return CApiScenario(
            input="""\
create
collect_tokens 1
reset
SELECT 1; -- orphan
SELECT 2;
.
parse_one
dump_comments
token_comments 2
""",
            expected="""\
create ok
collect_tokens ok
reset ok len=29
parse_one ok root=3 recovery=0
comments count=1
com[0] side=trailing kind=line off=10 len=9 token_idx=2 layer=0
.
token_comments idx=2 leading=0 trailing=1
trail[0] side=trailing kind=line off=10 len=9 token_idx=2 layer=0
.
""",
        )

    def test_reset_clears_comment_index(self):
        # After reset, the comment index from the previous input is gone.
        # The second parse sees only comments from the fresh input.
        return CApiScenario(
            input="""\
create
collect_tokens 1
reset
-- leading
SELECT 1;
.
parse_one
dump_comments
reset
SELECT 2;
.
parse_one
dump_comments
""",
            expected="""\
create ok
collect_tokens ok
reset ok len=20
parse_one ok root=3 recovery=0
comments count=1
com[0] side=leading kind=line off=0 len=10 token_idx=0 layer=0
.
reset ok len=9
parse_one ok root=3 recovery=0
comments count=0
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


class NodeTokenRangeParser(CApiTestSuite):
    """Tests for `syntaqlite_node_token_range` — the primitive that maps an
    AST node to the inclusive `[first, last]` token indices the parser fed
    to Lemon while reducing it.

    The primitive is intentionally low-level.  Callers compose it with
    `token_leading_comments` / `token_trailing_comments` to answer "what
    comments are attached to this node's boundaries?" and use the
    `layer_id` field on each comment to filter authored-source comments
    from expansion-body ones.
    """

    def test_returns_none_without_extents(self):
        return CApiScenario(
            input="""\
create
collect_tokens 1
reset
SELECT 1 FROM t;
.
parse_one
node_token_range 4
""",
            expected="""\
create ok
collect_tokens ok
reset ok len=16
parse_one ok root=4 recovery=0
node_token_range id=4 none
""",
        )

    def test_returns_none_for_null_or_out_of_range(self):
        return CApiScenario(
            input="""\
create
collect_tokens 1
collect_extents 1
reset
SELECT 1 FROM t;
.
parse_one
node_token_range 4294967295
node_token_range 99
""",
            expected="""\
create ok
collect_tokens ok
collect_extents ok
reset ok len=16
parse_one ok root=4 recovery=0
node_token_range id=4294967295 none
node_token_range id=99 none
""",
        )

    def test_root_covers_all_significant_tokens(self):
        # Extent of the outermost SELECT reduction ends at `t`; the
        # terminating `;` is not part of the reduction, so the range is
        # [0, 3] (SELECT..t), not [0, 4] (SELECT..;).  This is what the
        # existing `node_text` test at id=4 shows: the extent string is
        # "SELECT 1 FROM t" without the `;`.
        return CApiScenario(
            input="""\
create
collect_tokens 1
collect_extents 1
reset
SELECT 1 FROM t;
.
parse_one
dump_tokens
node_token_range 4
""",
            expected="""\
create ok
collect_tokens ok
collect_extents ok
reset ok len=16
parse_one ok root=4 recovery=0
tokens count=5
tok[0] type=161 off=0 len=6 flags=0 layer=0
tok[1] type=110 off=7 len=1 flags=0 layer=0
tok[2] type=127 off=9 len=4 flags=0 layer=0
tok[3] type=40 off=14 len=1 flags=1 layer=0
tok[4] type=112 off=15 len=1 flags=0 layer=0
.
node_token_range id=4 first=0 last=3
""",
        )

    def test_compose_with_token_comments(self):
        # Demonstrate the canonical composition: node_token_range gives
        # you a token index, and token_leading_comments / token_trailing_comments
        # attach the per-token comments.  The returned comments carry
        # layer=0 because they were authored in the source.
        return CApiScenario(
            input="""\
create
collect_tokens 1
collect_extents 1
reset
-- preamble
SELECT 1 FROM t; -- tail
.
parse_one
node_token_range 4
token_comments 0
token_comments 4
""",
            expected="""\
create ok
collect_tokens ok
collect_extents ok
reset ok len=36
parse_one ok root=4 recovery=0
node_token_range id=4 first=0 last=3
token_comments idx=0 leading=1 trailing=0
lead[0] side=leading kind=line off=0 len=11 token_idx=0 layer=0
.
token_comments idx=4 leading=0 trailing=1
trail[0] side=trailing kind=line off=29 len=7 token_idx=4 layer=0
.
""",
        )

    def test_node_comments_surface_node_boundary_comments(self):
        # `node_comments` composes node_token_range with
        # token_{leading,trailing}_comments.  Comments at the first
        # token of the node are leading; at the last token, trailing.
        # Comments attached to interior tokens (e.g. the SELECT
        # keyword trail) are NOT surfaced here.
        return CApiScenario(
            input="""\
create
collect_tokens 1
collect_extents 1
reset
-- preamble
SELECT /* after keyword */ 1 FROM t; -- tail
.
parse_one
node_comments 4
""",
            expected="""\
create ok
collect_tokens ok
collect_extents ok
reset ok len=56
parse_one ok root=4 recovery=0
node_comments id=4 leading=1 trailing=0
lead[0] side=leading kind=line off=0 len=11 token_idx=0 layer=0
.
""",
        )

    def test_node_comments_invalid_node_id_returns_empty(self):
        # Out-of-range and sentinel (UINT32_MAX) node ids return zero counts
        # without crashing or triggering an error line — same shape as a
        # valid node with no comments.
        return CApiScenario(
            input="""\
create
collect_tokens 1
collect_extents 1
reset
SELECT 1;
.
parse_one
node_comments 4294967295
node_comments 99
""",
            expected="""\
create ok
collect_tokens ok
collect_extents ok
reset ok len=9
parse_one ok root=3 recovery=0
node_comments id=4294967295 leading=0 trailing=0
.
node_comments id=99 leading=0 trailing=0
.
""",
        )

    def test_node_comments_without_extents_returns_empty(self):
        # When extents are not collected, node_token_range returns none, so
        # node_comments falls back to zero counts for every node id.
        return CApiScenario(
            input="""\
create
collect_tokens 1
reset
-- preamble
SELECT 1;
.
parse_one
node_comments 3
""",
            expected="""\
create ok
collect_tokens ok
reset ok len=21
parse_one ok root=3 recovery=0
node_comments id=3 leading=0 trailing=0
.
""",
        )

    def test_node_comments_expansion_layer_comment_surfaced(self):
        # When a node's first token lives in an expansion layer, its leading
        # comment is reported with layer=1, making it distinguishable from
        # authored-source comments.
        return CApiScenario(
            input="""\
create
collect_tokens 1
collect_extents 1
macro_fallback 1
macro_register annotate
/* inner */ 42
.
reset
SELECT annotate!(x) FROM t;
.
parse_one
node_comments 0
node_comments 1
""",
            expected="""\
create ok
collect_tokens ok
collect_extents ok
macro_fallback ok
macro_register ok name=annotate len=14
reset ok len=27
parse_one ok root=4 recovery=0
node_comments id=0 leading=1 trailing=0
lead[0] side=leading kind=block off=0 len=11 token_idx=1 layer=1
.
node_comments id=1 leading=1 trailing=0
lead[0] side=leading kind=block off=0 len=11 token_idx=1 layer=1
.
""",
        )

    def test_macro_fallback_call_is_single_layer0_token(self):
        # With macro_fallback enabled, an unregistered `name!(args)` is
        # consumed as a single TK_ID token (still layer 0).  The
        # enclosing SELECT's node_token_range simply spans those
        # layer-0 tokens — no expansion layer involved here.
        return CApiScenario(
            input="""\
create
collect_tokens 1
collect_extents 1
macro_fallback 1
reset
SELECT foo!(a) FROM t;
.
parse_one
dump_tokens
node_token_range 4
""",
            expected="""\
create ok
collect_tokens ok
collect_extents ok
macro_fallback ok
reset ok len=22
parse_one ok root=4 recovery=0
tokens count=5
tok[0] type=161 off=0 len=6 flags=0 layer=0
tok[1] type=40 off=7 len=7 flags=1 layer=0
tok[2] type=127 off=15 len=4 flags=0 layer=0
tok[3] type=40 off=20 len=1 flags=1 layer=0
tok[4] type=112 off=21 len=1 flags=0 layer=0
.
node_token_range id=4 first=0 last=3
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

    def test_registered_macro_body_comment_has_nonzero_layer_id(self):
        # A block comment inside a registered macro body is recorded with
        # layer_id=1.  This distinguishes it from authored-source comments
        # (layer_id=0) so callers can filter by provenance.
        return CApiScenario(
            input="""\
create
collect_tokens 1
collect_extents 1
macro_fallback 1
macro_register annotate
/* inner */ 42
.
reset
SELECT annotate!(x) FROM t;
.
parse_one
dump_tokens
dump_comments
""",
            expected="""\
create ok
collect_tokens ok
collect_extents ok
macro_fallback ok
macro_register ok name=annotate len=14
reset ok len=27
parse_one ok root=4 recovery=0
tokens count=5
tok[0] type=161 off=0 len=6 flags=0 layer=0
tok[1] type=110 off=12 len=2 flags=0 layer=1
tok[2] type=127 off=20 len=4 flags=0 layer=0
tok[3] type=40 off=25 len=1 flags=1 layer=0
tok[4] type=112 off=26 len=1 flags=0 layer=0
.
comments count=1
com[0] side=leading kind=block off=0 len=11 token_idx=1 layer=1
.
""",
        )

    def test_expansion_same_line_trailing_comment(self):
        # A comment on the same line as the only token in a macro body is
        # classified as trailing on that token (same rule as layer 0).
        return CApiScenario(
            input="""\
create
collect_tokens 1
collect_extents 1
macro_fallback 1
macro_register annotate
42 /* side note */
.
reset
SELECT annotate!(x) FROM t;
.
parse_one
dump_tokens
dump_comments
""",
            expected="""\
create ok
collect_tokens ok
collect_extents ok
macro_fallback ok
macro_register ok name=annotate len=18
reset ok len=27
parse_one ok root=4 recovery=0
tokens count=5
tok[0] type=161 off=0 len=6 flags=0 layer=0
tok[1] type=110 off=0 len=2 flags=0 layer=1
tok[2] type=127 off=20 len=4 flags=0 layer=0
tok[3] type=40 off=25 len=1 flags=1 layer=0
tok[4] type=112 off=26 len=1 flags=0 layer=0
.
comments count=1
com[0] side=trailing kind=block off=3 len=15 token_idx=1 layer=1
.
""",
        )

    def test_both_outer_and_expansion_leading_comments(self):
        # When the outer source has a leading comment AND the macro body has
        # a leading comment, both are captured.  They are distinguished by
        # layer_id: the outer comment has layer=0 (visible via the SelectStmt
        # boundary), the expansion comment has layer=1 (visible via the Literal
        # boundary whose first token is the expansion token).
        return CApiScenario(
            input="""\
create
collect_tokens 1
collect_extents 1
macro_fallback 1
macro_register annotate
/* inner */ 42
.
reset
-- outer
SELECT annotate!(x) FROM t;
.
parse_one
dump_tokens
dump_comments
node_comments 4
node_comments 0
""",
            expected="""\
create ok
collect_tokens ok
collect_extents ok
macro_fallback ok
macro_register ok name=annotate len=14
reset ok len=36
parse_one ok root=4 recovery=0
tokens count=5
tok[0] type=161 off=9 len=6 flags=0 layer=0
tok[1] type=110 off=12 len=2 flags=0 layer=1
tok[2] type=127 off=29 len=4 flags=0 layer=0
tok[3] type=40 off=34 len=1 flags=1 layer=0
tok[4] type=112 off=35 len=1 flags=0 layer=0
.
comments count=2
com[0] side=leading kind=line off=0 len=8 token_idx=0 layer=0
com[1] side=leading kind=block off=0 len=11 token_idx=1 layer=1
.
node_comments id=4 leading=1 trailing=0
lead[0] side=leading kind=line off=0 len=8 token_idx=0 layer=0
.
node_comments id=0 leading=1 trailing=0
lead[0] side=leading kind=block off=0 len=11 token_idx=1 layer=1
.
""",
        )

    def test_both_outer_and_expansion_trailing_comments(self):
        # When the macro body has a trailing block comment on the expansion
        # token AND the outer source has a trailing LINE comment on the
        # SelectStmt's last token (`t`), both are captured with correct layers.
        # Line comments are always immediately trailing (nothing can follow
        # them on the same line).  node_comments on the SelectStmt surfaces
        # the outer trailing (layer=0 on tok[3]=`t`); node_comments on the
        # Literal surfaces the expansion trailing (layer=1 on tok[1]).
        return CApiScenario(
            input="""\
create
collect_tokens 1
collect_extents 1
macro_fallback 1
macro_register annotate
42 /* inner */
.
reset
SELECT annotate!(x) FROM t -- outer
.
parse_one
dump_tokens
dump_comments
node_comments 4
node_comments 0
""",
            expected="""\
create ok
collect_tokens ok
collect_extents ok
macro_fallback ok
macro_register ok name=annotate len=14
reset ok len=35
parse_one ok root=4 recovery=0
tokens count=4
tok[0] type=161 off=0 len=6 flags=0 layer=0
tok[1] type=110 off=0 len=2 flags=0 layer=1
tok[2] type=127 off=20 len=4 flags=0 layer=0
tok[3] type=40 off=25 len=1 flags=1 layer=0
.
comments count=2
com[0] side=trailing kind=block off=3 len=11 token_idx=1 layer=1
com[1] side=trailing kind=line off=27 len=8 token_idx=3 layer=0
.
node_comments id=4 leading=0 trailing=1
trail[0] side=trailing kind=line off=27 len=8 token_idx=3 layer=0
.
node_comments id=0 leading=0 trailing=1
trail[0] side=trailing kind=block off=3 len=11 token_idx=1 layer=1
.
""",
        )

    def test_block_comment_between_tokens_same_line_is_leading_on_next(self):
        # Under the deferred-classification rule, a block comment that is
        # on the same line as the preceding token becomes LEADING on the
        # next token (not trailing on the preceding one) when a token
        # follows it on the same line.  Line comments are exempt: they are
        # always immediately trailing because nothing can follow them.
        return CApiScenario(
            input="""\
create
collect_tokens 1
collect_extents 1
reset
SELECT 1 /* mid */ + 2 FROM t;
.
parse_one
dump_comments
token_comments 1
token_comments 2
""",
            expected="""\
create ok
collect_tokens ok
collect_extents ok
reset ok len=30
parse_one ok root=6 recovery=0
comments count=1
com[0] side=leading kind=block off=9 len=9 token_idx=2 layer=0
.
token_comments idx=1 leading=0 trailing=0
.
token_comments idx=2 leading=1 trailing=0
lead[0] side=leading kind=block off=9 len=9 token_idx=2 layer=0
.
""",
        )

    def test_source_block_comment_between_select_and_macro_is_leading_on_expansion(self):
        # A block comment in the outer source between SELECT and a macro
        # call is deferred and, since the expansion token follows on the
        # same source line, resolves as LEADING on the first expansion
        # token.  The comment retains layer=0 (it lives in the source
        # buffer) while token_idx points to the layer=1 expansion token.
        return CApiScenario(
            input="""\
create
collect_tokens 1
collect_extents 1
macro_fallback 1
macro_register wrap
42
.
reset
SELECT /* between */ wrap!(x) FROM t;
.
parse_one
dump_tokens
dump_comments
token_comments 0
token_comments 1
node_comments 0
""",
            expected="""\
create ok
collect_tokens ok
collect_extents ok
macro_fallback ok
macro_register ok name=wrap len=2
reset ok len=37
parse_one ok root=4 recovery=0
tokens count=5
tok[0] type=161 off=0 len=6 flags=0 layer=0
tok[1] type=110 off=0 len=2 flags=0 layer=1
tok[2] type=127 off=30 len=4 flags=0 layer=0
tok[3] type=40 off=35 len=1 flags=1 layer=0
tok[4] type=112 off=36 len=1 flags=0 layer=0
.
comments count=1
com[0] side=leading kind=block off=7 len=13 token_idx=1 layer=0
.
token_comments idx=0 leading=0 trailing=0
.
token_comments idx=1 leading=1 trailing=0
lead[0] side=leading kind=block off=7 len=13 token_idx=1 layer=0
.
node_comments id=0 leading=1 trailing=0
lead[0] side=leading kind=block off=7 len=13 token_idx=1 layer=0
.
""",
        )

    def test_expansion_leading_comment_after_newline(self):
        # A comment on a fresh line before the first expansion token is
        # classified as leading (the newline breaks the same-line rule).
        # offset is layer-local: 1 byte into the body (past the leading \n).
        return CApiScenario(
            input="""\
create
collect_tokens 1
collect_extents 1
macro_fallback 1
macro_register wrap

/* before */ 42
.
reset
SELECT wrap!(x) FROM t;
.
parse_one
dump_tokens
dump_comments
""",
            expected="""\
create ok
collect_tokens ok
collect_extents ok
macro_fallback ok
macro_register ok name=wrap len=16
reset ok len=23
parse_one ok root=4 recovery=0
tokens count=5
tok[0] type=161 off=0 len=6 flags=0 layer=0
tok[1] type=110 off=14 len=2 flags=0 layer=1
tok[2] type=127 off=16 len=4 flags=0 layer=0
tok[3] type=40 off=21 len=1 flags=1 layer=0
tok[4] type=112 off=22 len=1 flags=0 layer=0
.
comments count=1
com[0] side=leading kind=block off=1 len=12 token_idx=1 layer=1
.
""",
        )
