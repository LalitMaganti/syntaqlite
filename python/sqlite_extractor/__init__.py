# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""SQLite code extraction abstraction layer.

This module provides composable, declarative, and validated operations for
extracting, transforming, and writing SQLite code. It replaces ad-hoc text
manipulation with safe, reusable abstractions.

Example usage:
    from sqlite_extractor import SourceFile, Pipeline, TruncateAt, SymbolRename, HeaderGenerator

    source = SourceFile.from_path(SQLITE_SRC / "tokenize.c")
    pipeline = Pipeline([
        TruncateAt("Run the parser on the given SQL string"),
        SymbolRename("sqlite3", "my_prefix_sqlite3"),
    ])
    content = pipeline.apply(source.content)
    HeaderGenerator(guard="MY_HEADER_H").write(output_path, content)
"""

from .source import SourceFile
from .transforms import (
    Transform,
    Pipeline,
    ReplaceText,
    TruncateAt,
    RemoveLines,
    RemoveRegex,
    SymbolRename,
    SymbolRenameExact,
    RemoveFunctionCalls,
    StripBlessingComment,
)
from .c_tokenizer import (
    TokenType,
    Token,
    tokenize_c,
    rename_symbols_safe,
)
from .generators import (
    HeaderGenerator,
    SourceFileGenerator,
    guard_from_path,
    SQLITE_BLESSING,
)
from .tools import ToolRunner

__all__ = [
    # source.py
    "SourceFile",
    # transforms.py
    "Transform",
    "Pipeline",
    "ReplaceText",
    "TruncateAt",
    "RemoveLines",
    "RemoveRegex",
    "SymbolRename",
    "SymbolRenameExact",
    "RemoveFunctionCalls",
    "StripBlessingComment",
    # c_tokenizer.py
    "TokenType",
    "Token",
    "tokenize_c",
    "rename_symbols_safe",
    # generators.py
    "HeaderGenerator",
    "SourceFileGenerator",
    "guard_from_path",
    "SQLITE_BLESSING",
    # tools.py
    "ToolRunner",
]
