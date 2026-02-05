# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Build amalgamated extension grammar file for SQLite dialects.

This script allows external users to generate a single header file containing
all the data needed to extend syntaqlite with custom keywords and grammar rules.
The generated file can be injected at compile time.

The amalgamated file includes:
1. Token definitions (#define TK_*)
2. Keyword hash data (for tokenizer)
3. Parser tables and data (for parser)
4. Extension reduce function (for parser)

Usage:
    # First, set up the build environment:
    tools/dev/install-build-deps

    # Then build extension grammar from your .y file:
    tools/build-extension-grammar --grammar my_dialect.y --output my_dialect.h

    # To use the generated file, compile with:
    clang -c sqlite_tokenize.c sqlite_parse.c \
        -DSYNTAQLITE_EXTENSION_GRAMMAR=\"my_dialect.h\" \
        -I/path/to/syntaqlite
"""
import argparse
import re
import sys
import tempfile
from pathlib import Path

ROOT_DIR = Path(__file__).parent.parent.parent
if str(ROOT_DIR) not in sys.path:
    sys.path.insert(0, str(ROOT_DIR))

from python.sqlite_extractor import (
    ToolRunner,
    HeaderGenerator,
    SQLITE_BLESSING,
    build_syntaqlite_grammar,
    transform_to_base_template,
    extract_parser_data,
    format_parser_data_header,
    format_extension_reduce_function,
    KEYWORDHASH_SCORE_MARKER,
    KEYWORD_CODE_FUNC_MARKER,
    create_parser_symbol_rename_pipeline,
    create_keywordhash_rename_pipeline,
)
from python.sqlite_extractor.generators import extract_tk_defines


def parse_extension_keywords(grammar_path: Path) -> list[str]:
    """Parse %token declarations from an extension grammar file."""
    content = grammar_path.read_text()
    keywords = []
    for match in re.finditer(r'%token\s+([^%{]+?)(?=\n(?:%|\s*$)|$)', content, re.DOTALL):
        keywords.extend(re.findall(r'\b([A-Z][A-Z0-9_]*)\b', match.group(1)))
    # Dedupe preserving order
    return list(dict.fromkeys(keywords))


def generate_token_defines(runner: ToolRunner, extension_grammar: Path) -> str:
    """Generate token definitions by merging extension grammar with SQLite's.

    Returns the token #define lines as a string.
    """
    with tempfile.TemporaryDirectory() as tmpdir:
        tmpdir = Path(tmpdir)

        # Concatenate: extension grammar + base grammar
        # Extension goes first because SQLite requires SPACE/COMMENT/ILLEGAL
        # to be the last tokens (they appear at the end of parse.y)
        base_grammar = runner.get_base_grammar()
        ext_grammar = extension_grammar.read_text()
        combined = ext_grammar + "\n" + base_grammar

        (tmpdir / "parse.y").write_text(combined)

        # Run lemon
        parse_h = runner.run_lemon(tmpdir / "parse.y")
        parse_h_content = parse_h.read_text()

        # Extract TK_* defines
        defines = extract_tk_defines(parse_h_content)
        return "\n".join(defines)


def generate_keywordhash_data(runner: ToolRunner, extra_keywords: list[str]) -> str:
    """Generate keyword hash data with extra keywords.

    Returns the data arrays as a string.
    """
    # Run mkkeywordhash with extra keywords
    output = runner.run_mkkeywordhash(extra_keywords=extra_keywords)

    # Find start of actual generated content (skip the SQLite header comment)
    start = output.find(KEYWORDHASH_SCORE_MARKER)
    if start == -1:
        start = 0

    generated = output[start:]

    # Find end of data section (before keywordCode function)
    keyword_code_start = generated.find(KEYWORD_CODE_FUNC_MARKER)
    if keyword_code_start == -1:
        print("Error: Could not find keywordCode function", file=sys.stderr)
        sys.exit(1)

    # Find the "Hash table decoded" comment that ends the data section
    hash_table_decoded = generated.rfind("/* Hash table decoded:", 0, keyword_code_start)
    if hash_table_decoded != -1:
        comment_end = generated.find("*/", hash_table_decoded) + 2
        data_section = generated[:comment_end]
    else:
        # Fallback: find SQLITE_N_KEYWORD define
        n_keyword = generated.find("#define SQLITE_N_KEYWORD")
        if n_keyword != -1:
            line_end = generated.find("\n", n_keyword) + 1
            data_section = generated[:line_end]
        else:
            data_section = generated[:keyword_code_start]

    # Rename keywordhash arrays to have syntaqlite_ prefix
    data_section = create_keywordhash_rename_pipeline("syntaqlite").apply(data_section)

    return data_section.strip()


def generate_parser_data(
    runner: ToolRunner,
    extension_grammar: Path,
    prefix: str = "syntaqlite",
) -> str:
    """Generate parser data from merged extension + SQLite grammar.

    This generates the parser tables and extension reduce function
    that can be used to extend the parser at compile time.

    Args:
        runner: ToolRunner instance.
        extension_grammar: Path to extension grammar file.
        prefix: Symbol prefix.

    Returns:
        Parser data as a string for inclusion in the amalgamated header.
    """
    with tempfile.TemporaryDirectory() as tmpdir:
        tmpdir_path = Path(tmpdir)

        # Read extension grammar
        ext_grammar_content = extension_grammar.read_text()

        # Build clean grammar with extension merged
        print("Building merged grammar...")
        grammar_content = build_syntaqlite_grammar(
            runner, prefix, extension_grammar=ext_grammar_content
        )
        grammar_path = tmpdir_path / "syntaqlite_parse.y"
        grammar_path.write_text(grammar_content)

        # Generate modified lempar.c template
        lempar_content = runner.get_lempar_template().decode("utf-8")
        modified_lempar = transform_to_base_template(lempar_content)
        lempar_path = tmpdir_path / "lempar.c"
        lempar_path.write_text(modified_lempar)

        # Run Lemon
        print("Running Lemon on merged grammar...")
        parse_c_path, _ = runner.run_lemon_with_template(
            grammar_path, lempar_path, tmpdir_path
        )
        parse_c_content = parse_c_path.read_text()

        # Apply symbol renaming
        rename_pipeline = create_parser_symbol_rename_pipeline(prefix)
        parse_c_content = rename_pipeline.apply(parse_c_content)

        # Extract parser data
        parser_data = extract_parser_data(parse_c_content)

        # Format as header content
        data_header = format_parser_data_header(parser_data)
        reduce_func = format_extension_reduce_function(parser_data)

        return data_header + "\n" + reduce_func


def main():
    parser = argparse.ArgumentParser(
        description="Build amalgamated extension grammar file for SQLite dialects",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Example:
    tools/build-extension-grammar --grammar my_dialect.y --output my_dialect.h

Then compile with:
    clang -c sqlite_tokenize.c sqlite_parse.c \\
        -DSYNTAQLITE_EXTENSION_GRAMMAR=\\"my_dialect.h\\" \\
        -I/path/to/syntaqlite
"""
    )
    parser.add_argument(
        "--grammar", type=Path, required=True,
        help="Grammar file (.y) with %%token declarations for extra keywords"
    )
    parser.add_argument(
        "--output", "-o", type=Path, required=True,
        help="Output file path for amalgamated header"
    )
    parser.add_argument(
        "--tokenizer-only", action="store_true",
        help="Only generate tokenizer data (skip parser)"
    )
    parser.add_argument(
        "--prefix", default="syntaqlite",
        help="Symbol prefix (default: syntaqlite)"
    )
    args = parser.parse_args()

    runner = ToolRunner(root_dir=ROOT_DIR)

    if not runner.sqlite_src.exists():
        print(f"SQLite not found at {runner.sqlite_src}.", file=sys.stderr)
        print("Run tools/dev/install-build-deps first.", file=sys.stderr)
        return 1

    if not args.grammar.exists():
        print(f"Grammar file not found: {args.grammar}", file=sys.stderr)
        return 1

    # Parse %token declarations from extension grammar
    extra_keywords = parse_extension_keywords(args.grammar)

    if not extra_keywords:
        print(f"Warning: No %token declarations found in {args.grammar}", file=sys.stderr)
        print("Expected format: %token KEYWORD1 KEYWORD2.", file=sys.stderr)
        return 1

    print(f"Grammar: {args.grammar}")
    print(f"  Extra keywords: {extra_keywords}")

    # Generate token definitions
    print("\nGenerating token definitions...")
    token_defines = generate_token_defines(runner, args.grammar)

    # Generate keywordhash data
    print("Generating keyword hash data...")
    keywordhash_data = generate_keywordhash_data(runner, extra_keywords)

    # Combine into single amalgamated file
    content_parts = [
        "/*",
        " * Token definitions",
        " */",
        "",
        token_defines,
        "",
        "/*",
        " * Keyword hash data",
        " */",
        "",
        keywordhash_data,
    ]

    # Optionally generate parser data
    if not args.tokenizer_only:
        print("Generating parser data...")
        parser_data = generate_parser_data(runner, args.grammar, args.prefix)
        content_parts.extend([
            "",
            "/*",
            " * Parser data",
            " */",
            "",
            parser_data,
        ])

    content = "\n".join(content_parts)

    # Write amalgamated file
    guard = f"SYNTAQLITE_EXTENSION_{args.output.stem.upper()}_H"
    gen = HeaderGenerator(
        guard=guard,
        description=f"Amalgamated extension grammar for syntaqlite.\n** Generated by: tools/build-extension-grammar --grammar {args.grammar.name}",
        regenerate_cmd="",
    )
    gen.write(args.output, content)

    print(f"\nTo use with syntaqlite, compile with:")
    print(f'  -DSYNTAQLITE_EXTENSION_GRAMMAR=\\"{args.output}\\"')
    return 0


if __name__ == "__main__":
    sys.exit(main())
