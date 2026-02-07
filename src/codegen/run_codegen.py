#!/usr/bin/env python3
# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Run synq-codegen to generate AST and formatter code.

GN action entry point. Takes a pre-built synq-codegen binary (built by
build_synq_codegen.py) and runs it to produce all generated files.
"""

import argparse
import shutil
import subprocess
import sys
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description="Run synq-codegen")
    parser.add_argument("--synq-codegen-bin", type=Path, required=True,
                        help="Path to pre-built synq-codegen binary")
    parser.add_argument("--output", type=Path, required=True,
                        help="Output directory (maps to $target_gen_dir/src)")
    parser.add_argument("--nodes-dir", type=Path, required=True,
                        help="Directory containing node JSON definitions")
    args = parser.parse_args()

    binary = args.synq_codegen_bin.resolve()
    output = args.output.resolve()
    nodes_dir = args.nodes_dir.resolve()

    # synq-codegen writes all files into --out-dir:
    #   ast_nodes_gen.h, ast_nodes_gen_internal.h, ast_builder_gen.h,
    #   ast_builder_gen.c, ast_print_gen.c, fmt_gen.c
    parser_dir = output / "parser"
    parser_dir.mkdir(parents=True, exist_ok=True)

    subprocess.check_call([
        str(binary), "generate",
        "--nodes-dir", str(nodes_dir),
        "--out-dir", str(parser_dir),
    ])

    # Move fmt_gen.c to the formatter directory
    fmt_dir = output / "formatter"
    fmt_dir.mkdir(parents=True, exist_ok=True)
    shutil.move(str(parser_dir / "fmt_gen.c"), str(fmt_dir / "fmt_gen.c"))

    # Copy public header to the include directory
    include_dir = output.parent / "include" / "syntaqlite"
    include_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(
        str(parser_dir / "ast_nodes_gen.h"),
        str(include_dir / "ast_nodes_gen.h"),
    )

    return 0


if __name__ == "__main__":
    sys.exit(main())
