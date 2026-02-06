# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""AST node type definitions for syntaqlite.

Design rules:
- Non-cyclic data -> inline: Scalars, flags, source offsets/lengths
- Cyclic references -> index: When type A can contain type B which can contain A
- No pointers: Only indices (node IDs) and offsets (into source text)
- Variable sizes: Each node type uses exactly the space it needs

For nullable index fields, use SYNTAQLITE_NULL_NODE (0xFFFFFFFF).
"""

# Order matters for binary compatibility: enums are assigned IDs sequentially,
# and nodes are assigned type tags sequentially. Do not reorder these imports
# or the lists below without updating all generated code.

from ._common import ENUMS as _COMMON_ENUMS, NODES as _COMMON_NODES
from .expressions import ENUMS as _EXPR_ENUMS, NODES as _EXPR_NODES
from .select import ENUMS as _SELECT_ENUMS, NODES as _SELECT_NODES, FLAGS as _SELECT_FLAGS
from .column_ref import ENUMS as _COLREF_ENUMS, NODES as _COLREF_NODES
from .functions import ENUMS as _FUNC_ENUMS, NODES as _FUNC_NODES, FLAGS as _FUNC_FLAGS
from .conditionals import ENUMS as _COND_ENUMS, NODES as _COND_NODES
from .compound import ENUMS as _COMPOUND_ENUMS, NODES as _COMPOUND_NODES
from .misc_expr import ENUMS as _MISC_ENUMS, NODES as _MISC_NODES
from .cast import ENUMS as _CAST_ENUMS, NODES as _CAST_NODES
from .values import ENUMS as _VALUES_ENUMS, NODES as _VALUES_NODES
from .cte import ENUMS as _CTE_ENUMS, NODES as _CTE_NODES
from .aggregate import ENUMS as _AGG_ENUMS, NODES as _AGG_NODES, FLAGS as _AGG_FLAGS
from .raise_expr import ENUMS as _RAISE_ENUMS, NODES as _RAISE_NODES
from .table_source import ENUMS as _TABSRC_ENUMS, NODES as _TABSRC_NODES
from .dml import ENUMS as _DML_ENUMS, NODES as _DML_NODES
from .schema_ops import ENUMS as _SCHEMA_ENUMS, NODES as _SCHEMA_NODES
from .utility_stmts import ENUMS as _UTIL_ENUMS, NODES as _UTIL_NODES
from .create_table import ENUMS as _CTABLE_ENUMS, NODES as _CTABLE_NODES
from .window import ENUMS as _WINDOW_ENUMS, NODES as _WINDOW_NODES
from .trigger import ENUMS as _TRIGGER_ENUMS, NODES as _TRIGGER_NODES

ENUMS = _COMMON_ENUMS + _EXPR_ENUMS + _SELECT_ENUMS + _COLREF_ENUMS + _FUNC_ENUMS + _COND_ENUMS + _COMPOUND_ENUMS + _MISC_ENUMS + _CAST_ENUMS + _VALUES_ENUMS + _CTE_ENUMS + _AGG_ENUMS + _RAISE_ENUMS + _TABSRC_ENUMS + _DML_ENUMS + _SCHEMA_ENUMS + _UTIL_ENUMS + _CTABLE_ENUMS + _WINDOW_ENUMS + _TRIGGER_ENUMS
NODES = _COMMON_NODES + _EXPR_NODES + _SELECT_NODES + _COLREF_NODES + _FUNC_NODES + _COND_NODES + _COMPOUND_NODES + _MISC_NODES + _CAST_NODES + _VALUES_NODES + _CTE_NODES + _AGG_NODES + _RAISE_NODES + _TABSRC_NODES + _DML_NODES + _SCHEMA_NODES + _UTIL_NODES + _CTABLE_NODES + _WINDOW_NODES + _TRIGGER_NODES
FLAGS = _SELECT_FLAGS + _FUNC_FLAGS + _AGG_FLAGS
