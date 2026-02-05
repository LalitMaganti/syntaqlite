# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Transform lempar.c into syntaqlite templates.

This module transforms Lemon's lempar.c template into two variants:
1. syntaqlite_lempar.c - Base parser template with #ifdef guards for injection
2. syntaqlite_extension_lempar.c - Extension template that outputs data as header

The templates allow grammar data to be injected at compile time, enabling
dialect extensibility without recompiling the parser logic.
"""

from __future__ import annotations

import re
from pathlib import Path

from .constants import (
    TOKEN_DEFS_START,
    TOKEN_DEFS_END,
    CONTROL_DEFS_START,
    CONTROL_DEFS_END,
    PARSING_TABLES_START,
    PARSING_TABLES_END,
    REDUCE_ACTIONS_START,
    REDUCE_ACTIONS_END,
    FALLBACK_ARRAY_DECL,
    TOKEN_NAMES_ARRAY_DECL,
    RULE_NAMES_ARRAY_DECL,
    RULE_INFO_LHS_ARRAY_DECL,
    RULE_INFO_NRHS_ARRAY_DECL,
    EXTERNAL_PARSER_GUARD,
    PARSER_DATA_FILE_MACRO,
    YYFALLBACK_IFDEF,
    YYCOVERAGE_NDEBUG_IFDEF,
    NDEBUG_IFNDEF,
)


def transform_to_base_template(lempar_content: str) -> str:
    """Transform lempar.c into base parser template with injection points.

    This creates syntaqlite_lempar.c which:
    1. Has #ifdef SYNTAQLITE_PARSER_DATA_FILE at the top to include external data
    2. Wraps data sections with #ifndef _SYNTAQLITE_EXTERNAL_PARSER
    3. Adds default fallthrough in reduce switch to call syntaqlite_extension_reduce()

    Args:
        lempar_content: Content of original lempar.c.

    Returns:
        Transformed template content.
    """
    content = lempar_content

    # Add injection point include at the very top (after initial comment block)
    injection_header = f'''
/*
** Syntaqlite parser injection support.
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

    # Wrap data sections with #ifndef _SYNTAQLITE_EXTERNAL_PARSER
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
    """Wrap a section with #ifndef _SYNTAQLITE_EXTERNAL_PARSER guards.

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

    Changes:
    ```
    switch( yyruleno ){
    /********** Begin reduce actions ***********/
    %%
    /********** End reduce actions *************/
    };
    ```

    To:
    ```
    switch( yyruleno ){
    #ifndef _SYNTAQLITE_EXTERNAL_PARSER
    /********** Begin reduce actions ***********/
    %%
    /********** End reduce actions *************/
    #else
      default:
        syntaqlite_extension_reduce(yypParser, yyruleno, yymsp, yyLookahead,
                                    yyLookaheadToken ParseCTX_PARAM);
        break;
    #endif
    };
    ```
    """
    # Find the reduce actions section
    begin_marker = REDUCE_ACTIONS_START
    end_marker = REDUCE_ACTIONS_END

    begin_pos = content.find(begin_marker)
    if begin_pos == -1:
        return content

    end_pos = content.find(end_marker, begin_pos)
    if end_pos == -1:
        return content

    # Find the switch statement before begin_marker
    switch_search_start = max(0, begin_pos - 200)
    switch_pattern = r"switch\s*\(\s*yyruleno\s*\)\s*\{"
    switch_match = re.search(switch_pattern, content[switch_search_start:begin_pos])
    if not switch_match:
        return content

    # Find the closing brace and semicolon after end_marker
    close_search_start = end_pos + len(end_marker)
    # Look for }; pattern that closes the switch
    close_match = re.search(r"\s*\};", content[close_search_start:close_search_start + 50])
    if not close_match:
        return content

    # Calculate actual positions
    switch_pos = switch_search_start + switch_match.end()  # After the {
    close_pos = close_search_start + close_match.start()  # Before the };

    # Find start of line containing begin_marker
    line_start = content.rfind("\n", 0, begin_pos) + 1

    # Find end of line containing end_marker
    line_end = content.find("\n", end_pos)
    if line_end == -1:
        line_end = len(content)

    # Build the transformed section
    before = content[:line_start]
    reduce_section = content[line_start : line_end + 1]
    after = content[close_pos:]  # Includes the };

    transformed = (
        f"#ifndef {EXTERNAL_PARSER_GUARD}\n"
        + reduce_section
        + "#else\n"
        + "  default:\n"
        + "    syntaqlite_extension_reduce(yypParser, yyruleno, yymsp, yyLookahead,\n"
        + "                                yyLookaheadToken ParseCTX_PARAM);\n"
        + "    break;\n"
        + f"#endif /* {EXTERNAL_PARSER_GUARD} */\n"
    )

    return before + transformed + after


def transform_to_extension_template(lempar_content: str) -> str:
    """Transform lempar.c into extension header template.

    This creates syntaqlite_extension_lempar.c which, when processed by Lemon,
    outputs a header file containing:
    1. Token definitions (#define TK_*)
    2. Control defines (YYCODETYPE, YYNSTATE, etc.)
    3. All parser tables as static arrays
    4. syntaqlite_extension_reduce() function wrapping the reduce switch

    Args:
        lempar_content: Content of original lempar.c.

    Returns:
        Extension template content.
    """
    # This is a more complex transformation that essentially restructures
    # lempar.c to output a header file format instead of a source file.
    # For now, we'll create a minimal template that can be expanded later.

    template = '''/*
** Extension parser template for syntaqlite.
** When processed by Lemon, this generates a header file containing
** all grammar data and the syntaqlite_extension_reduce() function.
**
** Usage: lemon -T syntaqlite_extension_lempar.c extension_grammar.y
*/

/************ Begin %include sections from the grammar ************************/
%%
/**************** End of %include directives **********************************/

/* Token definitions */
%%

/* Control defines */
#ifndef INTERFACE
# define INTERFACE 1
#endif
%%

/* Parser tables */
%%

/* Fallback table */
#ifdef YYFALLBACK
static const YYCODETYPE yyFallback[] = {
%%
};
#endif /* YYFALLBACK */

/* Token names (for debug) */
#if defined(YYCOVERAGE) || !defined(NDEBUG)
static const char *const yyTokenName[] = {
%%
};
#endif /* defined(YYCOVERAGE) || !defined(NDEBUG) */

/* Rule names (for debug) */
#ifndef NDEBUG
static const char *const yyRuleName[] = {
%%
};
#endif /* NDEBUG */

/* Rule info */
static const YYCODETYPE yyRuleInfoLhs[] = {
%%
};

static const signed char yyRuleInfoNRhs[] = {
%%
};

/* Extension reduce function */
static void syntaqlite_extension_reduce(
  yyParser *yypParser,
  unsigned int yyruleno,
  yyStackEntry *yymsp,
  int yyLookahead,
  ParseTOKENTYPE yyLookaheadToken
  ParseCTX_PDECL
) {
  ParseARG_FETCH
  (void)yyLookahead;
  (void)yyLookaheadToken;
  (void)yymsp;

  switch( yyruleno ){
%%
  }
}
'''
    return template


def generate_templates(lempar_path: Path, output_dir: Path) -> tuple[Path, Path]:
    """Generate both syntaqlite templates from lempar.c.

    Args:
        lempar_path: Path to original lempar.c.
        output_dir: Directory to write templates.

    Returns:
        Tuple of (base_template_path, extension_template_path).
    """
    lempar_content = lempar_path.read_text()

    # Generate base template
    base_content = transform_to_base_template(lempar_content)
    base_path = output_dir / "syntaqlite_lempar.c"
    base_path.write_text(base_content)
    print(f"  Written: {base_path}")

    # Generate extension template
    ext_content = transform_to_extension_template(lempar_content)
    ext_path = output_dir / "syntaqlite_extension_lempar.c"
    ext_path.write_text(ext_content)
    print(f"  Written: {ext_path}")

    return base_path, ext_path
