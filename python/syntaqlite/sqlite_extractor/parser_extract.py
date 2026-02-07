# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Extract parser data from Lemon-generated parse.c.

This module extracts grammar data from a Lemon-generated parser file
and formats it for inclusion in amalgamated extension headers.
"""

from __future__ import annotations

import re
from dataclasses import dataclass

# Lemon-generated section markers
TOKEN_DEFS_START = "/* These constants specify the various numeric values for terminal symbols."
TOKEN_DEFS_END = "/**************** End token definitions ***************************************/"
CONTROL_DEFS_START = "/************* Begin control #defines *****************************************/"
CONTROL_DEFS_END = "/************* End control #defines *******************************************/"
CONTROL_DEFS_COMMENT_START = "/* The next sections is a series of control #defines."
PARSING_TABLES_COMMENT_START = "/* Next are the tables used to determine what action to take"
PARSING_TABLES_START = "*********** Begin parsing tables **********************************************/"
PARSING_TABLES_END = "/********** End of lemon-generated parsing tables *****************************/"
FALLBACK_COMMENT_START = "/* The next table maps tokens (terminal symbols) into fallback tokens."
REDUCE_ACTIONS_START = "/********** Begin reduce actions **********************************************/"
REDUCE_ACTIONS_END = "/********** End reduce actions ************************************************/"

# Struct markers
STRUCT_COMMENT_START = "/* The following structure represents a single element of the"
INCLUDE_END = "/**************** End of %include directives **********************************/"


@dataclass
class ParserData:
    """Extracted parser data from a Lemon-generated parse.c."""

    token_defines: str = ""
    control_defines: str = ""
    parsing_tables: str = ""
    fallback_table: str = ""
    token_names: str = ""
    rule_names: str = ""
    rule_info_lhs: str = ""
    rule_info_nrhs: str = ""
    reduce_actions: str = ""
    struct_defs: str = ""


def extract_section(
    content: str,
    start_marker: str,
    end_marker: str,
    include_markers: bool = False,
) -> str:
    """Extract a section of content between markers.

    Args:
        content: The content to search.
        start_marker: Text marking the start of the section.
        end_marker: Text marking the end of the section.
        include_markers: Whether to include the markers in the result.

    Returns:
        The extracted section, or empty string if not found.
    """
    start_pos = content.find(start_marker)
    if start_pos == -1:
        return ""

    if include_markers:
        extract_start = start_pos
    else:
        extract_start = start_pos + len(start_marker)

    end_pos = content.find(end_marker, extract_start)
    if end_pos == -1:
        return ""

    if include_markers:
        extract_end = end_pos + len(end_marker)
    else:
        extract_end = end_pos

    return content[extract_start:extract_end].strip()


def extract_array(content: str, array_pattern: str) -> str:
    """Extract an array definition from content.

    Args:
        content: The content to search.
        array_pattern: Regex pattern for the array declaration.

    Returns:
        The full array definition including declaration and closing brace.
    """
    match = re.search(array_pattern, content, re.DOTALL)
    if not match:
        return ""

    start = match.start()
    brace_count = 0

    for i, char in enumerate(content[start:], start):
        if char == "{":
            brace_count += 1
        elif char == "}":
            brace_count -= 1
            if brace_count == 0:
                semicolon = content.find(";", i)
                end = semicolon + 1 if semicolon != -1 else i + 1
                return content[start:end].strip()

    return ""


def extract_struct_defs(content: str) -> str:
    """Extract parser struct definitions (yyStackEntry + yyParser).

    Args:
        content: Lemon-generated parser content.

    Returns:
        The struct definitions block including comment and typedefs.
    """
    start_pos = content.find(STRUCT_COMMENT_START)
    if start_pos == -1:
        return ""

    # Find the end: "typedef struct <prefix>_yyParser <prefix>_yyParser;"
    end_match = re.search(r"typedef struct \w+_yyParser \w+_yyParser;", content[start_pos:])
    if not end_match:
        return ""

    end_pos = start_pos + end_match.end()
    return content[start_pos:end_pos]


def extract_parser_data(parse_c_content: str) -> ParserData:
    """Extract parser data from Lemon-generated parse.c.

    Args:
        parse_c_content: Content of the generated parse.c file.

    Returns:
        ParserData containing all extracted sections.
    """
    # Array patterns: (field_name, regex_pattern)
    arrays = [
        ("fallback_table", r"static const YYCODETYPE yyFallback\[\]"),
        ("token_names", r"static const char \*const yyTokenName\[\]"),
        ("rule_names", r"static const char \*const yyRuleName\[\]"),
        ("rule_info_lhs", r"static const YYCODETYPE yyRuleInfoLhs\[\]"),
        ("rule_info_nrhs", r"static const signed char yyRuleInfoNRhs\[\]"),
    ]

    return ParserData(
        token_defines="\n".join(re.findall(r"^#define SYNTAQLITE_TOKEN_\w+\s+\d+", parse_c_content, re.MULTILINE)),
        control_defines=extract_section(parse_c_content, CONTROL_DEFS_START, CONTROL_DEFS_END),
        parsing_tables=extract_section(parse_c_content, PARSING_TABLES_START, PARSING_TABLES_END),
        reduce_actions=extract_section(parse_c_content, REDUCE_ACTIONS_START, REDUCE_ACTIONS_END),
        struct_defs=extract_struct_defs(parse_c_content),
        **{name: extract_array(parse_c_content, pattern) for name, pattern in arrays},
    )


def format_parser_data_header(data: ParserData) -> str:
    """Format extracted parser data as a header file.

    Args:
        data: Extracted parser data.

    Returns:
        Content formatted as a C header file.
    """
    parts = []

    # Includes needed by control defines (YYMINORTYPE references AST types)
    # and by reduce actions (AST builder function calls).
    parts.append('#include "src/common/synq_sqlite_defs.h"\n')
    parts.append('#include "src/parser/ast_base.h"\n')
    parts.append('#include "src/parser/ast_builder_gen.h"\n\n')

    # Note: token defines are NOT emitted here — they come from
    # generate_token_defines() which is the canonical source.

    parts.append("/* Control defines */\n")
    if data.control_defines:
        parts.append(data.control_defines)
        parts.append("\n\n")

    parts.append("/* Parser struct definitions */\n")
    if data.struct_defs:
        parts.append(data.struct_defs)
        parts.append("\n\n")

    parts.append("/* Parsing tables */\n")
    if data.parsing_tables:
        parts.append(data.parsing_tables)
        parts.append("\n\n")

    parts.append("/* Fallback table */\n")
    parts.append("#ifdef YYFALLBACK\n")
    if data.fallback_table:
        parts.append(data.fallback_table)
    parts.append("\n#endif /* YYFALLBACK */\n\n")

    parts.append("/* Token names (debug) */\n")
    parts.append("#if defined(YYCOVERAGE) || !defined(NDEBUG)\n")
    if data.token_names:
        parts.append(data.token_names)
    parts.append("\n#endif\n\n")

    parts.append("/* Rule names (debug) */\n")
    parts.append("#ifndef NDEBUG\n")
    if data.rule_names:
        parts.append(data.rule_names)
    parts.append("\n#endif\n\n")

    parts.append("/* Rule info */\n")
    if data.rule_info_lhs:
        parts.append(data.rule_info_lhs)
        parts.append("\n\n")
    if data.rule_info_nrhs:
        parts.append(data.rule_info_nrhs)
        parts.append("\n\n")

    return "".join(parts)


def format_extension_reduce_function(data: ParserData, prefix: str = "synq") -> str:
    """Format the extension reduce function.

    Args:
        data: Extracted parser data containing reduce actions.
        prefix: Symbol prefix for renamed lemon internals.

    Returns:
        The synq_parser_reduce function definition.
    """
    sqlite3_prefix = f"{prefix}_sqlite3"
    return f'''#ifndef yytestcase
# define yytestcase(X)
#endif

/* Reduce function */
static inline void {prefix}_parser_reduce(
  {prefix}_yyParser *yypParser,
  unsigned int yyruleno,
  {prefix}_yyStackEntry *yymsp,
  int yyLookahead,
  {sqlite3_prefix}ParserTOKENTYPE yyLookaheadToken
  {sqlite3_prefix}ParserCTX_PDECL
) {{
  {sqlite3_prefix}ParserARG_FETCH
  (void)yypParser;
  (void)yyLookahead;
  (void)yyLookaheadToken;
  (void)yymsp;

  switch( yyruleno ){{
{data.reduce_actions}
  }};
}}
'''


