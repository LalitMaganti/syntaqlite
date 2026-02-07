# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Transform lempar.c into syntaqlite templates.

This module transforms Lemon's lempar.c template to add split markers around
each data section. After Lemon generates the parser, parser_split.py uses
these markers to split the output into three files:
  - sqlite_parse_defs.h   (struct definitions)
  - sqlite_parse_tables.h (grammar data + reduce function)
  - sqlite_parse_gen.c    (parsing engine)
"""

from __future__ import annotations

import re

# Lemon-generated section markers
TOKEN_DEFS_START = "/* These constants specify the various numeric values for terminal symbols."
TOKEN_DEFS_END = "/**************** End token definitions ***************************************/"
CONTROL_DEFS_START = "/************* Begin control #defines *****************************************/"
CONTROL_DEFS_END = "/************* End control #defines *******************************************/"
PARSING_TABLES_START = "*********** Begin parsing tables **********************************************/"
PARSING_TABLES_END = "/********** End of lemon-generated parsing tables *****************************/"
REDUCE_ACTIONS_START = "/********** Begin reduce actions **********************************************/"
REDUCE_ACTIONS_END = "/********** End reduce actions ************************************************/"

# Array declarations
FALLBACK_ARRAY_DECL = "static const YYCODETYPE yyFallback[] = {"
TOKEN_NAMES_ARRAY_DECL = "static const char *const yyTokenName[] = {"
RULE_NAMES_ARRAY_DECL = "static const char *const yyRuleName[] = {"
RULE_INFO_LHS_ARRAY_DECL = "static const YYCODETYPE yyRuleInfoLhs[] = {"
RULE_INFO_NRHS_ARRAY_DECL = "static const signed char yyRuleInfoNRhs[] = {"

# Conditional compilation markers
YYFALLBACK_IFDEF = "#ifdef YYFALLBACK"
YYCOVERAGE_NDEBUG_IFDEF = "#if defined(YYCOVERAGE) || !defined(NDEBUG)"
NDEBUG_IFNDEF = "#ifndef NDEBUG"

# Struct markers
STRUCT_COMMENT = "/* The following structure represents a single element of the"
STRUCT_END = "typedef struct yyParser yyParser;"


def transform_for_split(lempar_content: str) -> str:
    """Transform lempar.c by adding split markers around data sections.

    This creates a template that, after Lemon processes it, produces output
    with clearly marked sections that parser_split.py can extract.

    Markers use the format: /* SYNQ:<category>:<name>:BEGIN/END */

    Args:
        lempar_content: Content of original lempar.c.

    Returns:
        Transformed template content with split markers.
    """
    content = lempar_content

    # Mark token definitions
    content = _add_marker(content, "DATA", "token_defs",
                          TOKEN_DEFS_START, TOKEN_DEFS_END)

    # Mark control defines
    content = _add_marker(content, "DATA", "control_defs",
                          CONTROL_DEFS_START, CONTROL_DEFS_END)

    # Mark parsing tables
    content = _add_marker(content, "DATA", "tables",
                          PARSING_TABLES_START, PARSING_TABLES_END)

    # Mark fallback array (inside #ifdef YYFALLBACK)
    content = _add_marker(content, "DATA", "fallback",
                          FALLBACK_ARRAY_DECL, "};",
                          after_marker=YYFALLBACK_IFDEF)

    # Mark token names array (inside debug guard)
    content = _add_marker(content, "DATA", "token_names",
                          TOKEN_NAMES_ARRAY_DECL, "};",
                          after_marker=YYCOVERAGE_NDEBUG_IFDEF)

    # Mark rule names array (inside debug guard)
    content = _add_marker(content, "DATA", "rule_names",
                          RULE_NAMES_ARRAY_DECL, "};",
                          after_marker=NDEBUG_IFNDEF)

    # Mark rule info arrays
    content = _add_marker(content, "DATA", "rule_info_lhs",
                          RULE_INFO_LHS_ARRAY_DECL, "};")

    content = _add_marker(content, "DATA", "rule_info_nrhs",
                          RULE_INFO_NRHS_ARRAY_DECL, "};")

    # Mark struct definitions (yyStackEntry + yyParser)
    content = _add_marker(content, "STRUCTS", "defs",
                          STRUCT_COMMENT, STRUCT_END)

    # Mark reduce actions (the switch body inside yy_reduce)
    content = _add_marker(content, "REDUCE", "actions",
                          REDUCE_ACTIONS_START, REDUCE_ACTIONS_END)

    return content


def _add_marker(
    content: str,
    category: str,
    name: str,
    start_marker: str,
    end_marker: str,
    after_marker: str | None = None,
) -> str:
    """Add SYNQ split markers around a section.

    Args:
        content: The content to modify.
        category: Marker category (DATA, STRUCTS, REDUCE).
        name: Section name within the category.
        start_marker: Text marking the start of the section.
        end_marker: Text marking the end of the section.
        after_marker: Optional marker that must appear before start_marker.

    Returns:
        Modified content with markers added.
    """
    search_start = 0
    if after_marker:
        after_pos = content.find(after_marker, search_start)
        if after_pos == -1:
            return content
        search_start = after_pos

    start_pos = content.find(start_marker, search_start)
    if start_pos == -1:
        return content

    end_pos = content.find(end_marker, start_pos)
    if end_pos == -1:
        return content

    end_pos += len(end_marker)

    # Find the start of the line containing start_marker
    line_start = content.rfind("\n", 0, start_pos) + 1

    # Find the end of the line containing end_marker
    line_end = content.find("\n", end_pos)
    if line_end == -1:
        line_end = len(content)

    section = content[line_start:line_end]

    begin_tag = f"/* SYNQ:{category}:{name}:BEGIN */"
    end_tag = f"/* SYNQ:{category}:{name}:END */"

    marked = begin_tag + "\n" + section + "\n" + end_tag

    return content[:line_start] + marked + content[line_end:]
