# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Build amalgamated extension grammar file for SQLite dialects.

This script allows external users to generate a single header file containing
all the data needed to extend syntaqlite with custom keywords and grammar rules.
The generated file can be injected at compile time.

The amalgamated file includes:
1. Token definitions (#define SYNTAQLITE_TOKEN_*)
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

from python.syntaqlite.sqlite_extractor import (
    ToolRunner,
    HeaderGenerator,
    build_synq_grammar,
    split_extension_grammar,
    extract_parser_data,
    format_parser_data_header,
    format_extension_reduce_function,
    create_parser_symbol_rename_pipeline,
    process_keywordhash,
)
from python.syntaqlite.sqlite_extractor.generators import extract_token_defines


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

        # Split extension grammar: directives go first, rules go last
        # This ensures extension tokens get IDs after base tokens while
        # rules can reference base grammar nonterminals
        base_grammar = runner.get_base_grammar()
        ext_grammar = extension_grammar.read_text()
        ext_directives, ext_rules = split_extension_grammar(ext_grammar)

        combined = ext_directives + "\n" + base_grammar
        if ext_rules:
            combined += "\n" + ext_rules

        # Replace TK_ prefix with SYNTAQLITE_TOKEN_ to match our convention
        combined = combined.replace("%token_prefix TK_",
                                    "%token_prefix SYNTAQLITE_TOKEN_")

        (tmpdir / "parse.y").write_text(combined)

        # Run lemon once to get parse.h
        parse_h = runner.run_lemon(tmpdir / "parse.y")
        parse_h_content = parse_h.read_text()

        defines = extract_token_defines(parse_h_content)
        return "\n".join(defines)


def generate_keywordhash_data(runner: ToolRunner, extra_keywords: list[str]) -> str:
    """Generate keyword hash data with extra keywords.

    Returns the data arrays + hash parameter #defines as a string.
    The keywordCode function is generic (parameterized via SYNQ_KW_HASH_*
    defines) and lives in the base tokenizer — extensions only swap data.
    """
    output = runner.run_mkkeywordhash(extra_keywords=extra_keywords)
    result = process_keywordhash(output, "synq")
    return result.data_section


def generate_parser_data(
    runner: ToolRunner,
    extension_grammar: Path,
    prefix: str = "synq",
    verbose: bool = False,
) -> str:
    """Generate parser data from merged extension + SQLite grammar.

    This generates the parser tables and extension reduce function
    that can be used to extend the parser at compile time.

    Args:
        runner: ToolRunner instance.
        extension_grammar: Path to extension grammar file.
        prefix: Symbol prefix.
        verbose: Print progress messages.

    Returns:
        Parser data as a string for inclusion in the amalgamated header.
    """
    with tempfile.TemporaryDirectory() as tmpdir:
        tmpdir_path = Path(tmpdir)

        # Read extension grammar
        ext_grammar_content = extension_grammar.read_text()

        # Build clean grammar with extension merged
        if verbose:
            print("Building merged grammar...")
        grammar_content = build_synq_grammar(
            runner, prefix, extension_grammar=ext_grammar_content
        )
        grammar_path = tmpdir_path / "synq_parse.y"
        grammar_path.write_text(grammar_content)

        # Copy stock lempar.c template
        lempar_path = tmpdir_path / "lempar.c"
        lempar_path.write_text(runner.get_lempar_path().read_text())

        # Run Lemon
        if verbose:
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
        reduce_func = format_extension_reduce_function(parser_data, prefix)

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
        "--prefix", default="synq",
        help="Symbol prefix (default: syntaqlite)"
    )
    parser.add_argument(
        "-v", "--verbose", action="store_true",
        help="Print progress messages"
    )
    args = parser.parse_args()

    verbose = args.verbose
    runner = ToolRunner(root_dir=ROOT_DIR, verbose=verbose)

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

    if verbose:
        print(f"Grammar: {args.grammar}")
        print(f"  Extra keywords: {extra_keywords}")

    # Generate token definitions
    if verbose:
        print("\nGenerating token definitions...")
    token_defines = generate_token_defines(runner, args.grammar)

    # Generate keywordhash data (tables + hash param defines)
    if verbose:
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
        if verbose:
            print("Generating parser data...")
        parser_data = generate_parser_data(runner, args.grammar, args.prefix, verbose=verbose)
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
    guard = f"SYNQ_EXTENSION_{args.output.stem.upper()}_H"
    gen = HeaderGenerator(
        guard=guard,
        description=f"Amalgamated extension grammar for syntaqlite.\n** Generated by: tools/build-extension-grammar --grammar {args.grammar.name}",
        regenerate_cmd="",
        verbose=verbose,
    )
    gen.write(args.output, content)

    if verbose:
        print(f"\nTo use with syntaqlite, compile with:")
        print(f'  -DSYNTAQLITE_EXTENSION_GRAMMAR=\\"{args.output}\\"')
    return 0


if __name__ == "__main__":
    sys.exit(main())
