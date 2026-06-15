# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Build the Pyodide (Emscripten) wheel for `syntaqlite`.

Cross-builds the `syntaqlite` crate as an Emscripten *side module* cdylib for
`wasm32-unknown-emscripten`, then packages it into a wheel tagged
`py3-none-pyemscripten_<abi>_wasm32`.

Two toolchains cooperate:
  - Rust: the **hermetic** third_party toolchain (`tools/cargo --hermetic`)
    plus the pinned wasm32 std from `tools/install-build-deps --ui`. The std
    is version-locked to that rustc, so we never use rustup.
  - emscripten: Pyodide's **patched** emcc, managed by `pyodide-build`. Stock
    emscripten rejects Rust std's `$`-mangled side-module exports; Pyodide's
    patches are required. We discover it via `pyodide config list`.

Prerequisites (the release CI installs these):
  tools/install-build-deps --ui          # hermetic rust + wasm32 std
  pip install pyodide-build build        # in the host Python (3.14)
  pyodide xbuildenv install 314.0.0
  pyodide xbuildenv install-emscripten
"""

from __future__ import annotations

import argparse
import os
import platform
import shutil
import subprocess
import sys

ROOT_DIR: str = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

TARGET = "wasm32-unknown-emscripten"


def platform_dir() -> str:
    machine = platform.machine().lower()
    arch = "arm64" if machine in ("arm64", "aarch64") else "amd64"
    sys_name = platform.system().lower()
    if sys_name == "darwin":
        return "mac-" + arch
    return "linux-" + arch


def pyodide_config() -> dict[str, str]:
    """Parse `pyodide config list` into a dict (requires pyodide-build)."""
    try:
        out = subprocess.check_output(["pyodide", "config", "list"], text=True)
    except (OSError, subprocess.CalledProcessError) as e:
        sys.exit(
            "error: `pyodide config list` failed — is pyodide-build installed "
            f"and an xbuildenv selected? ({e})"
        )
    cfg: dict[str, str] = {}
    for line in out.splitlines():
        if "=" in line:
            key, _, val = line.partition("=")
            cfg[key.strip()] = val.strip().strip('"')
    return cfg


def emsdk_env(emsdk_dir: str) -> dict[str, str]:
    """Return the environment after sourcing Pyodide emsdk's `emsdk_env.sh`."""
    script = os.path.join(emsdk_dir, "emsdk_env.sh")
    out = subprocess.check_output(
        ["bash", "-c", f'source "{script}" >/dev/null 2>&1 && env -0'],
    )
    env: dict[str, str] = {}
    for entry in out.split(b"\0"):
        if b"=" in entry:
            key, _, val = entry.partition(b"=")
            env[key.decode()] = val.decode()
    return env


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--outdir", default=os.path.join(ROOT_DIR, "wheelhouse-pyodide"),
        help="directory to write the built wheel into",
    )
    args = parser.parse_args()

    cfg = pyodide_config()
    emsdk_dir = cfg.get("emsdk_dir")
    abi = cfg.get("pyodide_abi_version")
    # Pyodide's blessed rust link flags (currently `-C link-arg=-sSIDE_MODULE=2`).
    pyodide_rustflags = cfg.get("rustflags", "-C link-arg=-sSIDE_MODULE=2")
    if not emsdk_dir or not abi:
        sys.exit("error: pyodide config missing emsdk_dir / pyodide_abi_version")

    wasm_sysroot = os.path.join(
        ROOT_DIR, "third_party", "bin", platform_dir(),
        "rust-wasm32", "rust-std-wasm32-unknown-emscripten",
    )
    if not os.path.isdir(wasm_sysroot):
        sys.exit(
            f"error: wasm32 std not found at {wasm_sysroot}\n"
            "run: tools/install-build-deps --ui"
        )

    env = emsdk_env(emsdk_dir)
    # WASM_BIGINT is required: the rpc C ABI passes 64-bit lengths (uint64_t).
    env[f"CARGO_TARGET_{TARGET.upper().replace('-', '_')}_RUSTFLAGS"] = (
        f"--sysroot {wasm_sysroot} {pyodide_rustflags} -C link-arg=-sWASM_BIGINT"
    )

    cargo = os.path.join(ROOT_DIR, "tools", "cargo")
    print("building cdylib side module for", TARGET)
    rc = subprocess.call(
        [
            sys.executable, cargo, "--no-sysroot", "--hermetic",
            "rustc", "-p", "syntaqlite", "--release", "--target", TARGET,
            "--no-default-features", "--features", "rpc,sqlite,fmt,analysis",
            "--crate-type", "cdylib",
        ],
        cwd=ROOT_DIR, env=env,
    )
    if rc != 0:
        return rc

    wasm = os.path.join(ROOT_DIR, "target", TARGET, "release", "syntaqlite.wasm")
    if not os.path.isfile(wasm):
        sys.exit(f"error: expected cdylib not found at {wasm}")

    # Package the wheel. setup.py copies `syntaqlite.wasm` -> `libsyntaqlite.so`
    # when CARGO_BUILD_TARGET is emscripten; _PYTHON_HOST_PLATFORM drives the
    # wheel's platform tag (py3-none-pyemscripten_<abi>_wasm32).
    build_env = os.environ.copy()
    build_env["CARGO_BUILD_TARGET"] = TARGET
    build_env["_PYTHON_HOST_PLATFORM"] = f"pyemscripten_{abi}_wasm32"
    print("packaging wheel (tag: py3-none-pyemscripten_%s_wasm32)" % abi)
    rc = subprocess.call(
        [sys.executable, "-m", "build", "--wheel", "--outdir", args.outdir,
         os.path.join(ROOT_DIR, "python")],
        cwd=ROOT_DIR, env=build_env,
    )
    if rc != 0:
        return rc

    wheels = [f for f in os.listdir(args.outdir) if f.endswith("_wasm32.whl")]
    for w in wheels:
        print("wrote", os.path.join(args.outdir, w))
    return 0


if __name__ == "__main__":
    sys.exit(main())
