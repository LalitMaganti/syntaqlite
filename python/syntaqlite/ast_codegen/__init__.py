# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""AST code generation tools for syntaqlite.

Usage:
    from python.syntaqlite.ast_codegen import codegen, validator
    from python.syntaqlite.ast_codegen.nodes import NODES, ENUMS

    errors = validator.validate_node_references(NODES)
    codegen.generate_all(NODES, ENUMS, output_dir)
"""
