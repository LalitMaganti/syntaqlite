# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Extract SQLite tokenization code.

Following Perfetto's approach:
1. Build mkkeywordhash using GN/ninja
2. Run mkkeywordhash to generate keywordhash data
3. Copy tokenize.c with minimal changes
4. Generate helper header files

Usage:
    python3 python/tools/extract_tokenizer.py
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
OUTPUT_DIR = ROOT_DIR / "src" / "tokenizer"

# Dedicated build directory for this script
TOOLS_BUILD_DIR = ROOT_DIR / "out" / ".extract_tokenizer"

# Default prefix for renamed sqlite3 symbols to avoid clashes
DEFAULT_PREFIX = "syntaqlite"

# SQLite's blessing comment for generated files
SQLITE_BLESSING = """**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
"""


def build_tool(name: str) -> Path:
    """Build a tool and return its path."""
    build_ninja = TOOLS_BUILD_DIR / "build.ninja"
    if not build_ninja.exists():
        print(f"Configuring build in {TOOLS_BUILD_DIR}...")
        TOOLS_BUILD_DIR.mkdir(parents=True, exist_ok=True)
        subprocess.run(
            [str(ROOT_DIR / "tools" / "dev" / "gn"), "gen", str(TOOLS_BUILD_DIR), "--args=is_debug=false"],
            capture_output=True, text=True,
        )
        if not build_ninja.exists():
            print("Failed to configure build", file=sys.stderr)
            sys.exit(1)

    print(f"Building {name}...")
    result = subprocess.run(
        [str(ROOT_DIR / "tools" / "dev" / "ninja"), "-C", str(TOOLS_BUILD_DIR), name],
        capture_output=True, text=True,
    )
    if result.returncode != 0:
        print(f"Build failed:\n{result.stderr}", file=sys.stderr)
        sys.exit(1)

    return TOOLS_BUILD_DIR / (f"{name}.exe" if os.name == "nt" else name)


def find_blessing_end(content: str) -> int:
    marker = content.find("May you share freely")
    if marker == -1:
        return 0
    end = content.find("*/", marker)
    return end + 2 if end != -1 else 0


def rename_sqlite3_symbols(content: str, prefix: str) -> str:
    sqlite3_prefix = f"{prefix}_sqlite3"

    # Rename sqlite3XxxYyy -> {prefix}_sqlite3XxxYyy
    content = re.sub(r'\bsqlite3([A-Z_][A-Za-z0-9_]*)', f'{sqlite3_prefix}\\1', content)

    # Rename testcase() which doesn't have sqlite3 prefix
    content = content.replace('testcase(', f'{sqlite3_prefix}Testcase(')

    # Remove the charMap macro and use Tolower from sqlite_tables.h instead
    content = re.sub(r'#\s*define\s+charMap\(X\)[^\n]*\n', '', content)
    content = content.replace('charMap(', f'{sqlite3_prefix}Tolower(')

    # Rename keywordCode to have proper prefix and remove static
    # Use word boundary to avoid double replacement
    content = re.sub(r'\bstatic int keywordCode\b', f'int {prefix}_keywordCode', content)
    content = re.sub(r'\bkeywordCode\b', f'{prefix}_keywordCode', content)

    # Remove Testcase calls from keywordCode function (including the entire line)
    content = re.sub(rf'[ \t]*{sqlite3_prefix}Testcase\([^)]*\);[^\n]*\n', '', content)

    return content


def build_keywordhash(output_dir: Path, prefix: str, include_dir: str) -> str:
    """Build keyword hash data file.

    Returns the keywordCode function source to be inlined into sqlite_tokenize.c.
    """
    sqlite3_prefix = f"{prefix}_sqlite3"
    data_guard = f"{prefix.upper()}_SRC_TOKENIZER_KEYWORDHASH_DATA_H"

    # Build using ninja
    exe = build_tool("mkkeywordhash")

    with tempfile.TemporaryDirectory() as tmpdir:
        tmpdir = Path(tmpdir)

        # Run and capture output
        print(f"Running mkkeywordhash...")
        result = subprocess.run([str(exe)], capture_output=True, text=True)
        if result.returncode != 0:
            print(f"Run failed: {result.stderr}", file=sys.stderr)
            sys.exit(1)

        # Process output - rename sqlite3* symbols
        output = rename_sqlite3_symbols(result.stdout, prefix)

        # Find start of actual generated content (skip the SQLite header comment)
        start_marker = "/* Hash score:"
        start = output.find(start_marker)
        if start == -1:
            start = 0

        generated = output[start:]

        # Find keywordCode function
        keyword_code_marker = f"int {prefix}_keywordCode("
        keyword_code_start = generated.find(keyword_code_marker)
        if keyword_code_start == -1:
            print("Error: Could not find keywordCode function", file=sys.stderr)
            sys.exit(1)

        # Get data section (everything before keywordCode)
        data_section = generated[:keyword_code_start]

        # Get logic section (keywordCode function only)
        logic_section = generated[keyword_code_start:]
        keyword_code_end = logic_section.find('\n}\n')
        if keyword_code_end != -1:
            logic_section = logic_section[:keyword_code_end + 3]

        # Write data file (data arrays only)
        data_header = f"""/*
{SQLITE_BLESSING}** Keyword hash data for {prefix} tokenizer.
** DO NOT EDIT - regenerate with: python3 python/tools/extract_tokenizer.py
*/
#ifndef {data_guard}
#define {data_guard}

#include "{include_dir}/sqlite_tokens.h"

"""
        data_footer = f"\n#endif /* {data_guard} */\n"

        data_path = output_dir / "sqlite_keywordhash_data.h"
        data_path.write_text(data_header + data_section.strip() + "\n" + data_footer)
        print(f"  Written: {data_path}")

        return logic_section.strip()


def copy_tokenize_c(output_path: Path, prefix: str, include_dir: str, keyword_code_func: str) -> None:
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

    # Replace the SQLite internal header with our includes and inlined code
    inlined_header = f"""#include "{include_dir}/{prefix}_defs.h"
#include "{include_dir}/sqlite_tables.h"

/* Keyword hash data - injectable via compile flag */
#ifndef SYNTAQLITE_KEYWORDHASH_DATA_FILE
#include "{include_dir}/sqlite_keywordhash_data.h"
#else
#include SYNTAQLITE_KEYWORDHASH_DATA_FILE
#endif

/* Stub for parser fallback - not needed for pure tokenization */
static inline int {sqlite3_prefix}ParserFallback(int token) {{
  (void)token;
  return 0;
}}

/* Forward declaration */
i64 {sqlite3_prefix}GetToken(const unsigned char *z, int *tokenType);

/* Keyword lookup function */
{keyword_code_func}
"""
    content = content.replace('#include "sqliteInt.h"', inlined_header)

    # Remove keywordhash.h include - already included above
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
{SQLITE_BLESSING}** SQLite tokenizer for {prefix}.
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


def generate_token_defs(output_path: Path, prefix: str) -> None:
    guard = f"{prefix.upper()}_SRC_TOKENIZER_TOKENS_H"

    # Build lemon using ninja
    lemon_exe = build_tool("lemon")

    with tempfile.TemporaryDirectory() as tmpdir:
        tmpdir = Path(tmpdir)

        # Copy parse.y
        (tmpdir / "parse.y").write_text((SQLITE_SRC / "parse.y").read_text())
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
{SQLITE_BLESSING}** Token definitions from SQLite's parse.y via Lemon.
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
    sqlite3_prefix = f"{prefix}_sqlite3"
    guard = f"{prefix.upper()}_SRC_TOKENIZER_TABLES_H"

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
{SQLITE_BLESSING}** Character tables extracted from SQLite's global.c
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


def main():
    parser = argparse.ArgumentParser(description="Extract SQLite tokenizer")
    parser.add_argument("--prefix", default=DEFAULT_PREFIX, help=f"Symbol prefix (default: {DEFAULT_PREFIX})")
    parser.add_argument("--include-dir", help="Include path prefix (default: derived from output)")
    parser.add_argument("--output", type=Path, default=OUTPUT_DIR, help="Output directory")
    args = parser.parse_args()

    if not SQLITE_SRC.exists():
        print(f"SQLite not found at {SQLITE_SRC}. Run tools/dev/install-build-deps first", file=sys.stderr)
        return 1

    args.output.mkdir(parents=True, exist_ok=True)
    try:
        include_dir = args.include_dir or str(args.output.relative_to(ROOT_DIR))
    except ValueError:
        include_dir = args.include_dir or str(args.output)

    prefix = args.prefix
    print(f"Using prefix: {prefix}")
    print(f"Using include dir: {include_dir}")

    generate_token_defs(args.output / "sqlite_tokens.h", prefix)
    copy_global_tables(args.output / "sqlite_tables.h", prefix)
    keyword_code_func = build_keywordhash(args.output, prefix, include_dir)

    copy_tokenize_c(args.output / "sqlite_tokenize.c", prefix, include_dir, keyword_code_func)
    print(f"\nDone! Generated files in {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
