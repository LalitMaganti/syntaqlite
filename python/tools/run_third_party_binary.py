# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""A wrapper to run gn, ninja and other third_party/ binaries for all platforms."""

import os
import platform
import subprocess
import sys

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def get_platform_dir():
    """Returns the platform-specific buildtools subdirectory name."""
    sys_name = platform.system().lower()
    machine = platform.machine().lower()
    arch = "arm64" if machine in ("arm64", "aarch64") else "amd64"

    if sys_name == "darwin":
        return "mac-" + arch, ""
    elif sys_name == "linux":
        return "linux-" + arch, ""
    elif sys_name == "windows":
        return "win-" + arch, ".exe"
    else:
        return None, ""


def run_third_party_binary(args, cwd=None):
    if len(args) < 1:
        print("Usage: %s command [args]" % sys.argv[0])
        return 1

    os_dir, ext = get_platform_dir()
    if os_dir is None:
        print("OS not supported: %s" % platform.system())
        return 1

    cmd = args[0]
    args = args[1:]

    exe_path = os.path.join(ROOT_DIR, "third_party", "bin", os_dir, cmd) + ext

    if not os.path.exists(exe_path):
        print("Binary not found: %s" % exe_path)
        print("Run tools/dev/install-build-deps to install build dependencies.")
        return 1

    if cwd or platform.system().lower() == "windows":
        # execl() doesn't support cwd and behaves oddly on Windows: the spawned
        # process doesn't seem to receive CTRL+C. Use subprocess instead.
        sys.exit(subprocess.call([exe_path] + args, cwd=cwd))
    else:
        os.execl(exe_path, os.path.basename(exe_path), *args)
