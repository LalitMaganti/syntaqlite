# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Split Lemon-generated parser into three files.

Uses extract_parser_data() to pull grammar data from the stock Lemon output,
then assembles:
  - sqlite_parse_defs.h   — grammar-independent struct definitions
  - sqlite_parse_tables.h — grammar data + synq_parser_reduce()
  - sqlite_parse_gen.c    — parsing engine functions (includes tables.h)
"""

from __future__ import annotations

import re

from .parser_extract import (
    ParserData,
    extract_parser_data,
    format_extension_reduce_function,
    # Section markers
    TOKEN_DEFS_START,
    TOKEN_DEFS_END,
    CONTROL_DEFS_COMMENT_START,
    CONTROL_DEFS_END,
    PARSING_TABLES_COMMENT_START,
    PARSING_TABLES_END,
    FALLBACK_COMMENT_START,
    REDUCE_ACTIONS_START,
    REDUCE_ACTIONS_END,
    STRUCT_COMMENT_START,
    INCLUDE_END,
)
from .transforms import (
    Pipeline,
    ReplaceText,
    ReplaceRegex,
    RemoveSection,
)


def split_parser_output(
    content: str,
    prefix: str = "synq",
) -> tuple[str, str, str]:
    """Split Lemon-generated parser into three files.

    Args:
        content: Lemon-generated parser (stock template, after symbol renaming).
        prefix: Symbol prefix (e.g. "synq").

    Returns:
        Tuple of (defs_h, tables_h, engine_c) content strings.
    """
    data = extract_parser_data(content)

    if not data.struct_defs:
        raise ValueError("Could not extract struct definitions from parser output")

    defs_h = _build_defs_h(data, prefix)
    tables_h = _build_tables_h(data, prefix)
    engine_c = _build_engine_c(content, data, prefix)

    return defs_h, tables_h, engine_c


def _build_defs_h(data: ParserData, prefix: str) -> str:
    """Build sqlite_parse_defs.h — control defines + parser struct definitions."""
    guard = f"{prefix.upper()}_SRC_PARSER_SQLITE_PARSE_DEFS_H"

    parts = []
    parts.append(f"#ifndef {guard}\n")
    parts.append(f"#define {guard}\n\n")

    # Includes needed by control defines (YYMINORTYPE references AST types).
    parts.append('#include "src/common/synq_sqlite_defs.h"\n')
    parts.append('#include "src/parser/ast_base.h"\n')
    parts.append('#include "src/parser/ast_builder_gen.h"\n\n')

    parts.append("/* Control defines */\n")
    if data.control_defines:
        parts.append(data.control_defines)
        parts.append("\n\n")

    parts.append("/* Parser struct definitions */\n")
    parts.append(data.struct_defs)
    parts.append("\n\n")

    parts.append(f"#endif /* {guard} */\n")

    return "".join(parts)


def _build_tables_h(data: ParserData, prefix: str) -> str:
    """Build sqlite_parse_tables.h from grammar data sections."""
    guard = f"{prefix.upper()}_SRC_PARSER_SQLITE_PARSE_TABLES_H"

    parts = []
    parts.append(f"#ifndef {guard}\n")
    parts.append(f"#define {guard}\n\n")

    # defs.h provides control defines, struct definitions, and their
    # transitive includes (synq_sqlite_defs.h, ast_base.h, ast_builder_gen.h).
    parts.append('#include "src/parser/sqlite_parse_defs.h"\n\n')

    parts.append("/* Token definitions */\n")
    if data.token_defines:
        parts.append(data.token_defines)
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

    parts.append(format_extension_reduce_function(data, prefix))
    parts.append("\n")

    parts.append(f"#endif /* {guard} */\n")

    return "".join(parts)


def _build_engine_c(
    content: str,
    data: ParserData,
    prefix: str,
) -> str:
    """Build sqlite_parse_gen.c by removing data sections from the Lemon output.

    Uses a Pipeline of transforms — no ad-hoc string operations.
    """
    sqlite3_prefix = f"{prefix}_sqlite3"
    struct_typedef = f"typedef struct {prefix}_yyParser {prefix}_yyParser;"

    reduce_call = (
        f"    default:\n"
        f"      {prefix}_parser_reduce(yypParser, yyruleno, yymsp, yyLookahead,\n"
        f"                              yyLookaheadToken {sqlite3_prefix}ParserCTX_PARAM);\n"
        f"      break;"
    )

    conditional_include = (
        "\n"
        "#ifdef SYNTAQLITE_EXTENSION_GRAMMAR\n"
        "#include SYNTAQLITE_EXTENSION_GRAMMAR\n"
        "#else\n"
        '#include "src/parser/sqlite_parse_tables.h"\n'
        "#endif\n"
    )

    pipeline = Pipeline([
        # --- Insert conditional include after %include directives ---
        ReplaceText(
            INCLUDE_END + "\n",
            INCLUDE_END + "\n" + conditional_include,
            count=1,
        ),

        # --- Remove data sections (moved to tables.h) ---
        RemoveSection(TOKEN_DEFS_START, TOKEN_DEFS_END),
        RemoveSection(CONTROL_DEFS_COMMENT_START, CONTROL_DEFS_END),
        ReplaceText("#ifndef INTERFACE\n# define INTERFACE 1\n#endif\n", ""),
        RemoveSection(PARSING_TABLES_COMMENT_START, PARSING_TABLES_END),
        RemoveSection(FALLBACK_COMMENT_START, "#endif /* YYFALLBACK */"),

        # --- Remove struct definitions (moved to defs.h) ---
        RemoveSection(STRUCT_COMMENT_START, struct_typedef),

        # --- Remove arrays (leave their preprocessor guards empty but valid) ---
        ReplaceText(data.token_names, ""),
        ReplaceText(data.rule_names, ""),
        RemoveSection("/* For rule J, yyRuleInfoLhs[J]",
                       data.rule_info_lhs.split("\n")[-1]),
        RemoveSection("/* For rule J, yyRuleInfoNRhs[J]",
                       data.rule_info_nrhs.split("\n")[-1]),

        # --- Replace reduce switch body with function call ---
        # First remove the reduce actions between their markers
        RemoveSection(REDUCE_ACTIONS_START, REDUCE_ACTIONS_END),
        # Then remove the "Beginning here are the reduction cases" comment
        RemoveSection("/* Beginning here are the reduction cases.", "*/"),
        # Now the switch body is empty whitespace — replace it
        ReplaceRegex(
            r"(  switch\( yyruleno \)\{)\s*(\};)",
            r"\1\n" + reduce_call + r"\n  \2",
            flags=re.DOTALL,
        ),

        # --- Collapse excessive blank lines ---
        ReplaceRegex(r"\n{4,}", "\n\n\n"),
    ])

    return pipeline.apply(content)
