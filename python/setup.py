"""Package the `syntaqlite` Python distribution.

The Python package drives the syntaqlite core in-process over a C ABI via
ctypes, talking to the bundled cdylib under `python/syntaqlite/lib/`. The
`syntaqlite` CLI binary is also bundled (under `bin/`) for the console script.

Release CI builds the cdylib (and CLI) per platform, then invokes
`python -m build`. The wheel is tagged `py3-none-<plat>` so one wheel per
OS/arch covers every supported Py3 (see the `bdist_wheel` subclass below).

For Pyodide, the cdylib is cross-built for `wasm32-unknown-emscripten` as an
Emscripten side module and the wheel is tagged `py3-none-pyemscripten_*` —
see `tools/build-pyodide-wheel`.
"""

import os
import shutil
import sys
from pathlib import Path

from setuptools import Distribution, setup
from setuptools.command.bdist_wheel import bdist_wheel as _bdist_wheel

ROOT = Path(__file__).resolve().parent.parent
PKG_DIR = Path(__file__).resolve().parent / "syntaqlite"

# ── CLI binary bundling ─────────────────────────────────────────────────────

_binary_name = "syntaqlite.exe" if sys.platform == "win32" else "syntaqlite"
_cargo_target = os.environ.get("CARGO_BUILD_TARGET")
if _cargo_target:
    _release_dir = ROOT / "target" / _cargo_target / "release"
else:
    _release_dir = ROOT / "target" / "release"
_binary_src = _release_dir / _binary_name
if _binary_src.exists():
    _bin_dest = PKG_DIR / "bin"
    _bin_dest.mkdir(exist_ok=True)
    shutil.copy2(_binary_src, _bin_dest / _binary_name)

# ── cdylib bundling (in-process transport) ──────────────────────────────────

# When cross-building for Emscripten (Pyodide), the cdylib is emitted as
# `syntaqlite.wasm` but must ship as `libsyntaqlite.so` — the name ctypes
# loads when `sys.platform == "emscripten"`.
if _cargo_target and "emscripten" in _cargo_target:
    _ffi_src_name, _ffi_dest_name = "syntaqlite.wasm", "libsyntaqlite.so"
elif sys.platform == "win32":
    _ffi_src_name = _ffi_dest_name = "syntaqlite.dll"
elif sys.platform == "darwin":
    _ffi_src_name = _ffi_dest_name = "libsyntaqlite.dylib"
else:
    _ffi_src_name = _ffi_dest_name = "libsyntaqlite.so"
_ffi_src = _release_dir / _ffi_src_name
if _ffi_src.exists():
    _lib_dest = PKG_DIR / "lib"
    _lib_dest.mkdir(exist_ok=True)
    shutil.copy2(_ffi_src, _lib_dest / _ffi_dest_name)

# ── package_data ────────────────────────────────────────────────────────────

package_data = {}
_data_globs = []
_bin_dir = PKG_DIR / "bin"
if _bin_dir.exists() and any(_bin_dir.iterdir()):
    _data_globs.append("bin/*")
_lib_dir = PKG_DIR / "lib"
if _lib_dir.exists() and any(_lib_dir.iterdir()):
    _data_globs.append("lib/*")
if _data_globs:
    package_data["syntaqlite"] = _data_globs


class BinaryDistribution(Distribution):
    # Forces a platlib wheel (Root-Is-Purelib: false) so auditwheel is willing
    # to inspect the bundled CLI binary under syntaqlite/bin/.
    def has_ext_modules(self):
        return True


class bdist_wheel(_bdist_wheel):
    def get_tag(self):
        _, _, plat = super().get_tag()
        return "py3", "none", plat


setup(
    name="syntaqlite",
    distclass=BinaryDistribution,
    cmdclass={"bdist_wheel": bdist_wheel},
    package_data=package_data,
)
