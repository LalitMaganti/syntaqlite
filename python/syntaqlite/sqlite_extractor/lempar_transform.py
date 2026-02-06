# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Transform lempar.c into syntaqlite templates.

This module transforms Lemon's lempar.c template into two variants:
1. synq_lempar.c - Base parser template with #ifdef guards for injection
2. synq_extension_lempar.c - Extension template that outputs data as header

The templates allow grammar data to be injected at compile time, enabling
dialect extensibility without recompiling the parser logic.
"""

from __future__ import annotations

import re

# Lemon-generated section markers
TOKEN_DEFS_START = "/* These constants specify the various numeric values for terminal symbols."
TOKEN_DEFS_END = "/**************** End token definitions ***************************************/"
CONTROL_DEFS_START = "/************* Begin control #defines *****************************************/"
CONTROL_DEFS_END = "/************* End control #defines *******************************************/"
PARSING_TABLES_START = "/*********** Begin parsing tables **********************************************/"
PARSING_TABLES_END = "/********** End of lemon-generated parsing tables *****************************/"
REDUCE_ACTIONS_START = "/********** Begin reduce actions **********************************************/"
REDUCE_ACTIONS_END = "/********** End reduce actions ************************************************/"

# Array declarations
FALLBACK_ARRAY_DECL = "static const YYCODETYPE yyFallback[] = {"
TOKEN_NAMES_ARRAY_DECL = "static const char *const yyTokenName[] = {"
RULE_NAMES_ARRAY_DECL = "static const char *const yyRuleName[] = {"
RULE_INFO_LHS_ARRAY_DECL = "static const YYCODETYPE yyRuleInfoLhs[] = {"
RULE_INFO_NRHS_ARRAY_DECL = "static const signed char yyRuleInfoNRhs[] = {"

# Preprocessor guards and macros
EXTERNAL_PARSER_GUARD = "_SYNQ_EXTERNAL_PARSER"
PARSER_DATA_FILE_MACRO = "SYNQ_PARSER_DATA_FILE"

# Conditional compilation markers
YYFALLBACK_IFDEF = "#ifdef YYFALLBACK"
YYCOVERAGE_NDEBUG_IFDEF = "#if defined(YYCOVERAGE) || !defined(NDEBUG)"
NDEBUG_IFNDEF = "#ifndef NDEBUG"


def transform_to_base_template(lempar_content: str) -> str:
    """Transform lempar.c into base parser template with injection points.

    This creates synq_lempar.c which:
    1. Has #ifdef SYNQ_PARSER_DATA_FILE at the top to include external data
    2. Wraps data sections with #ifndef _SYNQ_EXTERNAL_PARSER
    3. Adds default fallthrough in reduce switch to call synq_extension_reduce()

    Args:
        lempar_content: Content of original lempar.c.

    Returns:
        Transformed template content.
    """
    content = lempar_content

    # Add injection point include at the very top (after initial comment block)
    injection_header = f'''
/*
** Synq parser injection support.
** When {PARSER_DATA_FILE_MACRO} is defined, external grammar data is used.
*/
#ifdef {PARSER_DATA_FILE_MACRO}
#include {PARSER_DATA_FILE_MACRO}
#define {EXTERNAL_PARSER_GUARD} 1
#endif

'''

    # Find end of initial comment block (first */)
    first_comment_end = content.find("*/")
    if first_comment_end != -1:
        insert_pos = first_comment_end + 2
        # Skip any whitespace after comment
        while insert_pos < len(content) and content[insert_pos] in " \t\n":
            insert_pos += 1
        content = content[:insert_pos] + injection_header + content[insert_pos:]

    # Wrap data sections with #ifndef _SYNQ_EXTERNAL_PARSER
    content = _wrap_data_sections(content)

    # Transform reduce switch to add extension fallthrough
    content = _transform_reduce_switch(content)

    return content


def _wrap_data_sections(content: str) -> str:
    """Wrap grammar data sections with #ifndef guards."""
    # Wrap the token definitions section
    content = _wrap_section(content, TOKEN_DEFS_START, TOKEN_DEFS_END)

    # Wrap control #defines section
    content = _wrap_section(content, CONTROL_DEFS_START, CONTROL_DEFS_END)

    # Wrap parsing tables section
    content = _wrap_section(content, PARSING_TABLES_START, PARSING_TABLES_END)

    # Wrap yyFallback array (it's inside #ifdef YYFALLBACK, so handle specially)
    content = _wrap_section(
        content, FALLBACK_ARRAY_DECL, "};", after_marker=YYFALLBACK_IFDEF
    )

    # Wrap yyTokenName array
    content = _wrap_section(
        content, TOKEN_NAMES_ARRAY_DECL, "};", after_marker=YYCOVERAGE_NDEBUG_IFDEF
    )

    # Wrap yyRuleName array
    content = _wrap_section(
        content, RULE_NAMES_ARRAY_DECL, "};", after_marker=NDEBUG_IFNDEF
    )

    # Wrap yyRuleInfoLhs array
    content = _wrap_section(content, RULE_INFO_LHS_ARRAY_DECL, "};")

    # Wrap yyRuleInfoNRhs array
    content = _wrap_section(content, RULE_INFO_NRHS_ARRAY_DECL, "};")

    return content


def _wrap_section(
    content: str,
    start_marker: str,
    end_marker: str,
    after_marker: str | None = None,
) -> str:
    """Wrap a section with #ifndef _SYNQ_EXTERNAL_PARSER guards.

    Args:
        content: The content to modify.
        start_marker: Text marking the start of the section.
        end_marker: Text marking the end of the section.
        after_marker: Optional marker that must appear before start_marker.

    Returns:
        Modified content with section wrapped.
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

    # Build the wrapped section
    before = content[:line_start]
    section = content[line_start:line_end]
    after = content[line_end:]

    wrapped = (
        f"#ifndef {EXTERNAL_PARSER_GUARD}\n"
        + section
        + f"\n#endif /* !{EXTERNAL_PARSER_GUARD} */"
    )

    return before + wrapped + after


def _transform_reduce_switch(content: str) -> str:
    """Transform the reduce switch to add extension fallthrough.

    Wraps the reduce actions section with preprocessor guards and adds
    a default case that calls synq_extension_reduce() when using
    external parser data.
    """
    begin_pos = content.find(REDUCE_ACTIONS_START)
    if begin_pos == -1:
        return content

    end_pos = content.find(REDUCE_ACTIONS_END, begin_pos)
    if end_pos == -1:
        return content

    # Verify we're inside a switch(yyruleno) statement
    switch_search_start = max(0, begin_pos - 200)
    if not re.search(r"switch\s*\(\s*yyruleno\s*\)\s*\{", content[switch_search_start:begin_pos]):
        return content

    # Find the closing }; after the reduce section
    close_search_start = end_pos + len(REDUCE_ACTIONS_END)
    close_match = re.search(r"\s*\};", content[close_search_start:close_search_start + 50])
    if not close_match:
        return content

    # Find line boundaries around the reduce section
    line_start = content.rfind("\n", 0, begin_pos) + 1
    line_end = content.find("\n", end_pos)
    if line_end == -1:
        line_end = len(content)

    # Extract the reduce section and wrap it
    reduce_section = content[line_start:line_end + 1]
    transformed = (
        f"#ifndef {EXTERNAL_PARSER_GUARD}\n"
        f"{reduce_section}"
        f"#else\n"
        f"  default:\n"
        f"    synq_extension_reduce(yypParser, yyruleno, yymsp, yyLookahead,\n"
        f"                                yyLookaheadToken ParseCTX_PARAM);\n"
        f"    break;\n"
        f"#endif /* {EXTERNAL_PARSER_GUARD} */\n"
    )

    close_pos = close_search_start + close_match.start()
    return content[:line_start] + transformed + content[close_pos:]


