# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Installs build dependencies (gn, ninja) to third_party/<platform>/."""

import hashlib
import os
import platform
import subprocess
import sys
import tarfile
import tempfile
import zipfile
from dataclasses import dataclass

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
THIRD_PARTY_DIR = os.path.join(ROOT_DIR, "third_party")

GN_VERSION = "9673115bc14c8630da5b7f6fe07e0b362ac49dcb"
NINJA_VERSION = "1.13.2"


@dataclass
class Dep:
    name: str
    version: str
    url: str
    sha256: str
    target_os: str  # darwin, linux, windows, or all
    target_arch: str  # x64, arm64, or all
    format: str = "zip"  # zip or tar.gz


# fmt: off
DEPS = [
    Dep("gn", GN_VERSION, f"https://chrome-infra-packages.appspot.com/dl/gn/gn/mac-amd64/+/git_revision:{GN_VERSION}", "388d837b6f0d7e93ffed4f5458533a1833ec9a6e87615349ef2fc18c6849a058", "darwin", "x64"),
    Dep("gn", GN_VERSION, f"https://chrome-infra-packages.appspot.com/dl/gn/gn/mac-arm64/+/git_revision:{GN_VERSION}", "f76fed9e3d0265da47c30b174f928317521f644bb0cb99269862e2eb8d7d152d", "darwin", "arm64"),
    Dep("gn", GN_VERSION, f"https://chrome-infra-packages.appspot.com/dl/gn/gn/linux-amd64/+/git_revision:{GN_VERSION}", "5fb33009129ba68f68b9f86a1a8679a192d7786e8a5751453afbef2f984d98a9", "linux", "x64"),
    Dep("gn", GN_VERSION, f"https://chrome-infra-packages.appspot.com/dl/gn/gn/linux-arm64/+/git_revision:{GN_VERSION}", "92af30c58394742fb11d6852db486bcf9b0e2edbf59403f9c111ce8a6f117847", "linux", "arm64"),
    Dep("gn", GN_VERSION, f"https://chrome-infra-packages.appspot.com/dl/gn/gn/windows-amd64/+/git_revision:{GN_VERSION}", "64cd63417cc1813dccd4e06f0145e62dc0236a696c183e587ebaaeb1cbfc50b2", "windows", "x64"),
    Dep("ninja", NINJA_VERSION, f"https://github.com/ninja-build/ninja/releases/download/v{NINJA_VERSION}/ninja-mac.zip", "c99048673aa765960a99cf10c6ddb9f1fad506099ff0a0e137ad8960a88f321b", "darwin", "all"),
    Dep("ninja", NINJA_VERSION, f"https://github.com/ninja-build/ninja/releases/download/v{NINJA_VERSION}/ninja-linux.zip", "5749cbc4e668273514150a80e387a957f933c6ed3f5f11e03fb30955e2bbead6", "linux", "x64"),
    Dep("ninja", NINJA_VERSION, f"https://github.com/ninja-build/ninja/releases/download/v{NINJA_VERSION}/ninja-linux-aarch64.zip", "fd2cacc8050a7f12a16a2e48f9e06fca5c14fc4c2bee2babb67b58be17a607fc", "linux", "arm64"),
    Dep("ninja", NINJA_VERSION, f"https://github.com/ninja-build/ninja/releases/download/v{NINJA_VERSION}/ninja-win.zip", "07fc8261b42b20e71d1720b39068c2e14ffcee6396b76fb7a795fb460b78dc65", "windows", "x64"),
]
# fmt: on


def get_platform():
    """Returns (os, arch, platform_dir)."""
    sys_name = platform.system().lower()
    machine = platform.machine().lower()

    if sys_name == "darwin":
        host_os, prefix = "darwin", "mac"
    elif sys_name == "linux":
        host_os, prefix = "linux", "linux"
    elif sys_name == "windows":
        host_os, prefix = "windows", "win"
    else:
        sys.exit(f"Unsupported OS: {sys_name}")

    host_arch = "arm64" if machine in ("arm64", "aarch64") else "x64"
    platform_dir = f"{prefix}-{'arm64' if host_arch == 'arm64' else 'amd64'}"

    return host_os, host_arch, platform_dir


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(8192), b""):
            h.update(chunk)
    return h.hexdigest()


def extract(archive_path, dest_dir, format):
    if format == "zip":
        with zipfile.ZipFile(archive_path) as zf:
            zf.extractall(dest_dir)
    elif format == "tar.gz":
        with tarfile.open(archive_path, "r:gz") as tf:
            tf.extractall(dest_dir)
    else:
        sys.exit(f"Unsupported format: {format}")


def install_dep(dep, target_dir):
    """Install a dependency. Returns True on success."""
    stamp_path = os.path.join(target_dir, f".{dep.name}.stamp")

    if os.path.exists(stamp_path):
        with open(stamp_path) as f:
            if f.read().strip() == dep.version:
                return True

    print(f"Downloading {dep.name}...")
    os.makedirs(target_dir, exist_ok=True)

    suffix = ".tar.gz" if dep.format == "tar.gz" else ".zip"
    with tempfile.NamedTemporaryFile(suffix=suffix, delete=False) as tmp:
        tmp_path = tmp.name

    try:
        result = subprocess.run(["curl", "-fsSL", "-o", tmp_path, dep.url], capture_output=True)
        if result.returncode != 0:
            print(f"Download failed: {result.stderr.decode()}", file=sys.stderr)
            return False

        actual_sha256 = sha256_file(tmp_path)
        if actual_sha256 != dep.sha256:
            print(f"SHA256 mismatch for {dep.name}: expected {dep.sha256}, got {actual_sha256}", file=sys.stderr)
            return False

        extract(tmp_path, target_dir, dep.format)

        exe_path = os.path.join(target_dir, dep.name)
        if os.path.exists(exe_path):
            os.chmod(exe_path, 0o755)

        with open(stamp_path, "w") as f:
            f.write(dep.version)

        return True
    finally:
        if os.path.exists(tmp_path):
            os.unlink(tmp_path)


def main():
    host_os, host_arch, platform_dir = get_platform()
    target_dir = os.path.join(THIRD_PARTY_DIR, platform_dir)

    success = True
    for dep in DEPS:
        os_match = dep.target_os == "all" or dep.target_os == host_os
        arch_match = dep.target_arch == "all" or dep.target_arch == host_arch
        if os_match and arch_match:
            if not install_dep(dep, target_dir):
                success = False

    return 0 if success else 1
