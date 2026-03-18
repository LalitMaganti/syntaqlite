"""Build the _syntaqlite C extension.

Usage (local development):
    cd python && pip install -e .
    # or
    cd python && python setup.py build_ext --inplace

In CI, cibuildwheel invokes this via the PEP 517 build backend. The Rust
static library and CLI binary are pre-built by cibuildwheel's before-all hook.
"""

import os
import shutil
import subprocess
import sys
from pathlib import Path

from setuptools import Extension, setup

ROOT = Path(__file__).resolve().parent.parent
PKG_DIR = Path(__file__).resolve().parent / "syntaqlite"

# ---------------------------------------------------------------------------
# CLI binary bundling
# ---------------------------------------------------------------------------
# Copy the CLI binary into syntaqlite/bin/ so setuptools includes it via
# package_data. Skipped when the binary hasn't been built (e.g. local
# `pip install -e .` without building the CLI).

_binary_name = "syntaqlite.exe" if sys.platform == "win32" else "syntaqlite"
# When cross-compiling (e.g. ARM64 on AMD64 Windows), the binary lives under
# target/<triple>/release/ instead of target/release/.
_cargo_target = os.environ.get("CARGO_BUILD_TARGET")
if _cargo_target:
    _binary_src = ROOT / "target" / _cargo_target / "release" / _binary_name
else:
    _binary_src = ROOT / "target" / "release" / _binary_name
if _binary_src.exists():
    _bin_dest = PKG_DIR / "bin"
    _bin_dest.mkdir(exist_ok=True)
    shutil.copy2(_binary_src, _bin_dest / _binary_name)

# ---------------------------------------------------------------------------
# Static library for C extension
# ---------------------------------------------------------------------------

def _rust_target_triple() -> str | None:
    """Return the Rust target triple matching the current Python platform, or None for native."""
    # cibuildwheel sets CARGO_BUILD_TARGET when cross-compiling (e.g. ARM64 on
    # an AMD64 host).  This is the most reliable signal because
    # sysconfig.get_platform() still reports the *host* platform inside
    # cibuildwheel's build environment.
    env_target = os.environ.get("CARGO_BUILD_TARGET")
    if env_target:
        return env_target

    import sysconfig
    plat = sysconfig.get_platform()  # e.g. 'win-arm64', 'win-amd64'
    if plat == "win-arm64":
        return "aarch64-pc-windows-msvc"
    return None


def _find_static_lib() -> Path:
    """Find the Rust static library, checking cross-compiled target dirs."""
    override = os.environ.get("SYNTAQLITE_STATIC_LIB")
    if override:
        return Path(override)

    if sys.platform == "win32":
        lib_name = "syntaqlite.lib"
    else:
        lib_name = "libsyntaqlite.a"

    # For cross-compiled targets (e.g. ARM64 on x86_64 Windows), prefer the
    # target-specific directory over the native one.
    triple = _rust_target_triple()
    if triple:
        cross = ROOT / "target" / triple / "release" / lib_name
        if cross.exists():
            return cross

    # Check default (native) location.
    default = ROOT / "target" / "release" / lib_name
    if default.exists():
        return default

    # Check cross-compiled target directories as a fallback.
    target_dir = ROOT / "target"
    if target_dir.exists():
        for candidate in sorted(target_dir.iterdir()):
            lib = candidate / "release" / lib_name
            if lib.exists():
                return lib

    return default  # Fall back to default (will trigger build or error).


STATIC_LIB = _find_static_lib()


def _ensure_static_lib():
    if STATIC_LIB.exists():
        return
    if os.environ.get("SYNTAQLITE_STATIC_LIB"):
        sys.exit(f"SYNTAQLITE_STATIC_LIB set but not found: {STATIC_LIB}")
    print(f"Building {STATIC_LIB.name} ...")
    subprocess.check_call(
        ["cargo", "build", "-p", "syntaqlite", "--release"],
        cwd=ROOT,
    )


# ---------------------------------------------------------------------------
# Extension module
# ---------------------------------------------------------------------------

ext_modules = []

_cli_only = os.environ.get("SYNTAQLITE_CLI_ONLY", "")
if not _cli_only:
    _ensure_static_lib()

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

    ext_modules.append(ext)

# ---------------------------------------------------------------------------
# package_data: include CLI binary if present
# ---------------------------------------------------------------------------

package_data = {}
_bin_dir = PKG_DIR / "bin"
if _bin_dir.exists() and any(_bin_dir.iterdir()):
    package_data["syntaqlite"] = ["bin/*"]

setup(
    name="syntaqlite",
    ext_modules=ext_modules,
    package_data=package_data,
)
