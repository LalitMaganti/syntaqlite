"""Build the _syntaqlite C extension for local development.

Usage:
    cd python && pip install -e .
    # or
    cd python && python setup.py build_ext --inplace
"""

import os
import subprocess
import sys
from pathlib import Path

from setuptools import Extension, setup

ROOT = Path(__file__).resolve().parent.parent

# Build the static library if needed.
# Rust produces libsyntaqlite.a on Unix, syntaqlite.lib on Windows (MSVC).
if sys.platform == "win32":
    STATIC_LIB = ROOT / "target" / "release" / "syntaqlite.lib"
else:
    STATIC_LIB = ROOT / "target" / "release" / "libsyntaqlite.a"


def _ensure_static_lib():
    if STATIC_LIB.exists():
        return
    print(f"Building {STATIC_LIB.name} ...")
    subprocess.check_call(
        ["cargo", "build", "-p", "syntaqlite", "--release"],
        cwd=ROOT,
    )


_ensure_static_lib()

# Include paths for syntaqlite headers.
include_dirs = [
    str(ROOT / "syntaqlite-syntax" / "include"),
    str(ROOT / "syntaqlite" / "include"),
    str(ROOT / "python" / "csrc"),
]

ext = Extension(
    "syntaqlite._syntaqlite",
    sources=["csrc/_syntaqlite.c"],
    include_dirs=include_dirs,
    extra_objects=[str(STATIC_LIB)],
    language="c",
)

# On macOS, link system frameworks needed by Rust stdlib.
if sys.platform == "darwin":
    ext.extra_link_args = ["-framework", "Security", "-framework", "SystemConfiguration"]
# On Windows, link system libraries needed by Rust stdlib.
elif sys.platform == "win32":
    ext.libraries = ["ws2_32", "userenv", "advapi32", "bcrypt", "ntdll"]

setup(
    name="syntaqlite",
    ext_modules=[ext],
)
