# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""SQLite code extraction abstraction layer.

This module provides composable, declarative, and validated operations for
extracting, transforming, and writing SQLite code. It replaces ad-hoc text
manipulation with safe, reusable abstractions.

Example usage:
    from sqlite_extractor import Pipeline, TruncateAt, SymbolRename, HeaderGenerator

    content = Path("tokenize.c").read_text()
    pipeline = Pipeline([
        TruncateAt("Run the parser on the given SQL string"),
        SymbolRename("sqlite3", "my_prefix_sqlite3"),
    ])
    content = pipeline.apply(content)
    HeaderGenerator(guard="MY_HEADER_H").write(output_path, content)
"""

from .transforms import (
    Transform,
    Pipeline,
    ReplaceText,
    TruncateAt,
    RemoveRegex,
    ReplaceRegex,
    RemoveSection,
    SymbolRename,
    SymbolRenameExact,
    RemoveFunctionCalls,
    StripBlessingComment,
)
from .generators import (
    HeaderGenerator,
    SourceFileGenerator,
    SQLITE_BLESSING,
)
from .tools import (
    ToolRunner,
    create_parser_symbol_rename_pipeline,
    create_keywordhash_rename_pipeline,
)
from .grammar_build import (
    build_synq_grammar,
    split_extension_grammar,
)
from .parser_split import (
    split_parser_output,
)
from .parser_extract import (
    extract_parser_data,
    format_parser_data_header,
    format_extension_reduce_function,
)

__all__ = [
    # transforms.py
    "Transform",
    "Pipeline",
    "ReplaceText",
    "TruncateAt",
    "RemoveRegex",
    "ReplaceRegex",
    "RemoveSection",
    "SymbolRename",
    "SymbolRenameExact",
    "RemoveFunctionCalls",
    "StripBlessingComment",
    # generators.py
    "HeaderGenerator",
    "SourceFileGenerator",
    "SQLITE_BLESSING",
    # tools.py
    "ToolRunner",
    "create_parser_symbol_rename_pipeline",
    "create_keywordhash_rename_pipeline",
    # grammar_build.py
    "build_synq_grammar",
    "split_extension_grammar",
    # parser_split.py
    "split_parser_output",
    # parser_extract.py
    "extract_parser_data",
    "format_parser_data_header",
    "format_extension_reduce_function",
]
