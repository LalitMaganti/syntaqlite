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
    SymbolRename,
    build_synq_grammar,
    split_extension_grammar,
    extract_parser_data,
    format_parser_data_header,
    format_extension_reduce_function,
    process_keywordhash,
    run_lemon_pipeline,
)
from python.syntaqlite.sqlite_extractor.generators import extract_token_defines
from python.syntaqlite.ast_codegen.codegen import generate_extension_nodes_c
from python.syntaqlite.ast_codegen.fmt_codegen import generate_extension_fmt_c


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
) -> tuple[str, str]:
    """Generate parser data from merged extension + SQLite grammar.

    This generates the parser tables and extension reduce function
    that can be used to extend the parser at compile time.

    Args:
        runner: ToolRunner instance.
        extension_grammar: Path to extension grammar file.
        prefix: Symbol prefix.
        verbose: Print progress messages.

    Returns:
        Tuple of (parser_data, token_defines).
        parser_data: Parser tables + reduce function for the amalgamated header.
        token_defines: Token #define lines from the SAME Lemon run (for
            consistency with the parser tables' numeric token IDs).
    """
    # Read extension grammar
    ext_grammar_content = extension_grammar.read_text()

    # Build clean grammar with extension merged
    if verbose:
        print("Building merged grammar...")
    grammar_content = build_synq_grammar(
        runner, prefix, extension_grammar=ext_grammar_content
    )

    # Run Lemon pipeline (shared: tmpdir, template, lemon, rename)
    if verbose:
        print("Running Lemon on merged grammar...")
    parse_c_content, parse_h_content = run_lemon_pipeline(
        runner, grammar_content, prefix
    )

    # Extract parser data
    parser_data = extract_parser_data(parse_c_content)

    # Format as header content
    data_header = format_parser_data_header(parser_data)
    reduce_func = format_extension_reduce_function(parser_data, prefix)

    # Derive token defines from this same Lemon run's parse.h.
    # Lemon uses TK_ prefix; rename to SYNTAQLITE_TOKEN_ to match.
    parse_h_content = SymbolRename("TK_", "SYNTAQLITE_TOKEN_").apply(parse_h_content)
    defines = extract_token_defines(parse_h_content)
    token_defines = "\n".join(defines)

    return data_header + "\n" + reduce_func, token_defines


def generate_extension_nodes(nodes_path: Path) -> str:
    """Generate C code for extension AST nodes from a Python definition file.

    Dynamically imports the given module and calls generate_extension_nodes_c()
    with the NODES (and optional ENUMS, FLAGS) lists defined there.

    Args:
        nodes_path: Path to a Python file defining NODES, and optionally ENUMS/FLAGS.

    Returns:
        C code string with struct typedefs, tag defines, and static inline builders.
    """
    import importlib.util

    spec = importlib.util.spec_from_file_location("ext_nodes", nodes_path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)

    node_defs = getattr(mod, "NODES", [])
    enum_defs = getattr(mod, "ENUMS", [])
    flags_defs = getattr(mod, "FLAGS", [])

    code = generate_extension_nodes_c(node_defs, enum_defs, flags_defs)

    # Generate format recipes if any nodes have fmt annotations.
    has_fmt = any(getattr(n, 'fmt', None) is not None for n in node_defs)
    if has_fmt:
        fmt_code = generate_extension_fmt_c(node_defs, flags_defs)
        if fmt_code:
            code += "\n" + fmt_code

    return code


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
        "--nodes", type=Path, default=None,
        help="Python file with extension AST node definitions (NODES list)"
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

    # When generating parser data, derive token defines from the same Lemon
    # run to ensure consistent token IDs between tokenizer and parser.
    if not args.tokenizer_only:
        if verbose:
            print("\nGenerating parser data...")
        parser_data, token_defines = generate_parser_data(
            runner, args.grammar, args.prefix, verbose=verbose
        )
    else:
        # Tokenizer-only: generate token defines from a standalone Lemon run
        if verbose:
            print("\nGenerating token definitions...")
        token_defines = generate_token_defines(runner, args.grammar)
        parser_data = None

    # Generate keywordhash data (tables + hash param defines)
    if verbose:
        print("Generating keyword hash data...")
    keywordhash_data = generate_keywordhash_data(runner, extra_keywords)

    # Generate extension node code if node definitions provided
    extension_nodes_code = None
    if args.nodes:
        if not args.nodes.exists():
            print(f"Node definitions not found: {args.nodes}", file=sys.stderr)
            return 1
        if verbose:
            print("Generating extension node definitions...")
        extension_nodes_code = generate_extension_nodes(args.nodes)

    # Combine into single amalgamated file
    content_parts = [
        "/*",
        " * Token definitions",
        " */",
        "",
        token_defines,
        "",
    ]

    # Extension nodes go after token defines but before parser data,
    # so builder functions are available in reduce actions.
    if extension_nodes_code is not None:
        content_parts.extend([
            "/*",
            " * Extension node definitions",
            " */",
            "",
            extension_nodes_code,
            "",
        ])

    content_parts.extend([
        "/*",
        " * Keyword hash data",
        " */",
        "",
        keywordhash_data,
    ])

    if parser_data is not None:
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
