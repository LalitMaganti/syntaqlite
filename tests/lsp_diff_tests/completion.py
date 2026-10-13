# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

from python.dev.diff_tests.testing import LspDiffTestBlueprint, TestSuite


class Completion(TestSuite):
    def test_columns_after_select(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE users (id INTEGER, name TEXT);\nSELECT <|> FROM users;",
            op="completion",
            out="""\
            column: id
            column: name
            table: sqlite_master
            table: sqlite_schema
            table: sqlite_temp_master
            table: sqlite_temp_schema
            table: users
            keyword: ALL
            keyword: CASE
            keyword: CAST
            keyword: CROSS
            keyword: CURRENT_DATE
            keyword: CURRENT_TIME
            keyword: CURRENT_TIMESTAMP
            keyword: DISTINCT
            keyword: EXISTS
            keyword: FULL
            keyword: INDEXED
            keyword: INNER
            keyword: LEFT
            keyword: NATURAL
            keyword: NOT
            keyword: NULL
            keyword: OUTER
            keyword: RAISE
            keyword: RIGHT
            function: ->
            function: ->>
            function: abs
            function: avg
            function: changes
            function: char
            function: coalesce
            function: concat
            function: concat_ws
            function: count
            function: cume_dist
            function: current_date
            function: current_time
            function: current_timestamp
            function: date
            function: datetime
            function: dense_rank
            function: first_value
            function: format
            function: glob
            function: group_concat
            function: hex
            function: if
            function: ifnull
            function: iif
            function: instr
            function: json
            function: json_array
            function: json_array_length
            function: json_error_position
            function: json_extract
            function: json_group_array
            function: json_group_object
            function: json_insert
            function: json_object
            function: json_patch
            function: json_pretty
            function: json_quote
            function: json_remove
            function: json_replace
            function: json_set
            function: json_type
            function: json_valid
            function: jsonb
            function: jsonb_array
            function: jsonb_extract
            function: jsonb_group_array
            function: jsonb_group_object
            function: jsonb_insert
            function: jsonb_object
            function: jsonb_patch
            function: jsonb_remove
            function: jsonb_replace
            function: jsonb_set
            function: julianday
            function: lag
            function: last_insert_rowid
            function: last_value
            function: lead
            function: length
            function: like
            function: likelihood
            function: likely
            function: load_extension
            function: lower
            function: ltrim
            function: match
            function: max
            function: min
            function: nth_value
            function: ntile
            function: nullif
            function: octet_length
            function: percent_rank
            function: printf
            function: quote
            function: random
            function: randomblob
            function: rank
            function: replace
            function: round
            function: row_number
            function: rtrim
            function: sign
            function: snippet
            function: sqlite_compileoption_get
            function: sqlite_compileoption_used
            function: sqlite_log
            function: sqlite_source_id
            function: sqlite_version
            function: strftime
            function: string_agg
            function: substr
            function: substring
            function: subtype
            function: sum
            function: time
            function: timediff
            function: total
            function: total_changes
            function: trim
            function: typeof
            function: unhex
            function: unicode
            function: unistr
            function: unistr_quote
            function: unixepoch
            function: unlikely
            function: upper
            function: zeroblob
""",
        )

    def test_qualified_column_only_from_table(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE users (id INTEGER, name TEXT);\nSELECT users.<|> FROM users;",
            op="completion",
            out="""\
            column: id
            column: name
""",
        )

    def test_qualified_builtin_table_columns(self):
        return LspDiffTestBlueprint(
            sql="SELECT sqlite_master.<|> FROM sqlite_master;",
            op="completion",
            out="""\
            column: name
            column: rootpage
            column: sql
            column: tbl_name
            column: type
""",
        )

    def test_only_clause_keywords_after_select_star(self):
        return LspDiffTestBlueprint(
            sql="SELECT * <|>",
            op="completion",
            out="""\
            keyword: EXCEPT
            keyword: FROM
            keyword: GROUP
            keyword: HAVING
            keyword: INTERSECT
            keyword: LIMIT
            keyword: ORDER
            keyword: UNION
            keyword: WHERE
            keyword: WINDOW
""",
        )

    def test_tables_after_from(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE users (id INTEGER);\nCREATE TABLE orders (id INTEGER);\nSELECT * FROM <|>",
            op="completion",
            out="""\
table: orders
table: sqlite_master
table: sqlite_schema
table: sqlite_temp_master
table: sqlite_temp_schema
table: users
keyword: CROSS
keyword: FULL
keyword: INDEXED
keyword: INNER
keyword: LEFT
keyword: NATURAL
keyword: OUTER
keyword: RIGHT
""",
        )

    def test_from_keyword_after_select_star(self):
        return LspDiffTestBlueprint(
            sql="SELECT * FR<|>",
            op="completion",
            out="keyword: FROM",
        )

    def test_second_statement_after_parse_error(self):
        return LspDiffTestBlueprint(
            sql="SELEC 1; SELECT * FR<|>",
            op="completion",
            out="keyword: FROM",
        )

    def test_join_keyword_after_from_alias(self):
        return LspDiffTestBlueprint(
            sql="SELECT * FROM s AS x J<|>",
            op="completion",
            out="keyword: JOIN",
        )

    def test_join_keyword_after_from_table_trailing_space(self):
        return LspDiffTestBlueprint(
            sql="SELECT * FROM slice <|>",
            op="completion",
            out="""\
            table: sqlite_master
            table: sqlite_schema
            table: sqlite_temp_master
            table: sqlite_temp_schema
            keyword: AS
            keyword: CROSS
            keyword: EXCEPT
            keyword: FULL
            keyword: GROUP
            keyword: HAVING
            keyword: INDEXED
            keyword: INNER
            keyword: INTERSECT
            keyword: JOIN
            keyword: LEFT
            keyword: LIMIT
            keyword: NATURAL
            keyword: NOT
            keyword: ON
            keyword: ORDER
            keyword: OUTER
            keyword: RIGHT
            keyword: UNION
            keyword: USING
            keyword: WHERE
            keyword: WINDOW
""",
        )

    def test_partial_keyword_matches_fuzzily(self):
        return LspDiffTestBlueprint(
            sql="SELECT * FROM s AS x jn<|>",
            op="completion",
            out="""\
            keyword: JOIN
""",
        )

    def test_number_under_cursor_matches_nothing(self):
        return LspDiffTestBlueprint(
            sql="SELECT 1<|>",
            op="completion",
            out="""\
            (no completions)
""",
        )

    def test_partial_table_name_after_from(self):
        return LspDiffTestBlueprint(
            sql="SELECT * FROM slice<|>",
            op="completion",
            out="""\
            table: sqlite_schema
            table: sqlite_temp_schema
""",
        )

    def test_partial_table_name_ranks_contiguous_matches_first(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE thread_slice (id INTEGER);\nCREATE TABLE slice (id INTEGER);\nCREATE TABLE spline_cache (id INTEGER);\nSELECT * FROM slice<|>",
            op="completion",
            out="""\
            table: slice
            table: thread_slice
            table: spline_cache
            table: sqlite_schema
            table: sqlite_temp_schema
""",
        )

    def test_join_target_after_join_keyword(self):
        return LspDiffTestBlueprint(
            sql="CREATE TABLE a (id INTEGER);\nCREATE TABLE b (id INTEGER);\nSELECT * FROM a JOIN <|>",
            op="completion",
            out="""\
            table: a
            table: b
            table: sqlite_master
            table: sqlite_schema
            table: sqlite_temp_master
            table: sqlite_temp_schema
            keyword: CROSS
            keyword: FULL
            keyword: INDEXED
            keyword: INNER
            keyword: LEFT
            keyword: NATURAL
            keyword: OUTER
            keyword: RIGHT
""",
        )
