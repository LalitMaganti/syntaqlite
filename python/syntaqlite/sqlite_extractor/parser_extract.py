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
CONTROL_DEFS_START = "/************* Begin control #defines *****************************************/"
CONTROL_DEFS_END = "/************* End control #defines *******************************************/"
PARSING_TABLES_START = "/*********** Begin parsing tables **********************************************/"
PARSING_TABLES_END = "/********** End of lemon-generated parsing tables *****************************/"
REDUCE_ACTIONS_START = "/********** Begin reduce actions **********************************************/"
REDUCE_ACTIONS_END = "/********** End reduce actions ************************************************/"


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
        token_defines="\n".join(re.findall(r"^#define TK_\w+\s+\d+", parse_c_content, re.MULTILINE)),
        control_defines=extract_section(parse_c_content, CONTROL_DEFS_START, CONTROL_DEFS_END),
        parsing_tables=extract_section(parse_c_content, PARSING_TABLES_START, PARSING_TABLES_END),
        reduce_actions=extract_section(parse_c_content, REDUCE_ACTIONS_START, REDUCE_ACTIONS_END),
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

    parts.append("/* Token definitions */\n")
    if data.token_defines:
        parts.append(data.token_defines)
        parts.append("\n\n")

    parts.append("/* Control defines */\n")
    if data.control_defines:
        parts.append(data.control_defines)
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


def format_extension_reduce_function(data: ParserData) -> str:
    """Format the extension reduce function.

    Args:
        data: Extracted parser data containing reduce actions.

    Returns:
        The syntaqlite_extension_reduce function definition.
    """
    return f'''/* Extension reduce function */
static void syntaqlite_extension_reduce(
  yyParser *yypParser,
  unsigned int yyruleno,
  yyStackEntry *yymsp,
  int yyLookahead,
  ParseTOKENTYPE yyLookaheadToken
  ParseCTX_PDECL
) {{
  ParseARG_FETCH
  (void)yyLookahead;
  (void)yyLookaheadToken;
  (void)yymsp;

  switch( yyruleno ){{
{data.reduce_actions}
  }};
}}
'''


