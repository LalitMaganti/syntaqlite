"""Package the `syntaqlite` Python distribution.

The Python package is a pure-Python client that speaks a JSON RPC protocol
to the bundled `syntaqlite` CLI binary (see `syntaqlite serve`).

Release CI runs `cargo build --release -p syntaqlite-cli` per platform, drops
the binary under `python/syntaqlite/bin/`, then invokes `python -m build`.
The wheel is tagged `py3-none-<plat>` so one wheel per OS/arch covers every
supported Py3 (see the `bdist_wheel` subclass below).
"""

import os
import shutil
import sys
from pathlib import Path

from setuptools import setup
from setuptools.command.bdist_wheel import bdist_wheel as _bdist_wheel

ROOT = Path(__file__).resolve().parent.parent
PKG_DIR = Path(__file__).resolve().parent / "syntaqlite"

# ── CLI binary bundling ─────────────────────────────────────────────────────

_binary_name = "syntaqlite.exe" if sys.platform == "win32" else "syntaqlite"
_cargo_target = os.environ.get("CARGO_BUILD_TARGET")
if _cargo_target:
    _binary_src = ROOT / "target" / _cargo_target / "release" / _binary_name
else:
    _binary_src = ROOT / "target" / "release" / _binary_name
if _binary_src.exists():
    _bin_dest = PKG_DIR / "bin"
    _bin_dest.mkdir(exist_ok=True)
    shutil.copy2(_binary_src, _bin_dest / _binary_name)

# ── package_data ────────────────────────────────────────────────────────────

package_data = {}
_bin_dir = PKG_DIR / "bin"
if _bin_dir.exists() and any(_bin_dir.iterdir()):
    package_data["syntaqlite"] = ["bin/*"]


class bdist_wheel(_bdist_wheel):
    def finalize_options(self):
        super().finalize_options()
        self.root_is_pure = False

    def get_tag(self):
        _, _, plat = super().get_tag()
        return "py3", "none", plat


setup(
    name="syntaqlite",
    cmdclass={"bdist_wheel": bdist_wheel},
    package_data=package_data,
)
