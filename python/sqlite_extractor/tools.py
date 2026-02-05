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
    """Wrapper for building and running SQLite extraction tools.

    Consolidates the build logic for tools like lemon and mkkeywordhash
    that are used to extract SQLite code.

    Example:
        runner = ToolRunner(root_dir=Path("."))
        lemon_exe = runner.build("lemon")
        result = runner.run_lemon(grammar_path)
    """

    root_dir: Path
    build_dir: Path | None = None
    verbose: bool = False

    # Cached tool paths
    _tool_paths: dict[str, Path] = field(default_factory=dict, repr=False)

    def __post_init__(self) -> None:
        if self.build_dir is None:
            self.build_dir = self.root_dir / "out" / ".tool_cache"

    @property
    def gn_path(self) -> Path:
        return self.root_dir / "tools" / "dev" / "gn"

    @property
    def ninja_path(self) -> Path:
        return self.root_dir / "tools" / "dev" / "ninja"

    @property
    def sqlite_src(self) -> Path:
        return self.root_dir / "third_party" / "src" / "sqlite" / "src"

    @property
    def sqlite_tool(self) -> Path:
        return self.root_dir / "third_party" / "src" / "sqlite" / "tool"

    def _configure_build(self) -> bool:
        """Configure the build directory if needed."""
        build_ninja = self.build_dir / "build.ninja"
        if build_ninja.exists():
            return True

        print(f"Configuring build in {self.build_dir}...")
        self.build_dir.mkdir(parents=True, exist_ok=True)

        result = subprocess.run(
            [str(self.gn_path), "gen", str(self.build_dir), "--args=is_debug=false"],
            capture_output=True,
            text=True,
        )

        if not build_ninja.exists():
            if self.verbose:
                print(f"GN output: {result.stdout}")
                print(f"GN errors: {result.stderr}")
            print("Failed to configure build", file=sys.stderr)
            return False

        return True

    def build(self, target: str) -> Path:
        """Build a target and return the path to the executable.

        Args:
            target: The build target name (e.g., "lemon", "mkkeywordhash").

        Returns:
            Path to the built executable.

        Raises:
            RuntimeError: If the build fails.
        """
        if target in self._tool_paths:
            return self._tool_paths[target]

        if not self._configure_build():
            raise RuntimeError(f"Failed to configure build for {target}")

        print(f"Building {target}...")
        result = subprocess.run(
            [str(self.ninja_path), "-C", str(self.build_dir), target],
            capture_output=True,
            text=True,
        )

        if result.returncode != 0:
            if self.verbose:
                print(f"Ninja output: {result.stdout}")
            print(f"Build failed:\n{result.stderr}", file=sys.stderr)
            raise RuntimeError(f"Failed to build {target}")

        # Determine executable path
        exe_name = f"{target}.exe" if os.name == "nt" else target
        exe_path = self.build_dir / exe_name

        if not exe_path.exists():
            raise RuntimeError(f"Built executable not found: {exe_path}")

        self._tool_paths[target] = exe_path
        return exe_path

    def run_lemon(self, grammar_path: Path, output_dir: Path | None = None) -> Path:
        """Run lemon parser generator on a grammar file.

        Args:
            grammar_path: Path to the .y grammar file.
            output_dir: Directory for output files. If None, uses grammar_path's dir.

        Returns:
            Path to the generated .h file.

        Raises:
            RuntimeError: If lemon fails.
        """
        lemon_exe = self.build("lemon")

        if output_dir is None:
            output_dir = grammar_path.parent

        # Copy lempar.c template to output directory if needed
        lempar_dst = output_dir / "lempar.c"
        if not lempar_dst.exists():
            lempar_src = self.sqlite_tool / "lempar.c"
            lempar_dst.write_bytes(lempar_src.read_bytes())

        print("Running lemon to generate parse.h...")
        result = subprocess.run(
            [str(lemon_exe), "-l", str(grammar_path)],
            cwd=output_dir,
            capture_output=True,
            text=True,
        )

        if result.returncode != 0:
            if self.verbose:
                print(f"Lemon output: {result.stdout}")
            print(f"Lemon failed: {result.stderr}", file=sys.stderr)
            raise RuntimeError("Lemon parser generator failed")

        parse_h = output_dir / "parse.h"
        if not parse_h.exists():
            # Try with same name as grammar
            parse_h = grammar_path.with_suffix(".h")

        if not parse_h.exists():
            raise RuntimeError(f"Lemon did not generate parse.h in {output_dir}")

        return parse_h

    def run_mkkeywordhash(
        self,
        extra_keywords: list[str] | None = None,
        custom_source: str | None = None,
    ) -> str:
        """Run mkkeywordhash to generate keyword hash data.

        Args:
            extra_keywords: Additional keywords to add to the hash table.
            custom_source: Custom mkkeywordhash.c source (for modified versions).

        Returns:
            The stdout output from mkkeywordhash.

        Raises:
            RuntimeError: If mkkeywordhash fails.
        """
        if extra_keywords or custom_source:
            # Need to compile a custom version
            return self._run_custom_mkkeywordhash(extra_keywords, custom_source)

        # Use the standard built version
        exe = self.build("mkkeywordhash")

        print("Running mkkeywordhash...")
        result = subprocess.run([str(exe)], capture_output=True, text=True)

        if result.returncode != 0:
            print(f"mkkeywordhash failed: {result.stderr}", file=sys.stderr)
            raise RuntimeError("mkkeywordhash failed")

        return result.stdout

    def _run_custom_mkkeywordhash(
        self,
        extra_keywords: list[str] | None,
        custom_source: str | None,
    ) -> str:
        """Compile and run a custom mkkeywordhash."""
        with tempfile.TemporaryDirectory() as tmpdir:
            tmpdir = Path(tmpdir)

            # Get source - either custom or modified original
            if custom_source:
                source = custom_source
            else:
                source = (self.sqlite_tool / "mkkeywordhash.c").read_text()

                # Add extra keywords to the source
                if extra_keywords:
                    source = self._add_keywords_to_source(source, extra_keywords)

            # Write and compile
            src_path = tmpdir / "mkkeywordhash.c"
            src_path.write_text(source)

            exe_path = tmpdir / ("mkkeywordhash.exe" if os.name == "nt" else "mkkeywordhash")

            cc = os.environ.get("CC", "clang")
            print(f"Compiling custom mkkeywordhash...")
            result = subprocess.run(
                [cc, "-o", str(exe_path), str(src_path)],
                capture_output=True,
                text=True,
            )

            if result.returncode != 0:
                print(f"Compile failed: {result.stderr}", file=sys.stderr)
                raise RuntimeError("Failed to compile custom mkkeywordhash")

            # Run
            print("Running mkkeywordhash...")
            result = subprocess.run([str(exe_path)], capture_output=True, text=True)

            if result.returncode != 0:
                print(f"mkkeywordhash failed: {result.stderr}", file=sys.stderr)
                raise RuntimeError("mkkeywordhash failed")

            return result.stdout

    def _add_keywords_to_source(self, source: str, extra_keywords: list[str]) -> str:
        """Add extra keywords to mkkeywordhash.c source."""
        from .generators import format_keyword_table_entry

        # Find the end of the keyword table (after WITHOUT entry)
        keyword_table_end = '''  { "WITHOUT",          "TK_WITHOUT",      ALWAYS,           1      },
};'''

        # Generate new entries
        lines = ['  { "WITHOUT",          "TK_WITHOUT",      ALWAYS,           1      },']
        for kw in extra_keywords:
            lines.append(format_keyword_table_entry(kw))
        lines.append("};")

        replacement = "\n".join(lines)
        return source.replace(keyword_table_end, replacement)

    def get_base_grammar(self) -> str:
        """Get SQLite's base parse.y grammar content."""
        return (self.sqlite_src / "parse.y").read_text()

    def get_lempar_template(self) -> bytes:
        """Get lemon's lempar.c template."""
        return (self.sqlite_tool / "lempar.c").read_bytes()

    def get_lempar_path(self) -> Path:
        """Get path to lemon's lempar.c template."""
        return self.sqlite_tool / "lempar.c"

    def run_lemon_grammar_only(self, grammar_path: Path) -> str:
        """Run lemon with -g flag to get clean grammar rules.

        Args:
            grammar_path: Path to the .y grammar file.

        Returns:
            String containing clean grammar rules without semantic actions.

        Raises:
            RuntimeError: If lemon fails.
        """
        lemon_exe = self.build("lemon")

        print("Running lemon -g to extract clean grammar rules...")
        result = subprocess.run(
            [str(lemon_exe), "-g", str(grammar_path)],
            capture_output=True,
            text=True,
        )

        if result.returncode != 0:
            if self.verbose:
                print(f"Lemon output: {result.stdout}")
            print(f"Lemon -g failed: {result.stderr}", file=sys.stderr)
            raise RuntimeError("Lemon -g failed")

        return result.stdout

    def run_lemon_with_template(
        self,
        grammar_path: Path,
        template_path: Path,
        output_dir: Path | None = None,
    ) -> tuple[Path, Path]:
        """Run lemon parser generator with a custom template.

        Args:
            grammar_path: Path to the .y grammar file.
            template_path: Path to custom lempar.c template.
            output_dir: Directory for output files. If None, uses grammar_path's dir.

        Returns:
            Tuple of (parse.c path, parse.h path).

        Raises:
            RuntimeError: If lemon fails.
        """
        lemon_exe = self.build("lemon")

        if output_dir is None:
            output_dir = grammar_path.parent

        # Copy template to output directory as lempar.c
        lempar_dst = output_dir / "lempar.c"
        if template_path != lempar_dst:
            lempar_dst.write_bytes(template_path.read_bytes())

        print(f"Running lemon with custom template...")
        result = subprocess.run(
            [str(lemon_exe), "-l", str(grammar_path)],
            cwd=output_dir,
            capture_output=True,
            text=True,
        )

        # Determine output file names based on grammar file name
        base_name = grammar_path.stem
        parse_c = output_dir / f"{base_name}.c"
        parse_h = output_dir / f"{base_name}.h"

        # Lemon returns non-zero on conflicts, but still generates output.
        # Check if files exist rather than relying on exit code.
        if not parse_c.exists() or not parse_h.exists():
            if self.verbose:
                print(f"Lemon output: {result.stdout}")
            print(f"Lemon failed: {result.stderr}", file=sys.stderr)
            raise RuntimeError("Lemon parser generator failed")

        # Print any warnings (like conflicts)
        if result.stderr:
            print(f"Lemon warnings: {result.stderr}")

        return parse_c, parse_h
