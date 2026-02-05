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
from .select import ENUMS as _SELECT_ENUMS, NODES as _SELECT_NODES
from .column_ref import ENUMS as _COLREF_ENUMS, NODES as _COLREF_NODES
from .functions import ENUMS as _FUNC_ENUMS, NODES as _FUNC_NODES
from .conditionals import ENUMS as _COND_ENUMS, NODES as _COND_NODES
from .compound import ENUMS as _COMPOUND_ENUMS, NODES as _COMPOUND_NODES
from .misc_expr import ENUMS as _MISC_ENUMS, NODES as _MISC_NODES

ENUMS = _COMMON_ENUMS + _EXPR_ENUMS + _SELECT_ENUMS + _COLREF_ENUMS + _FUNC_ENUMS + _COND_ENUMS + _COMPOUND_ENUMS + _MISC_ENUMS
NODES = _COMMON_NODES + _EXPR_NODES + _SELECT_NODES + _COLREF_NODES + _FUNC_NODES + _COND_NODES + _COMPOUND_NODES + _MISC_NODES
