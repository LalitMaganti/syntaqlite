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
2. Transform lempar.c to add injection points
3. Generate parser with dialect extensibility support

Usage:
    python3 python/tools/extract_sqlite.py

For external users who want custom keywords for SQLite dialects,
use tools/build-extension-grammar instead.
"""
import argparse
import re
import sys
import tempfile
from pathlib import Path

# Add project root to path for imports
ROOT_DIR = Path(__file__).parent.parent.parent
if str(ROOT_DIR) not in sys.path:
    sys.path.insert(0, str(ROOT_DIR))

from python.sqlite_extractor import (
    SourceFile,
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
    SQLITE_BLESSING,
    create_parser_symbol_rename_pipeline,
    create_keywordhash_rename_pipeline,
)

# Keywordhash processing markers
KEYWORDHASH_SCORE_MARKER = "/* Hash score:"
KEYWORD_CODE_FUNC_MARKER = "static int keywordCode("
from python.sqlite_extractor.generators import extract_tk_defines
from python.sqlite_extractor.grammar_build import (
    build_syntaqlite_grammar,
)
from python.sqlite_extractor.lempar_transform import (
    transform_to_base_template,
)

ROOT_DIR = Path(__file__).parent.parent.parent
SQLITE_SRC = ROOT_DIR / "third_party" / "src" / "sqlite" / "src"
OUTPUT_DIR = ROOT_DIR / "src"

# Default prefix for renamed sqlite3 symbols to avoid clashes
DEFAULT_PREFIX = "syntaqlite"

REGENERATE_CMD = "python3 python/tools/extract_sqlite.py"


def create_symbol_rename_pipeline(prefix: str) -> Pipeline:
    """Create a pipeline for renaming sqlite3 symbols.

    This uses the C tokenizer for safe renaming that preserves comments/strings.
    """
    sqlite3_prefix = f"{prefix}_sqlite3"

    return Pipeline([
        # Rename sqlite3XxxYyy -> {prefix}_sqlite3XxxYyy
        SymbolRename("sqlite3", sqlite3_prefix),
        # Rename testcase() which doesn't have sqlite3 prefix
        SymbolRenameExact("testcase", f"{sqlite3_prefix}Testcase"),
        # Remove the charMap macro - we'll use Tolower from sqlite_tables.h
        RemoveRegex(r'#\s*define\s+charMap\(X\)[^\n]*\n'),
        # Rename charMap -> Tolower
        SymbolRenameExact("charMap", f"{sqlite3_prefix}Tolower"),
    ])


def process_keywordhash_output(output: str, prefix: str) -> tuple[str, str]:
    """Process mkkeywordhash output and split into data and function sections.

    Args:
        output: Raw output from mkkeywordhash.
        prefix: Symbol prefix.

    Returns:
        Tuple of (data_section, keyword_code_function).
    """
    sqlite3_prefix = f"{prefix}_sqlite3"

    # Rename symbols in the output using safe C tokenizer
    pipeline = create_symbol_rename_pipeline(prefix)
    output = pipeline.apply(output)

    # Rename keywordhash arrays to have our prefix
    output = create_keywordhash_rename_pipeline(prefix).apply(output)

    # Remove renamed Testcase calls (entire lines)
    output = RemoveFunctionCalls(f"{sqlite3_prefix}Testcase").apply(output)

    # Find start of actual generated content (skip the SQLite header comment)
    start = output.find(KEYWORDHASH_SCORE_MARKER)
    if start == -1:
        start = 0

    generated = output[start:]

    # Find keywordCode function - it starts with "static int keywordCode("
    keyword_code_start = generated.find(KEYWORD_CODE_FUNC_MARKER)
    if keyword_code_start == -1:
        raise ValueError("Could not find keywordCode function in mkkeywordhash output")

    # Get data section (everything before keywordCode)
    data_section = generated[:keyword_code_start].strip()

    # Get logic section (keywordCode function only)
    logic_section = generated[keyword_code_start:]
    keyword_code_end = logic_section.find('\n}\n')
    if keyword_code_end != -1:
        logic_section = logic_section[:keyword_code_end + 3]

    # Rename keywordCode to have our prefix and remove static
    logic_section = logic_section.replace("static int keywordCode(", f"int {prefix}_keywordCode(")

    return data_section, logic_section.strip()


def build_keywordhash(
    runner: ToolRunner,
    prefix: str,
) -> tuple[str, str]:
    """Build keyword hash data.

    Returns:
        Tuple of (data_section, keyword_code_func) to be inlined into sqlite_tokenize.c.
    """
    # Run mkkeywordhash
    output = runner.run_mkkeywordhash()

    # Process output
    return process_keywordhash_output(output, prefix)


def copy_tokenize_c(
    runner: ToolRunner,
    output_path: Path,
    prefix: str,
    keywordhash_data: str,
    keyword_code_func: str,
) -> None:
    """Copy and transform tokenize.c."""
    sqlite3_prefix = f"{prefix}_sqlite3"
    source = SourceFile.from_path(runner.sqlite_src / "tokenize.c")

    # Build the inlined header that replaces sqliteInt.h
    inlined_header = f'''#include "src/syntaqlite_sqlite_defs.h"
#include "src/sqlite_charmap.h"

/*
** Syntaqlite tokenizer injection support.
** When SYNTAQLITE_KEYWORDHASH_DATA_FILE is defined, external keyword data is used.
*/
#ifdef SYNTAQLITE_KEYWORDHASH_DATA_FILE
#include SYNTAQLITE_KEYWORDHASH_DATA_FILE
#define _SYNTAQLITE_EXTERNAL_KEYWORDHASH 1
#endif

#ifndef _SYNTAQLITE_EXTERNAL_KEYWORDHASH
#include "src/sqlite_tokens.h"
{keywordhash_data}
#endif /* _SYNTAQLITE_EXTERNAL_KEYWORDHASH */

/* Stub for parser fallback - not needed for pure tokenization */
static inline int {sqlite3_prefix}ParserFallback(int token) {{
  (void)token;
  return 0;
}}

/* Forward declaration */
i64 {sqlite3_prefix}GetToken(const unsigned char *z, int *tokenType);

/* Keyword lookup function */
{keyword_code_func}
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

    content = pipeline.apply(source.content)

    # Rename sqlite3* symbols (using safe C tokenizer)
    content = create_symbol_rename_pipeline(prefix).apply(content)

    # Rename keywordCode to have our prefix
    content = SymbolRenameExact("keywordCode", f"{prefix}_keywordCode").apply(content)

    # Remove Testcase calls
    content = RemoveFunctionCalls(f"{sqlite3_prefix}Testcase").apply(content)

    # Generate the final file
    gen = SourceFileGenerator(
        description=f"SQLite tokenizer for {prefix}.\n** Extracted from SQLite's tokenize.c with minimal modifications.",
        regenerate_cmd=REGENERATE_CMD,
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
    guard = f"{prefix.upper()}_SRC_SQLITE_TOKENS_H"

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

    # Extract TK_* defines
    defines = extract_tk_defines(parse_h_content)

    # Generate header
    gen = HeaderGenerator(
        guard=guard,
        description="Token definitions from SQLite's parse.y via Lemon.",
        regenerate_cmd=REGENERATE_CMD,
    )
    gen.write(output_path, "\n".join(defines))


def copy_global_tables(runner: ToolRunner, output_path: Path, prefix: str) -> None:
    """Copy character tables from SQLite's global.c."""
    sqlite3_prefix = f"{prefix}_sqlite3"
    guard = f"{prefix.upper()}_SRC_SQLITE_CHARMAP_H"

    source = SourceFile.from_path(runner.sqlite_src / "global.c")
    content = source.content

    # Find sqlite3UpperToLower array - extract only ASCII version
    upper_start = content.find("const unsigned char sqlite3UpperToLower[]")
    upper_ascii_start = content.find("#ifdef SQLITE_ASCII", upper_start)
    upper_ascii_end = content.find("#endif", upper_ascii_start)
    # Get the data between #ifdef SQLITE_ASCII and #endif
    upper_data = content[upper_ascii_start:upper_ascii_end]
    # Extract just the numeric values
    upper_values = re.search(r'#ifdef SQLITE_ASCII\s*([\d\s,]+)', upper_data, re.DOTALL)

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
    if upper_values:
        tables += upper_values.group(1).strip()

    tables += "\n};\n\n"

    # Add the ctype comment and map, renaming sqlite3CtypeMap
    tables += ctype_comment + "\n"
    ctype_renamed = ctype_block.replace("sqlite3CtypeMap", f"{sqlite3_prefix}CtypeMap")
    tables += "static " + ctype_renamed + "\n"

    # Extract character classification macros from sqliteInt.h
    sqliteint = SourceFile.from_path(runner.sqlite_src / "sqliteInt.h")

    # Find the ASCII block of macros
    ascii_start = sqliteint.content.find("/*\n** The following macros mimic the standard library functions")
    if ascii_start == -1:
        print("Warning: Could not find character classification macros in sqliteInt.h", file=sys.stderr)
    else:
        # Find the #ifdef SQLITE_ASCII block
        ifdef_start = sqliteint.content.find("#ifdef SQLITE_ASCII", ascii_start)
        else_start = sqliteint.content.find("#else", ifdef_start)

        # Extract the comment and the ASCII macros
        macro_comment = sqliteint.content[ascii_start:sqliteint.content.find("*/", ascii_start) + 2]
        ascii_macros = sqliteint.content[ifdef_start + len("#ifdef SQLITE_ASCII"):else_start].strip()

        tables += "\n" + macro_comment + "\n"
        # Rename sqlite3 -> {prefix}_sqlite3 in the macros (safe renaming)
        ascii_macros_renamed = create_symbol_rename_pipeline(prefix).apply(ascii_macros)
        tables += ascii_macros_renamed + "\n"

    # Generate header
    gen = HeaderGenerator(
        guard=guard,
        description="Character tables extracted from SQLite's global.c",
        regenerate_cmd=REGENERATE_CMD,
    )
    gen.write(output_path, tables)


def generate_parser(
    runner: ToolRunner,
    output_dir: Path,
    prefix: str,
) -> str:
    """Generate the SQLite parser with injection points.

    This function:
    1. Builds a clean grammar from SQLite's parse.y
    2. Generates a modified lempar.c template with injection points
    3. Runs Lemon with the custom template
    4. Applies symbol renaming

    Args:
        runner: ToolRunner instance for running tools.
        output_dir: Directory to write output files.
        prefix: Symbol prefix.

    Returns:
        The parse.h content from Lemon (for token generation).
    """
    sqlite3_prefix = f"{prefix}_sqlite3"

    print("\n=== Extracting Parser ===")

    with tempfile.TemporaryDirectory() as tmpdir:
        tmpdir_path = Path(tmpdir)

        # Step 1: Build the clean grammar
        print("Building clean grammar from SQLite's parse.y...")
        grammar_content = build_syntaqlite_grammar(runner, prefix)
        grammar_path = tmpdir_path / "syntaqlite_parse.y"
        grammar_path.write_text(grammar_content)

        # Step 2: Generate the modified lempar.c template
        print("Generating syntaqlite_lempar.c template...")
        lempar_content = runner.get_lempar_template().decode("utf-8")
        modified_lempar = transform_to_base_template(lempar_content)
        lempar_path = tmpdir_path / "lempar.c"
        lempar_path.write_text(modified_lempar)

        # Step 3: Run Lemon with the custom template
        print("Running Lemon to generate parser...")
        parse_c_path, parse_h_path = runner.run_lemon_with_template(
            grammar_path, lempar_path, tmpdir_path
        )
        parse_c_content = parse_c_path.read_text()
        parse_h_content = parse_h_path.read_text()

        # Step 4: Apply symbol renaming to the generated parser
        print("Applying symbol renaming...")
        rename_pipeline = create_parser_symbol_rename_pipeline(prefix)
        parse_c_content = rename_pipeline.apply(parse_c_content)

        # Step 5: Remove Lemon's source file comment (contains temp path)
        parse_c_content = re.sub(
            r'/\* This file is automatically generated by Lemon from input grammar\n'
            r'\*\* source file "[^"]+"\.\n'
            r'\*/\n',
            '',
            parse_c_content
        )

        # Step 6: Write output files
        output_dir.mkdir(parents=True, exist_ok=True)

        # Write the parser source
        gen = SourceFileGenerator(
            description=f"SQLite parser for {prefix}.\n** Generated from SQLite's parse.y via Lemon with injection points.",
            regenerate_cmd=REGENERATE_CMD,
        )
        gen.write(output_dir / "sqlite_parse.c", parse_c_content)

        return parse_h_content


def main():
    parser = argparse.ArgumentParser(description="Extract SQLite tokenizer and parser for syntaqlite")
    parser.add_argument("--prefix", default=DEFAULT_PREFIX, help=f"Symbol prefix (default: {DEFAULT_PREFIX})")
    parser.add_argument("--output", type=Path, default=OUTPUT_DIR, help="Output directory")
    args = parser.parse_args()

    runner = ToolRunner(root_dir=ROOT_DIR)

    if not runner.sqlite_src.exists():
        print(f"SQLite not found at {runner.sqlite_src}. Run tools/dev/install-build-deps first", file=sys.stderr)
        return 1

    prefix = args.prefix
    output_dir = args.output
    output_dir.mkdir(parents=True, exist_ok=True)

    print(f"Using prefix: {prefix}")
    print(f"Output directory: {output_dir}")

    # Generate parser first to get parse.h for consistent token values
    print("\n=== Generating Parser ===")
    parse_h_content = generate_parser(runner, output_dir, prefix)

    # Generate token definitions and character map
    print("\n=== Generating Tokens and Charmap ===")
    generate_token_defs(runner, output_dir / "sqlite_tokens.h", prefix, parse_h_content)
    copy_global_tables(runner, output_dir / "sqlite_charmap.h", prefix)

    # Generate tokenizer
    print("\n=== Generating Tokenizer ===")
    keywordhash_data, keyword_code_func = build_keywordhash(runner, prefix)
    copy_tokenize_c(runner, output_dir / "sqlite_tokenize.c", prefix, keywordhash_data, keyword_code_func)

    print(f"\nAll files written to {output_dir}")

    print("\nDone!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
