# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Extract SQLite code for syntaqlite.

This script extracts and transforms SQLite source code for use in syntaqlite.
Handles both the tokenizer and parser extraction.

Following Perfetto's approach:
1. Build SQLite tools (lemon, mkkeywordhash) using GN/ninja
2. Run tools to generate token definitions and keyword hash data
3. Copy and transform SQLite source files with minimal modifications
4. Generate helper header files

For the parser:
1. Extract clean grammar rules using lemon -g
2. Run Lemon with stock lempar.c template
3. Split output into defs.h, tables.h, and engine.c for extensibility

Usage:
    python3 python/tools/extract_sqlite.py

For external users who want custom keywords for SQLite dialects,
use tools/build-extension-grammar instead.
"""
import argparse
import sys
import tempfile
from pathlib import Path

# Add project root to path for imports
ROOT_DIR = Path(__file__).parent.parent.parent
if str(ROOT_DIR) not in sys.path:
    sys.path.insert(0, str(ROOT_DIR))

from python.syntaqlite.sqlite_extractor import (
    Pipeline,
    ReplaceText,
    TruncateAt,
    RemoveRegex,
    SymbolRename,
    SymbolRenameExact,
    RemoveFunctionCalls,
    StripBlessingComment,
    HeaderGenerator,
    SourceFileGenerator,
    ToolRunner,
    create_symbol_rename_pipeline,
    create_keywordhash_rename_pipeline,
    process_keywordhash,
    run_lemon_pipeline,
)
from python.syntaqlite.sqlite_extractor.generators import extract_token_defines
from python.syntaqlite.sqlite_extractor.grammar_build import (
    build_synq_grammar,
)
from python.syntaqlite.sqlite_extractor.parser_split import (
    split_parser_output,
)
from python.syntaqlite.ast_codegen import codegen as ast_codegen
from python.syntaqlite.ast_codegen import fmt_codegen as ast_fmt_codegen
from python.syntaqlite.ast_codegen import validator as ast_validator
from src.parser.nodes import NODES as AST_NODES, ENUMS as AST_ENUMS, FLAGS as AST_FLAGS

SQLITE_SRC = ROOT_DIR / "third_party" / "src" / "sqlite" / "src"
OUTPUT_DIR = ROOT_DIR / "src"

# Default prefix for renamed sqlite3 symbols to avoid clashes
DEFAULT_PREFIX = "synq"

REGENERATE_CMD = "python3 python/tools/extract_sqlite.py"

# Module-level verbose flag, set by main()
_verbose = False


def log(msg: str) -> None:
    """Print a message only when verbose mode is enabled."""
    if _verbose:
        print(msg)


def build_keywordhash(
    runner: ToolRunner,
    prefix: str,
) -> tuple[str, str]:
    """Build keyword hash data.

    Returns:
        Tuple of (data_section, keyword_code_func) to be inlined into sqlite_tokenize.c.
    """
    output = runner.run_mkkeywordhash()
    result = process_keywordhash(output, prefix)
    return result.data_section, result.keyword_code_func


def copy_tokenize_c(
    runner: ToolRunner,
    output_path: Path,
    prefix: str,
    keywordhash_data: str,
    keyword_code_func: str,
) -> None:
    """Copy and transform tokenize.c."""
    sqlite3_prefix = f"{prefix}_sqlite3"
    content = (runner.sqlite_src / "tokenize.c").read_text()

    # Build the inlined header that replaces sqliteInt.h
    inlined_header = f'''#include "src/common/synq_sqlite_defs.h"
#include "src/tokenizer/sqlite_charmap_gen.h"

/*
** Synq tokenizer injection support.
** When SYNTAQLITE_EXTENSION_GRAMMAR is defined, external keyword data is used.
*/
#ifdef SYNTAQLITE_EXTENSION_GRAMMAR
#include SYNTAQLITE_EXTENSION_GRAMMAR
#define _SYNQ_EXTERNAL_KEYWORDHASH 1
#endif

#ifndef _SYNQ_EXTERNAL_KEYWORDHASH
#include "syntaqlite/sqlite_tokens_gen.h"
{keywordhash_data}
#endif /* _SYNQ_EXTERNAL_KEYWORDHASH */

/* Keyword lookup function — driven by hash tables + SYNQ_KW_HASH_* params */
{keyword_code_func}

/* Stub for parser fallback - not needed for pure tokenization */
static inline int {sqlite3_prefix}ParserFallback(int token) {{
  (void)token;
  return 0;
}}

/* Forward declaration */
i64 {sqlite3_prefix}GetToken(const unsigned char *z, int *tokenType);
'''

    # Build the transformation pipeline
    pipeline = Pipeline([
        # Truncate at parser code - we only want the tokenization functions
        TruncateAt("/*\n** Run the parser on the given SQL string."),

        # Replace the SQLite internal header with our includes
        ReplaceText('#include "sqliteInt.h"', inlined_header),

        # Remove includes we don't need
        ReplaceText('#include "keywordhash.h"\n', ''),
        ReplaceText('#include <stdlib.h>\n', ''),

        # Rename static analyze functions to be non-static with our prefix
        ReplaceText('static int analyzeWindowKeyword', f'int {sqlite3_prefix}AnalyzeWindowKeyword'),
        ReplaceText('static int analyzeOverKeyword', f'int {sqlite3_prefix}AnalyzeOverKeyword'),
        ReplaceText('static int analyzeFilterKeyword', f'int {sqlite3_prefix}AnalyzeFilterKeyword'),

        # Update call sites
        ReplaceText('analyzeWindowKeyword(', f'{sqlite3_prefix}AnalyzeWindowKeyword('),
        ReplaceText('analyzeOverKeyword(', f'{sqlite3_prefix}AnalyzeOverKeyword('),
        ReplaceText('analyzeFilterKeyword(', f'{sqlite3_prefix}AnalyzeFilterKeyword('),

        # Strip the SQLite blessing - we'll add our own header
        StripBlessingComment(),
    ])

    content = pipeline.apply(content)

    # Rename sqlite3* symbols (using safe C tokenizer)
    content = create_symbol_rename_pipeline(prefix).apply(content)

    # Rename TK_ token prefix to SYNTAQLITE_TOKEN_
    content = SymbolRename("TK_", "SYNTAQLITE_TOKEN_").apply(content)

    # Rename keywordCode to have our prefix
    content = SymbolRenameExact("keywordCode", f"{prefix}_keywordCode").apply(content)

    # Remove Testcase calls
    content = RemoveFunctionCalls(f"{sqlite3_prefix}Testcase").apply(content)

    # Generate the final file
    gen = SourceFileGenerator(
        description=f"SQLite tokenizer for {prefix}.\n** Extracted from SQLite's tokenize.c with minimal modifications.",
        regenerate_cmd=REGENERATE_CMD,
        verbose=_verbose,
    )
    gen.write(output_path, content)


def generate_token_defs(
    runner: ToolRunner,
    output_path: Path,
    prefix: str,
    parse_h_content: str | None = None,
) -> None:
    """Generate token definitions from SQLite's parse.y via Lemon.

    Args:
        runner: ToolRunner instance.
        output_path: Path to write sqlite_tokens.h.
        prefix: Symbol prefix.
        parse_h_content: Optional parse.h content from parser generation.
            If provided, uses these tokens directly. If None, runs Lemon
            on base grammar (for tokenizer-only extraction).
    """
    guard = "SYNTAQLITE_SQLITE_TOKENS_GEN_H"

    if parse_h_content is None:
        # Tokenizer-only mode: run Lemon on base grammar
        with tempfile.TemporaryDirectory() as tmpdir:
            tmpdir = Path(tmpdir)

            # Get grammar content
            base_grammar = runner.get_base_grammar()
            (tmpdir / "parse.y").write_text(base_grammar)

            # Run lemon
            parse_h = runner.run_lemon(tmpdir / "parse.y")
            parse_h_content = parse_h.read_text()

    # Extract SYNTAQLITE_TOKEN_* defines
    defines = extract_token_defines(parse_h_content)

    # Wrap in SYNTAQLITE_EXTENSION_GRAMMAR ifdef so dialects can substitute
    # their own token definitions and the wrong set can never leak.
    body_lines = [
        "#ifdef SYNTAQLITE_EXTENSION_GRAMMAR",
        "#include SYNTAQLITE_EXTENSION_GRAMMAR",
        "#else",
        "",
    ] + defines + [
        "",
        "#endif /* SYNTAQLITE_EXTENSION_GRAMMAR */",
    ]

    # Generate header
    gen = HeaderGenerator(
        guard=guard,
        description="Token definitions from SQLite's parse.y via Lemon.",
        regenerate_cmd=REGENERATE_CMD,
        verbose=_verbose,
    )
    gen.write(output_path, "\n".join(body_lines))


def copy_global_tables(runner: ToolRunner, output_path: Path, prefix: str) -> None:
    """Copy character tables from SQLite's global.c."""
    sqlite3_prefix = f"{prefix}_sqlite3"
    guard = f"{prefix.upper()}_SRC_TOKENIZER_SQLITE_CHARMAP_GEN_H"

    content = (runner.sqlite_src / "global.c").read_text()

    # Find sqlite3UpperToLower array - extract only ASCII version
    upper_start = content.find("const unsigned char sqlite3UpperToLower[]")
    upper_ascii_start = content.find("#ifdef SQLITE_ASCII", upper_start)
    upper_ascii_end = content.find("#endif", upper_ascii_start)
    # Get the data between #ifdef SQLITE_ASCII and #endif
    upper_data = content[upper_ascii_start:upper_ascii_end]
    # Skip the #ifdef line to get numeric data
    data_start = upper_data.find('\n') + 1
    upper_values_str = upper_data[data_start:].strip()

    # Find sqlite3CtypeMap array (it's the same for ASCII and EBCDIC)
    ctype_start = content.find("const unsigned char sqlite3CtypeMap[256]")
    ctype_end = content.find("};", ctype_start) + 2
    ctype_block = content[ctype_start:ctype_end]

    # Find the comment block before sqlite3CtypeMap
    ctype_comment_start = content.rfind("/*", 0, ctype_start)
    ctype_comment_end = content.find("*/", ctype_comment_start) + 2
    ctype_comment = content[ctype_comment_start:ctype_comment_end]

    # Build the tables content
    tables = f"""/*
** Upper-to-lower case conversion table.
** SQLite only considers US-ASCII characters.
*/
static const unsigned char {sqlite3_prefix}UpperToLower[] = {{
"""
    if upper_values_str:
        tables += upper_values_str

    tables += "\n};\n\n"

    # Add the ctype comment and map, renaming sqlite3CtypeMap
    tables += ctype_comment + "\n"
    ctype_renamed = ctype_block.replace("sqlite3CtypeMap", f"{sqlite3_prefix}CtypeMap")
    tables += "static " + ctype_renamed + "\n"

    # Extract character classification macros from sqliteInt.h
    sqliteint_content = (runner.sqlite_src / "sqliteInt.h").read_text()

    # Find the ASCII block of macros
    ascii_start = sqliteint_content.find("/*\n** The following macros mimic the standard library functions")
    if ascii_start == -1:
        print("Warning: Could not find character classification macros in sqliteInt.h", file=sys.stderr)
    else:
        # Find the #ifdef SQLITE_ASCII block
        ifdef_start = sqliteint_content.find("#ifdef SQLITE_ASCII", ascii_start)
        else_start = sqliteint_content.find("#else", ifdef_start)

        # Extract the comment and the ASCII macros
        macro_comment = sqliteint_content[ascii_start:sqliteint_content.find("*/", ascii_start) + 2]
        ascii_macros = sqliteint_content[ifdef_start + len("#ifdef SQLITE_ASCII"):else_start].strip()

        tables += "\n" + macro_comment + "\n"
        # Rename sqlite3 -> {prefix}_sqlite3 in the macros (safe renaming)
        ascii_macros_renamed = create_symbol_rename_pipeline(prefix).apply(ascii_macros)
        tables += ascii_macros_renamed + "\n"

    # Generate header
    gen = HeaderGenerator(
        guard=guard,
        description="Character tables extracted from SQLite's global.c",
        regenerate_cmd=REGENERATE_CMD,
        verbose=_verbose,
    )
    gen.write(output_path, tables)


def generate_parser(
    runner: ToolRunner,
    output_dir: Path,
    prefix: str,
) -> str:
    """Generate the SQLite parser split into three files.

    This function:
    1. Builds a clean grammar from SQLite's parse.y
    2. Runs Lemon with the stock lempar.c template
    3. Applies symbol renaming
    4. Splits into defs.h, tables.h, and engine.c

    Args:
        runner: ToolRunner instance for running tools.
        output_dir: Directory to write output files.
        prefix: Symbol prefix.

    Returns:
        The parse.h content from Lemon (for token generation).
    """
    log("\n=== Extracting Parser ===")

    # Step 1: Build the clean grammar
    log("Building clean grammar from SQLite's parse.y...")
    grammar_content = build_synq_grammar(runner, prefix)

    # Step 2-4: Run Lemon with template + symbol renaming (shared pipeline)
    log("Running Lemon pipeline...")
    parse_c_content, parse_h_content = run_lemon_pipeline(
        runner, grammar_content, prefix
    )

    # Step 5: Split into three files using extract + pipeline
    log("Splitting parser into defs.h, tables.h, and engine.c...")
    defs_h, tables_h, engine_c = split_parser_output(parse_c_content, prefix)

    # Step 6: Write output files
    output_dir.mkdir(parents=True, exist_ok=True)
    parser_dir = output_dir / "parser"

    source_gen = SourceFileGenerator(
        description=f"SQLite parser for {prefix}.\n** Generated from SQLite's parse.y via Lemon.",
        regenerate_cmd=REGENERATE_CMD,
        verbose=_verbose,
    )
    header_gen = HeaderGenerator(
        guard="",  # guards are already in the content
        description="",
        regenerate_cmd=REGENERATE_CMD,
        verbose=_verbose,
    )

    source_gen.write(parser_dir / "sqlite_parse_gen.c", engine_c)
    header_gen.write_raw(parser_dir / "sqlite_parse_defs.h", defs_h)
    header_gen.write_raw(parser_dir / "sqlite_parse_tables.h", tables_h)

    return parse_h_content


def generate_ast(output_dir: Path) -> None:
    """Generate AST code from node definitions.

    Args:
        output_dir: Directory to write output files.
    """
    log(f"  {len(AST_NODES)} node types")

    # Validate
    errors = ast_validator.validate_node_references(AST_NODES)
    if errors:
        print("  Validation errors:", file=sys.stderr)
        for error in errors:
            print(f"    - {error}", file=sys.stderr)
        raise RuntimeError("AST validation failed")

    # Generate
    ast_dir = output_dir / "parser"
    include_dir = output_dir.parent / "include" / "syntaqlite"
    ast_codegen.generate_all(AST_NODES, AST_ENUMS, ast_dir, flags_defs=AST_FLAGS,
                             public_header_dir=include_dir)

    # Generate formatter
    fmt_output = output_dir / "formatter" / "fmt_gen.c"
    ast_fmt_codegen.generate_fmt_c(AST_NODES, AST_FLAGS, fmt_output)

    log(f"  {len(AST_NODES)} node types, {len(AST_ENUMS)} enums")
    log("  Generated: ast_nodes_gen.h (public + internal), ast_builder_gen.h, ast_builder_gen.c, ast_print_gen.c, fmt_gen.c")


def main():
    parser = argparse.ArgumentParser(description="Extract SQLite tokenizer and parser for syntaqlite")
    parser.add_argument("--prefix", default=DEFAULT_PREFIX, help=f"Symbol prefix (default: {DEFAULT_PREFIX})")
    parser.add_argument("--output", type=Path, default=OUTPUT_DIR, help="Output directory")
    parser.add_argument("--lemon-bin", type=Path, help="Path to pre-built lemon binary")
    parser.add_argument("--mkkeywordhash-bin", type=Path, help="Path to pre-built mkkeywordhash binary")
    parser.add_argument("-v", "--verbose", action="store_true", help="Print progress messages")
    args = parser.parse_args()

    global _verbose
    _verbose = args.verbose

    runner = ToolRunner(root_dir=ROOT_DIR, verbose=args.verbose)

    # Inject pre-built tool paths if provided (skips GN/ninja build step).
    # Resolve relative to CWD since GN passes build-dir-relative paths.
    if args.lemon_bin:
        runner._tool_paths["lemon"] = args.lemon_bin.resolve()
    if args.mkkeywordhash_bin:
        runner._tool_paths["mkkeywordhash"] = args.mkkeywordhash_bin.resolve()

    if not runner.sqlite_src.exists():
        print(f"SQLite not found at {runner.sqlite_src}. Run tools/dev/install-build-deps first", file=sys.stderr)
        return 1

    prefix = args.prefix
    output_dir = args.output
    output_dir.mkdir(parents=True, exist_ok=True)

    log(f"Using prefix: {prefix}")
    log(f"Output directory: {output_dir}")

    # Generate parser first to get parse.h for consistent token values
    log("\n=== Generating Parser ===")
    parse_h_content = generate_parser(runner, output_dir, prefix)

    # Generate token definitions and character map
    log("\n=== Generating Tokens and Charmap ===")
    include_dir = output_dir.parent / "include" / "syntaqlite"
    generate_token_defs(runner, include_dir / "sqlite_tokens_gen.h", prefix, parse_h_content)
    copy_global_tables(runner, output_dir / "tokenizer" / "sqlite_charmap_gen.h", prefix)

    # Generate tokenizer
    log("\n=== Generating Tokenizer ===")
    keywordhash_data, keyword_code_func = build_keywordhash(runner, prefix)
    copy_tokenize_c(runner, output_dir / "tokenizer" / "sqlite_tokenize_gen.c", prefix, keywordhash_data, keyword_code_func)

    # Generate AST code
    log("\n=== Generating AST ===")
    generate_ast(output_dir)

    log(f"\nAll files written to {output_dir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
