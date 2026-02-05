# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Build tool wrapper for SQLite extraction tools."""

from __future__ import annotations

import os
import subprocess
import sys
import tempfile
from dataclasses import dataclass, field
from pathlib import Path

from .transforms import Pipeline, SymbolRenameExact

# Keywordhash array symbols that need renaming
KEYWORDHASH_ARRAY_SYMBOLS = [
    "zKWText",
    "aKWHash",
    "aKWNext",
    "aKWLen",
    "aKWOffset",
    "aKWCode",
]


def create_parser_symbol_rename_pipeline(prefix: str) -> Pipeline:
    """Create a pipeline for renaming Lemon-generated parser symbols.

    This pipeline renames the public API symbols (Parse*, etc.) and
    internal symbols (yyParser, yyStackEntry) to use the specified prefix.
    """
    sqlite3_prefix = f"{prefix}_sqlite3"

    return Pipeline([
        # Public API symbols: Parse -> {prefix}_sqlite3Parser
        SymbolRenameExact("Parse", f"{sqlite3_prefix}Parser"),
        SymbolRenameExact("ParseAlloc", f"{sqlite3_prefix}ParserAlloc"),
        SymbolRenameExact("ParseFree", f"{sqlite3_prefix}ParserFree"),
        SymbolRenameExact("ParseInit", f"{sqlite3_prefix}ParserInit"),
        SymbolRenameExact("ParseFinalize", f"{sqlite3_prefix}ParserFinalize"),
        SymbolRenameExact("ParseTrace", f"{sqlite3_prefix}ParserTrace"),
        SymbolRenameExact("ParseFallback", f"{sqlite3_prefix}ParserFallback"),
        SymbolRenameExact("ParseCoverage", f"{sqlite3_prefix}ParserCoverage"),
        SymbolRenameExact("ParseStackPeak", f"{sqlite3_prefix}ParserStackPeak"),
        # Internal symbols: yyParser -> {prefix}_yyParser
        SymbolRenameExact("yyParser", f"{prefix}_yyParser"),
        SymbolRenameExact("yyStackEntry", f"{prefix}_yyStackEntry"),
    ])


def create_keywordhash_rename_pipeline(prefix: str) -> Pipeline:
    """Create a pipeline for renaming keywordhash array symbols.

    This pipeline renames the six keywordhash arrays (zKWText, aKWHash, etc.)
    to use the specified prefix.
    """
    return Pipeline([
        SymbolRenameExact(symbol, f"{prefix}_{symbol}")
        for symbol in KEYWORDHASH_ARRAY_SYMBOLS
    ])


@dataclass
class ToolRunner:
    """Wrapper for building and running SQLite extraction tools."""

    root_dir: Path
    _build_dir: Path | None = field(default=None, repr=False)
    verbose: bool = False
    _tool_paths: dict[str, Path] = field(default_factory=dict, repr=False)

    @property
    def build_dir(self) -> Path:
        if self._build_dir is None:
            return self.root_dir / "out" / ".tool_cache"
        return self._build_dir

    @property
    def sqlite_src(self) -> Path:
        return self.root_dir / "third_party" / "src" / "sqlite" / "src"

    @property
    def sqlite_tool(self) -> Path:
        return self.root_dir / "third_party" / "src" / "sqlite" / "tool"

    def _run(self, cmd: list[str], description: str, **kwargs) -> subprocess.CompletedProcess:
        """Run a command with standard error handling."""
        result = subprocess.run(cmd, capture_output=True, text=True, **kwargs)
        if result.returncode != 0:
            if self.verbose:
                print(f"Output: {result.stdout}")
            print(f"{description} failed: {result.stderr}", file=sys.stderr)
            raise RuntimeError(f"{description} failed")
        return result

    def build(self, target: str) -> Path:
        """Build a target and return the path to the executable."""
        if target in self._tool_paths:
            return self._tool_paths[target]

        # Configure if needed
        build_ninja = self.build_dir / "build.ninja"
        if not build_ninja.exists():
            print(f"Configuring build in {self.build_dir}...")
            self.build_dir.mkdir(parents=True, exist_ok=True)
            gn = self.root_dir / "tools" / "dev" / "gn"
            self._run([str(gn), "gen", str(self.build_dir), "--args=is_debug=false"], "GN configure")

        # Build
        print(f"Building {target}...")
        ninja = self.root_dir / "tools" / "dev" / "ninja"
        self._run([str(ninja), "-C", str(self.build_dir), target], f"Build {target}")

        exe_name = f"{target}.exe" if os.name == "nt" else target
        exe_path = self.build_dir / exe_name
        if not exe_path.exists():
            raise RuntimeError(f"Built executable not found: {exe_path}")

        self._tool_paths[target] = exe_path
        return exe_path

    def _run_lemon(
        self,
        grammar_path: Path,
        output_dir: Path,
        template_path: Path | None = None,
        flags: list[str] | None = None,
    ) -> subprocess.CompletedProcess:
        """Core lemon execution logic."""
        lemon_exe = self.build("lemon")

        # Set up template
        lempar_dst = output_dir / "lempar.c"
        template_src = template_path or (self.sqlite_tool / "lempar.c")
        if template_src != lempar_dst:
            lempar_dst.write_bytes(template_src.read_bytes())

        cmd = [str(lemon_exe)] + (flags or []) + [str(grammar_path)]
        return subprocess.run(cmd, cwd=output_dir, capture_output=True, text=True)

    def run_lemon(self, grammar_path: Path, output_dir: Path | None = None) -> Path:
        """Run lemon and return path to generated .h file."""
        output_dir = output_dir or grammar_path.parent
        print("Running lemon to generate parse.h...")

        result = self._run_lemon(grammar_path, output_dir, flags=["-l"])
        if result.returncode != 0:
            if self.verbose:
                print(f"Lemon output: {result.stdout}")
            print(f"Lemon failed: {result.stderr}", file=sys.stderr)
            raise RuntimeError("Lemon parser generator failed")

        parse_h = output_dir / "parse.h"
        if not parse_h.exists():
            parse_h = grammar_path.with_suffix(".h")
        if not parse_h.exists():
            raise RuntimeError(f"Lemon did not generate parse.h in {output_dir}")
        return parse_h

    def run_lemon_with_template(
        self,
        grammar_path: Path,
        template_path: Path,
        output_dir: Path | None = None,
    ) -> tuple[Path, Path]:
        """Run lemon with a custom template, returning (parse.c, parse.h)."""
        output_dir = output_dir or grammar_path.parent
        print("Running lemon with custom template...")

        result = self._run_lemon(grammar_path, output_dir, template_path, flags=["-l"])

        base_name = grammar_path.stem
        parse_c, parse_h = output_dir / f"{base_name}.c", output_dir / f"{base_name}.h"

        if not parse_c.exists() or not parse_h.exists():
            if self.verbose:
                print(f"Lemon output: {result.stdout}")
            print(f"Lemon failed: {result.stderr}", file=sys.stderr)
            raise RuntimeError("Lemon parser generator failed")

        if result.stderr:
            print(f"Lemon warnings: {result.stderr}")
        return parse_c, parse_h

    def run_lemon_grammar_only(self, grammar_path: Path) -> str:
        """Run lemon -g to get clean grammar rules."""
        lemon_exe = self.build("lemon")
        print("Running lemon -g to extract clean grammar rules...")
        result = self._run([str(lemon_exe), "-g", str(grammar_path)], "Lemon -g")
        return result.stdout

    def run_mkkeywordhash(
        self,
        extra_keywords: list[str] | None = None,
        custom_source: str | None = None,
    ) -> str:
        """Run mkkeywordhash to generate keyword hash data."""
        if not extra_keywords and not custom_source:
            exe = self.build("mkkeywordhash")
            print("Running mkkeywordhash...")
            return self._run([str(exe)], "mkkeywordhash").stdout

        # Custom version needed
        with tempfile.TemporaryDirectory() as tmpdir:
            tmpdir = Path(tmpdir)

            source = custom_source or (self.sqlite_tool / "mkkeywordhash.c").read_text()
            if extra_keywords and not custom_source:
                source = self._add_keywords_to_source(source, extra_keywords)

            src_path = tmpdir / "mkkeywordhash.c"
            src_path.write_text(source)

            exe_path = tmpdir / ("mkkeywordhash.exe" if os.name == "nt" else "mkkeywordhash")
            cc = os.environ.get("CC", "clang")

            print("Compiling custom mkkeywordhash...")
            self._run([cc, "-o", str(exe_path), str(src_path)], "Compile mkkeywordhash")

            print("Running mkkeywordhash...")
            return self._run([str(exe_path)], "mkkeywordhash").stdout

    def _add_keywords_to_source(self, source: str, extra_keywords: list[str]) -> str:
        """Add extra keywords to mkkeywordhash.c source."""
        from .generators import format_keyword_table_entry

        keyword_table_end = '''  { "WITHOUT",          "TK_WITHOUT",      ALWAYS,           1      },
};'''
        lines = ['  { "WITHOUT",          "TK_WITHOUT",      ALWAYS,           1      },']
        lines.extend(format_keyword_table_entry(kw) for kw in extra_keywords)
        lines.append("};")
        return source.replace(keyword_table_end, "\n".join(lines))

    def get_base_grammar(self) -> str:
        """Get SQLite's base parse.y grammar content."""
        return (self.sqlite_src / "parse.y").read_text()

    def get_lempar_path(self) -> Path:
        """Get path to lemon's lempar.c template."""
        return self.sqlite_tool / "lempar.c"
