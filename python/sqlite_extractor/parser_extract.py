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
    # Find the closing }; for the array
    brace_count = 0
    in_array = False
    end = start

    for i, char in enumerate(content[start:], start):
        if char == "{":
            brace_count += 1
            in_array = True
        elif char == "}":
            brace_count -= 1
            if in_array and brace_count == 0:
                # Find the semicolon
                semicolon = content.find(";", i)
                if semicolon != -1:
                    end = semicolon + 1
                else:
                    end = i + 1
                break

    return content[start:end].strip()


def extract_parser_data(parse_c_content: str) -> ParserData:
    """Extract parser data from Lemon-generated parse.c.

    Args:
        parse_c_content: Content of the generated parse.c file.

    Returns:
        ParserData containing all extracted sections.
    """
    data = ParserData()

    # Extract token definitions (#define TK_*)
    token_defines = []
    for match in re.finditer(r"^#define TK_\w+\s+\d+", parse_c_content, re.MULTILINE):
        token_defines.append(match.group(0))
    data.token_defines = "\n".join(token_defines)

    # Extract control #defines
    control_section = extract_section(
        parse_c_content,
        CONTROL_DEFS_START,
        CONTROL_DEFS_END,
    )
    data.control_defines = control_section

    # Extract parsing tables
    tables_section = extract_section(
        parse_c_content,
        PARSING_TABLES_START,
        PARSING_TABLES_END,
    )
    data.parsing_tables = tables_section

    # Extract fallback table
    fallback = extract_array(
        parse_c_content,
        r"static const YYCODETYPE yyFallback\[\]",
    )
    data.fallback_table = fallback

    # Extract token names
    token_names = extract_array(
        parse_c_content,
        r"static const char \*const yyTokenName\[\]",
    )
    data.token_names = token_names

    # Extract rule names
    rule_names = extract_array(
        parse_c_content,
        r"static const char \*const yyRuleName\[\]",
    )
    data.rule_names = rule_names

    # Extract rule info LHS
    rule_lhs = extract_array(
        parse_c_content,
        r"static const YYCODETYPE yyRuleInfoLhs\[\]",
    )
    data.rule_info_lhs = rule_lhs

    # Extract rule info NRhs
    rule_nrhs = extract_array(
        parse_c_content,
        r"static const signed char yyRuleInfoNRhs\[\]",
    )
    data.rule_info_nrhs = rule_nrhs

    # Extract reduce actions (the switch cases)
    reduce_section = extract_section(
        parse_c_content,
        REDUCE_ACTIONS_START,
        REDUCE_ACTIONS_END,
    )
    data.reduce_actions = reduce_section

    return data


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


def generate_extension_parser_header(data: ParserData) -> str:
    """Generate complete extension parser header content.

    This creates the content for a header file that can be included
    when SYNTAQLITE_PARSER_DATA_FILE is defined.

    Args:
        data: Extracted parser data.

    Returns:
        Complete header file content.
    """
    parts = []

    # Header comment
    parts.append("/*\n")
    parts.append("** Extension parser data for syntaqlite.\n")
    parts.append("** Include this file via -DSYNTAQLITE_PARSER_DATA_FILE\n")
    parts.append("*/\n\n")

    # All the data
    parts.append(format_parser_data_header(data))

    # The reduce function
    parts.append(format_extension_reduce_function(data))

    return "".join(parts)
