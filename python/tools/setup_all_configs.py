# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Sets up build configurations for all supported platforms."""

import argparse
import os
import platform
import subprocess
import sys

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def _brew_llvm_args():
    """Return GN args to use Homebrew LLVM, or empty string if unavailable."""
    try:
        prefix = subprocess.check_output(
            ["brew", "--prefix", "llvm"], text=True, stderr=subprocess.DEVNULL
        ).strip()
        cc = os.path.join(prefix, "bin", "clang")
        cxx = os.path.join(prefix, "bin", "clang++")
        if os.path.isfile(cc):
            return f' cc="{cc}" cxx="{cxx}"'
    except (subprocess.CalledProcessError, FileNotFoundError):
        pass
    return ""


# Build configurations: name -> gn args
MAC_BUILD_CONFIGS = {
    "mac_debug": "is_debug=true",
    "mac_release": "is_debug=false",
    "mac_asan": "is_debug=false is_asan=true" + _brew_llvm_args(),
}

LINUX_BUILD_CONFIGS = {
    "linux_debug": "is_debug=true",
    "linux_release": "is_debug=false",
    "linux_asan": "is_debug=false is_asan=true",
    "linux_tsan": "is_debug=false is_tsan=true",
    "linux_msan": "is_debug=false is_msan=true",
}

WINDOWS_BUILD_CONFIGS = {
    "win_debug": "is_debug=true",
    "win_release": "is_debug=false",
}


def get_gn_path():
    return os.path.join(ROOT_DIR, "tools", "dev", "gn")


def run_gn_gen(out_dir, gn_args):
    gn = get_gn_path()
    cmd = [gn, "gen", out_dir, "--args=" + gn_args]
    print(f"Running: {' '.join(cmd)}")
    return subprocess.call(cmd)


def main():
    parser = argparse.ArgumentParser(description="Setup build configurations")
    parser.add_argument("--host-only", action="store_true",
                        help="Only generate configs for the host platform")
    parser.add_argument("--config", type=str,
                        help="Only generate a specific config")
    parser.add_argument("--ccache", action="store_true",
                        help="Enable ccache")
    args = parser.parse_args()

    host_os = platform.system().lower()

    if host_os == "darwin":
        configs = MAC_BUILD_CONFIGS
    elif host_os == "linux":
        configs = LINUX_BUILD_CONFIGS
    elif host_os == "windows":
        configs = WINDOWS_BUILD_CONFIGS
    else:
        print(f"Unsupported OS: {host_os}", file=sys.stderr)
        return 1

    if args.config:
        if args.config not in configs:
            print(f"Unknown config: {args.config}", file=sys.stderr)
            print(f"Available: {', '.join(configs.keys())}", file=sys.stderr)
            return 1
        configs = {args.config: configs[args.config]}

    for name, gn_args in configs.items():
        out_dir = os.path.join(ROOT_DIR, "out", name)

        if args.ccache:
            gn_args += " cc_wrapper=\"ccache\""

        if run_gn_gen(out_dir, gn_args) != 0:
            print(f"Failed to configure {name}", file=sys.stderr)
            return 1

    print(f"\nConfigured {len(configs)} build(s) in out/")
    return 0
