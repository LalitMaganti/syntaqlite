# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Extract SQLite tokenization code.

Following Perfetto's approach:
1. Build mkkeywordhash using GN/ninja
2. Extract keywords from extension grammar file (if provided)
3. Run mkkeywordhash to generate keywordhash.h
4. Copy tokenize.c with minimal changes
5. Generate helper header files

Usage:
    python3 python/tools/extract_tokenizer.py
    python3 python/tools/extract_tokenizer.py --extend-grammar extensions.y
    python3 python/tools/extract_tokenizer.py --prefix mydialect --output out/mydialect
"""
import argparse
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT_DIR = Path(__file__).parent.parent.parent
SQLITE_SRC = ROOT_DIR / "third_party" / "src" / "sqlite" / "src"
SQLITE_TOOL = ROOT_DIR / "third_party" / "src" / "sqlite" / "tool"
OUTPUT_DIR = ROOT_DIR / "src" / "tokenizer" / "sqlite"

# Dedicated build directory for this script
TOOLS_BUILD_DIR = ROOT_DIR / "out" / ".extract_tokenizer"

# Default prefix for renamed sqlite3 symbols to avoid clashes
DEFAULT_PREFIX = "syntaqlite"


def ensure_build_dir() -> None:
    """Ensure the build directory is configured."""
    gn = ROOT_DIR / "tools" / "dev" / "gn"
    build_ninja = TOOLS_BUILD_DIR / "build.ninja"

    if not build_ninja.exists():
        print(f"Configuring build in {TOOLS_BUILD_DIR}...")
        TOOLS_BUILD_DIR.mkdir(parents=True, exist_ok=True)
        subprocess.run(
            [str(gn), "gen", str(TOOLS_BUILD_DIR), "--args=is_debug=false"],
            capture_output=True,
            text=True
        )
        if not build_ninja.exists():
            print("Failed to configure build", file=sys.stderr)
            sys.exit(1)


def build_tool(name: str) -> Path:
    """Build a tool and return its path."""
    ensure_build_dir()

    ninja = ROOT_DIR / "tools" / "dev" / "ninja"
    tool_path = TOOLS_BUILD_DIR / name
    if os.name == "nt":
        tool_path = TOOLS_BUILD_DIR / f"{name}.exe"

    print(f"Building {name}...")
    result = subprocess.run(
        [str(ninja), "-C", str(TOOLS_BUILD_DIR), name],
        capture_output=True,
        text=True
    )
    if result.returncode != 0:
        print(f"Build failed:\n{result.stderr}", file=sys.stderr)
        sys.exit(1)

    return tool_path


def find_blessing_end(content: str) -> int:
    """Find the end of SQLite's blessing comment, return index after '*/'."""
    marker = content.find("May you share freely")
    if marker == -1:
        return 0
    end = content.find("*/", marker)
    return end + 2 if end != -1 else 0


def rename_sqlite3_symbols(content: str, prefix: str) -> str:
    """Rename sqlite3* symbols to {prefix}_sqlite3* to avoid clashes."""
    sqlite3_prefix = f"{prefix}_sqlite3"

    # Rename sqlite3XxxYyy -> {prefix}_sqlite3XxxYyy
    content = re.sub(r'\bsqlite3([A-Z_][A-Za-z0-9_]*)', f'{sqlite3_prefix}\\1', content)

    # Rename testcase() which doesn't have sqlite3 prefix
    content = content.replace('testcase(', f'{sqlite3_prefix}Testcase(')

    # Remove the charMap macro (we provide our own via {prefix}_sqlite3CharMap)
    content = re.sub(r'#\s*define\s+charMap\(X\)[^\n]*\n', '', content)
    content = content.replace('charMap(', f'{sqlite3_prefix}CharMap(')

    return content


def parse_extension_grammar(grammar_path: Path) -> list[str]:
    """Parse a .y grammar file and extract %token declarations as keywords.

    Returns a list of keyword names extracted from %token lines.
    """
    content = grammar_path.read_text()
    keywords = []

    # Match %token TOKEN1 TOKEN2 ... (may span multiple lines with backslash)
    # Also handles %token TOKEN1. format
    for match in re.finditer(r'%token\s+([^%{]+?)(?=\n(?:%|\s*$)|$)', content, re.DOTALL):
        token_str = match.group(1)
        # Extract individual token names (uppercase identifiers)
        tokens = re.findall(r'\b([A-Z][A-Z0-9_]*)\b', token_str)
        keywords.extend(tokens)

    # Remove duplicates while preserving order
    seen = set()
    unique = []
    for kw in keywords:
        if kw not in seen:
            seen.add(kw)
            unique.append(kw)

    return unique


# The last entry in SQLite's keyword table - we append after this
KEYWORD_TABLE_END = '''  { "WITHOUT",          "TK_WITHOUT",      ALWAYS,           1      },
};'''


def generate_keyword_table_end(extra_keywords: list[str]) -> str:
    """Generate replacement for end of keyword table with extra keywords."""
    lines = ['  { "WITHOUT",          "TK_WITHOUT",      ALWAYS,           1      },']
    for kw in extra_keywords:
        # Pad to align with SQLite's formatting
        name_pad = 18 - len(kw) - 2  # Account for quotes
        token_pad = 15 - len(f"TK_{kw}") - 2
        lines.append(f'  {{ "{kw}",{" " * name_pad}"TK_{kw}",{" " * token_pad}ALWAYS,           1      }},')
    lines.append('};')
    return '\n'.join(lines)


def build_keywordhash(extra_keywords: list[str], output_path: Path, prefix: str, include_dir: str) -> None:
    """Build mkkeywordhash and generate keywordhash.h."""
    sqlite3_prefix = f"{prefix}_sqlite3"
    guard = f"{prefix.upper()}_SRC_TOKENIZER_SQLITE_KEYWORDHASH_H"

    # Build using ninja
    exe = build_tool("mkkeywordhash")

    with tempfile.TemporaryDirectory() as tmpdir:
        tmpdir = Path(tmpdir)

        # If extra keywords, we need to modify mkkeywordhash.c and rebuild
        if extra_keywords:
            src = SQLITE_TOOL / "mkkeywordhash.c"
            content = src.read_text()
            replacement = generate_keyword_table_end(extra_keywords)
            content = content.replace(KEYWORD_TABLE_END, replacement)

            # Write modified source and compile separately
            tmp_src = tmpdir / "mkkeywordhash.c"
            tmp_src.write_text(content)
            tmp_exe = tmpdir / "mkkeywordhash"

            cc = os.environ.get("CC", "clang")
            print(f"Compiling mkkeywordhash with extra keywords...")
            result = subprocess.run(
                [cc, "-o", str(tmp_exe), str(tmp_src)],
                capture_output=True, text=True
            )
            if result.returncode != 0:
                print(f"Compile failed: {result.stderr}", file=sys.stderr)
                sys.exit(1)
            exe = tmp_exe

        # Run and capture output
        print(f"Running mkkeywordhash...")
        result = subprocess.run([str(exe)], capture_output=True, text=True)
        if result.returncode != 0:
            print(f"Run failed: {result.stderr}", file=sys.stderr)
            sys.exit(1)

        # Process output
        output = result.stdout

        # Add header and write
        header = f"""/*
** Keyword hash extracted from SQLite's mkkeywordhash.c
** DO NOT EDIT - regenerate with: python3 python/tools/extract_tokenizer.py
*/
#ifndef {guard}
#define {guard}

#include "{include_dir}/{prefix}_defs.h"
#include "{include_dir}/sqlite_tables.h"
#include "{include_dir}/sqlite_tokens.h"

/*
** Character mapping macro for keyword hash lookup.
** Maps characters to lowercase using the extracted table.
*/
#define {sqlite3_prefix}CharMap(X) \\
    {sqlite3_prefix}UpperToLower[(unsigned char)(X)]

"""
        footer = f"\n#endif /* {guard} */\n"

        # Find start of actual generated content (skip the SQLite header comment)
        start_marker = "/* Hash score:"
        start = output.find(start_marker)
        if start == -1:
            start = 0

        # TODO(lalitm): remove the functions starting at syntaqlite_sqlite3KeywordCode
        # and afterwards.

        # Rename sqlite3* symbols to avoid clashes
        generated = rename_sqlite3_symbols(output[start:], prefix)

        output_path.write_text(header + generated + footer)
        print(f"  Written: {output_path}")


def copy_tokenize_c(output_path: Path, prefix: str, include_dir: str) -> None:
    """Copy tokenize.c with minimal modifications.

    Following Perfetto's approach: keep everything BEFORE the comment
    "/*\\n** Run the parser on the given SQL string." which is where
    the parser-related code begins.
    """
    sqlite3_prefix = f"{prefix}_sqlite3"
    src = SQLITE_SRC / "tokenize.c"
    content = src.read_text()

    # Truncate at parser code - we only want the tokenization functions
    parser_comment = "/*\n** Run the parser on the given SQL string."
    parser_start = content.find(parser_comment)
    if parser_start != -1:
        content = content[:parser_start]
    else:
        print("Warning: Could not find parser comment, keeping full file", file=sys.stderr)

    # Replace the SQLite internal header with our helper
    content = content.replace(
        '#include "sqliteInt.h"',
        f'#include "{include_dir}/{prefix}_tokenize_helper.h"\n#include "{include_dir}/sqlite_keywordhash.h"'
    )

    # Remove keywordhash.h include - it's included by the helper
    content = content.replace('#include "keywordhash.h"\n', '')

    # Remove #include <stdlib.h> - not needed for tokenization
    content = content.replace('#include <stdlib.h>\n', '')

    # Rename static analyze functions to be non-static with our prefix
    # (forward declarations are in the helper)
    content = content.replace(
        'static int analyzeWindowKeyword',
        f'int {sqlite3_prefix}AnalyzeWindowKeyword'
    )
    content = content.replace(
        'static int analyzeOverKeyword',
        f'int {sqlite3_prefix}AnalyzeOverKeyword'
    )
    content = content.replace(
        'static int analyzeFilterKeyword',
        f'int {sqlite3_prefix}AnalyzeFilterKeyword'
    )
    # Also update the call sites
    content = content.replace('analyzeWindowKeyword(', f'{sqlite3_prefix}AnalyzeWindowKeyword(')
    content = content.replace('analyzeOverKeyword(', f'{sqlite3_prefix}AnalyzeOverKeyword(')
    content = content.replace('analyzeFilterKeyword(', f'{sqlite3_prefix}AnalyzeFilterKeyword(')

    # Add header comment
    header = f"""/*
** SQLite tokenizer for {prefix}.
** Extracted from SQLite's tokenize.c with minimal modifications.
** DO NOT EDIT - regenerate with: python3 python/tools/extract_tokenizer.py
*/

"""

    # Find the SQLite blessing comment and replace it
    blessing_end = find_blessing_end(content)
    if blessing_end > 0:
        content = header + content[blessing_end:].lstrip()
    else:
        content = header + content

    # Rename sqlite3* symbols to avoid clashes
    content = rename_sqlite3_symbols(content, prefix)

    output_path.write_text(content)
    print(f"  Written: {output_path}")


def generate_token_defs(output_path: Path, prefix: str, extension_grammar: Path | None = None) -> None:
    """Run Lemon to generate parse.h and extract TK_* definitions.

    If extension_grammar is provided, its %token declarations will be
    appended to generate additional TK_* definitions.
    """
    guard = f"{prefix.upper()}_SRC_TOKENIZER_SQLITE_TOKENS_H"

    # Build lemon using ninja
    lemon_exe = build_tool("lemon")

    with tempfile.TemporaryDirectory() as tmpdir:
        tmpdir = Path(tmpdir)

        # Read base parse.y
        base_grammar = (SQLITE_SRC / "parse.y").read_text()

        # If extension grammar provided, concatenate %token declarations
        if extension_grammar:
            ext_content = extension_grammar.read_text()
            # Find %token lines in extension and append them after SQLite's
            ext_tokens = re.findall(r'^%token\s+[^\n]+', ext_content, re.MULTILINE)
            if ext_tokens:
                # Insert extension tokens just before the first %% (rules section)
                first_rule_marker = base_grammar.find('\n%%\n')
                if first_rule_marker != -1:
                    insert_pos = first_rule_marker
                    extra_tokens = '\n'.join(ext_tokens) + '\n'
                    base_grammar = base_grammar[:insert_pos] + '\n' + extra_tokens + base_grammar[insert_pos:]

        # Write combined grammar
        (tmpdir / "parse.y").write_text(base_grammar)
        (tmpdir / "lempar.c").write_bytes((SQLITE_TOOL / "lempar.c").read_bytes())

        # Run lemon
        print("Running lemon to generate parse.h...")
        result = subprocess.run(
            [str(lemon_exe), str(tmpdir / "parse.y")],
            cwd=tmpdir,
            capture_output=True, text=True
        )
        if result.returncode != 0:
            print(f"Lemon failed: {result.stderr}", file=sys.stderr)
            sys.exit(1)

        # Read generated parse.h and extract TK_* defines
        parse_h = (tmpdir / "parse.h").read_text()

        header = f"""/*
** Token definitions from SQLite's parse.y via Lemon.
** DO NOT EDIT - regenerate with: python3 python/tools/extract_tokenizer.py
*/
#ifndef {guard}
#define {guard}

"""
        # Extract all #define TK_* lines
        defines = re.findall(r'^#define TK_\w+\s+\d+', parse_h, re.MULTILINE)

        footer = f"\n\n#endif /* {guard} */\n"

        output_path.write_text(header + "\n".join(defines) + footer)
        print(f"  Written: {output_path}")


def copy_global_tables(output_path: Path, prefix: str) -> None:
    """Extract character tables from global.c."""
    sqlite3_prefix = f"{prefix}_sqlite3"
    guard = f"{prefix.upper()}_SRC_TOKENIZER_SQLITE_TABLES_H"

    src = SQLITE_SRC / "global.c"
    content = src.read_text()

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

    header = f"""/*
** Character tables extracted from SQLite's global.c
** DO NOT EDIT - regenerate with: python3 python/tools/extract_tokenizer.py
*/
#ifndef {guard}
#define {guard}

/*
** Upper-to-lower case conversion table.
** SQLite only considers US-ASCII characters.
*/
static const unsigned char {sqlite3_prefix}UpperToLower[] = {{
"""

    tables = ""
    if upper_values:
        tables += upper_values.group(1).strip()

    tables += "\n};\n\n"

    # Add the ctype comment and map, renaming sqlite3CtypeMap
    tables += ctype_comment + "\n"
    ctype_renamed = ctype_block.replace("sqlite3CtypeMap", f"{sqlite3_prefix}CtypeMap")
    tables += "static " + ctype_renamed + "\n"

    # Extract character classification macros from sqliteInt.h
    sqliteint = (SQLITE_SRC / "sqliteInt.h").read_text()

    # Find the ASCII block of macros
    ascii_start = sqliteint.find("/*\n** The following macros mimic the standard library functions")
    if ascii_start == -1:
        print("Warning: Could not find character classification macros in sqliteInt.h", file=sys.stderr)
    else:
        # Find the #ifdef SQLITE_ASCII block
        ifdef_start = sqliteint.find("#ifdef SQLITE_ASCII", ascii_start)
        else_start = sqliteint.find("#else", ifdef_start)

        # Extract the comment and the ASCII macros
        macro_comment = sqliteint[ascii_start:sqliteint.find("*/", ascii_start) + 2]
        ascii_macros = sqliteint[ifdef_start + len("#ifdef SQLITE_ASCII"):else_start].strip()

        tables += "\n" + macro_comment + "\n"
        # Rename sqlite3 -> {prefix}_sqlite3 in the macros
        ascii_macros_renamed = rename_sqlite3_symbols(ascii_macros, prefix)
        tables += ascii_macros_renamed + "\n"

    footer = f"\n#endif /* {guard} */\n"

    output_path.write_text(header + tables + footer)
    print(f"  Written: {output_path}")


def generate_defs_h(output_path: Path, prefix: str) -> None:
    """Generate the {prefix}_defs.h header with type definitions and macros."""
    sqlite3_prefix = f"{prefix}_sqlite3"
    guard = f"{prefix.upper()}_SRC_TOKENIZER_SQLITE_{prefix.upper()}_DEFS_H"

    content = f"""/*
** Type definitions and macros for {prefix} tokenizer.
** DO NOT EDIT - regenerate with: python3 python/tools/extract_tokenizer.py
*/
#ifndef {guard}
#define {guard}

#include <assert.h>
#include <stdint.h>

/* SQLite type aliases */
typedef int64_t i64;
typedef uint8_t u8;
typedef uint32_t u32;

/* SQLite result codes */
#define SQLITE_OK    0
#define SQLITE_ERROR 1

/* ASCII mode - disables EBCDIC support */
#define SQLITE_ASCII 1

/* Digit separator for numeric literals (e.g., 1_000_000) */
#define SQLITE_DIGIT_SEPARATOR '_'

/* No-op test macro */
#define {sqlite3_prefix}Testcase(x)

/* C++17 fallthrough, C no-op */
#ifdef __cplusplus
#define deliberate_fall_through [[fallthrough]]
#else
#define deliberate_fall_through
#endif

#endif /* {guard} */
"""
    output_path.write_text(content)
    print(f"  Written: {output_path}")


def generate_tokenize_helper_h(output_path: Path, prefix: str, include_dir: str) -> None:
    """Generate the {prefix}_tokenize_helper.h header with forward declarations."""
    sqlite3_prefix = f"{prefix}_sqlite3"
    guard = f"{prefix.upper()}_SRC_TOKENIZER_SQLITE_{prefix.upper()}_TOKENIZE_HELPER_H"

    content = f"""/*
** Helper header for {prefix} tokenizer.
** DO NOT EDIT - regenerate with: python3 python/tools/extract_tokenizer.py
*/
#ifndef {guard}
#define {guard}

#include "{include_dir}/sqlite_keywordhash.h" // IWYU pragma: export

/* Forward declarations for functions defined in sqlite_tokenize.c */
i64 {sqlite3_prefix}GetToken(const unsigned char *z, int *tokenType);
int {sqlite3_prefix}AnalyzeWindowKeyword(const unsigned char *z);
int {sqlite3_prefix}AnalyzeOverKeyword(const unsigned char *z, int lastToken);
int {sqlite3_prefix}AnalyzeFilterKeyword(const unsigned char *z,
                                           int lastToken);

/* Stub for parser fallback - not needed for pure tokenization */
static inline int {sqlite3_prefix}ParserFallback(int token) {{
  (void)token;
  return 0;
}}

#endif /* {guard} */
"""
    output_path.write_text(content)
    print(f"  Written: {output_path}")


def main():
    parser = argparse.ArgumentParser(description="Extract SQLite tokenizer")
    parser.add_argument(
        "--extend-grammar",
        type=Path,
        default=None,
        help="Extension grammar file (.y) with additional token declarations"
    )
    parser.add_argument(
        "--prefix",
        type=str,
        default=DEFAULT_PREFIX,
        help=f"Prefix for generated symbols and filenames (default: {DEFAULT_PREFIX})"
    )
    parser.add_argument(
        "--include-dir",
        type=str,
        default=None,
        help="Include path prefix for #include directives (default: derived from output path)"
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=OUTPUT_DIR,
        help="Output directory for generated files"
    )
    args = parser.parse_args()

    if not SQLITE_SRC.exists():
        print(f"SQLite not found at {SQLITE_SRC}", file=sys.stderr)
        print("Run tools/dev/install-build-deps first", file=sys.stderr)
        return 1

    # Collect extra keywords from both sources
    extra = []

    # From --extend-grammar file
    if args.extend_grammar:
        if not args.extend_grammar.exists():
            print(f"Extension grammar not found: {args.extend_grammar}", file=sys.stderr)
            return 1
        grammar_keywords = parse_extension_grammar(args.extend_grammar)
        extra.extend(grammar_keywords)
        print(f"Extension grammar: {args.extend_grammar}")
        print(f"  Extracted keywords: {grammar_keywords}")

    if extra:
        print(f"Extra keywords: {extra}")

    args.output.mkdir(parents=True, exist_ok=True)

    # Determine include directory path
    include_dir = args.include_dir
    if include_dir is None:
        # Try to derive from output path relative to repo root
        try:
            include_dir = str(args.output.relative_to(ROOT_DIR))
        except ValueError:
            include_dir = str(args.output)

    print(f"Using prefix: {args.prefix}")
    print(f"Using include dir: {include_dir}")

    # Generate/extract files from SQLite source
    generate_defs_h(args.output / f"{args.prefix}_defs.h", args.prefix)
    generate_token_defs(args.output / "sqlite_tokens.h", args.prefix, args.extend_grammar)
    copy_global_tables(args.output / "sqlite_tables.h", args.prefix)
    build_keywordhash(extra, args.output / "sqlite_keywordhash.h", args.prefix, include_dir)
    generate_tokenize_helper_h(args.output / f"{args.prefix}_tokenize_helper.h", args.prefix, include_dir)
    copy_tokenize_c(args.output / "sqlite_tokenize.c", args.prefix, include_dir)

    print(f"\nDone! Generated files in {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
