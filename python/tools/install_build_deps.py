# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Installs build dependencies to third_party/bin/ and third_party/src/."""

import hashlib
import os
import platform
import shutil
import subprocess
import sys
import tarfile
import tempfile
import zipfile
from dataclasses import dataclass

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
THIRD_PARTY_DIR = os.path.join(ROOT_DIR, "third_party")
THIRD_PARTY_BIN_DIR = os.path.join(THIRD_PARTY_DIR, "bin")
THIRD_PARTY_SRC_DIR = os.path.join(THIRD_PARTY_DIR, "src")

GN_VERSION = "5550ba0f4053c3cbb0bff3d60ded9d867b6fa371"
NINJA_VERSION = "1.13.2"
SQLITE_VERSION = "3510200"  # 3.51.2
SQLITE_YEAR = "2026"
ZIG_VERSION = "0.14.0"


@dataclass
class BinaryDep:
    """Binary dependency (platform-specific)."""
    name: str
    version: str
    url: str
    sha256: str
    target_os: str  # darwin, linux, windows, or all
    target_arch: str  # x64, arm64, or all
    format: str = "zip"
    strip_prefix: str = ""  # Directory prefix to strip from archive


@dataclass
class SourceDep:
    """Source dependency (platform-independent)."""
    name: str
    version: str
    url: str
    checksum: str
    strip_prefix: str  # Directory prefix to strip from archive
    format: str = "zip"
    hash_type: str = "sha3_256"  # sha256 or sha3_256


# fmt: off
BINARY_DEPS = [
    BinaryDep("gn", GN_VERSION, f"https://chrome-infra-packages.appspot.com/dl/gn/gn/mac-amd64/+/git_revision:{GN_VERSION}", "68c9ad9456dd93090c39134781833ee7865d19627541cb9ba9003aeea9ce4e26", "darwin", "x64"),
    BinaryDep("gn", GN_VERSION, f"https://chrome-infra-packages.appspot.com/dl/gn/gn/mac-arm64/+/git_revision:{GN_VERSION}", "2e55c4f65ce690fef9c03af8abe2b76e01e0017fc040c2d7529c089abbe48309", "darwin", "arm64"),
    BinaryDep("gn", GN_VERSION, f"https://chrome-infra-packages.appspot.com/dl/gn/gn/linux-amd64/+/git_revision:{GN_VERSION}", "be32d9e5a79d52145baf22f83a5e4aa83724d6bdcdf53370fa00e5eba45596fa", "linux", "x64"),
    BinaryDep("gn", GN_VERSION, f"https://chrome-infra-packages.appspot.com/dl/gn/gn/linux-arm64/+/git_revision:{GN_VERSION}", "bbd8bab058398a005d240c09c17ce0af4fab69ae8d022a40ac7d0a218681de73", "linux", "arm64"),
    BinaryDep("gn", GN_VERSION, f"https://chrome-infra-packages.appspot.com/dl/gn/gn/windows-amd64/+/git_revision:{GN_VERSION}", "c10f4622abd995a1c070e46b0df8bbe7b83278b9ff2b05ae8245dabf7cb02b8c", "windows", "x64"),
    BinaryDep("ninja", NINJA_VERSION, f"https://github.com/ninja-build/ninja/releases/download/v{NINJA_VERSION}/ninja-mac.zip", "c99048673aa765960a99cf10c6ddb9f1fad506099ff0a0e137ad8960a88f321b", "darwin", "all"),
    BinaryDep("ninja", NINJA_VERSION, f"https://github.com/ninja-build/ninja/releases/download/v{NINJA_VERSION}/ninja-linux.zip", "5749cbc4e668273514150a80e387a957f933c6ed3f5f11e03fb30955e2bbead6", "linux", "x64"),
    BinaryDep("ninja", NINJA_VERSION, f"https://github.com/ninja-build/ninja/releases/download/v{NINJA_VERSION}/ninja-linux-aarch64.zip", "fd2cacc8050a7f12a16a2e48f9e06fca5c14fc4c2bee2babb67b58be17a607fc", "linux", "arm64"),
    BinaryDep("ninja", NINJA_VERSION, f"https://github.com/ninja-build/ninja/releases/download/v{NINJA_VERSION}/ninja-win.zip", "07fc8261b42b20e71d1720b39068c2e14ffcee6396b76fb7a795fb460b78dc65", "windows", "x64"),
    # clang-format: raw binaries from Chromium's cloud storage.
    # SHA1s from https://chromium.googlesource.com/chromium/src/buildtools/+/refs/heads/master/{mac,linux64,win}/
    BinaryDep("clang-format", "8503422f", "https://storage.googleapis.com/chromium-clang-format/8503422f469ae56cc74f0ea2c03f2d872f4a2303", "dabf93691361e8bd1d07466d67584072ece5c24e2b812c16458b8ff801c33e29", "darwin", "arm64", "raw"),
    BinaryDep("clang-format", "7d46d237", "https://storage.googleapis.com/chromium-clang-format/7d46d237f9664f41ef46b10c1392dcb559250f25", "0c3c13febeb0495ef0086509c24605ecae9e3d968ff9669d12514b8a55c7824e", "darwin", "x64", "raw"),
    BinaryDep("clang-format", "79a7b4e5", "https://storage.googleapis.com/chromium-clang-format/79a7b4e5336339c17b828de10d80611ff0f85961", "889266a51681d55bd4b9e02c9a104fa6ee22ecdfa7e8253532e5ea47e2e4cb4a", "linux", "x64", "raw"),
    BinaryDep("clang-format", "565cab9c", "https://storage.googleapis.com/chromium-clang-format/565cab9c66d61360c27c7d4df5defe1a78ab56d3", "5557943a174e3b67cdc389c10b0ceea2195f318c5c665dd77a427ed01a094557", "windows", "x64", "raw"),
    # Zig compiler: tar.xz on macOS/Linux, zip on Windows.
    BinaryDep("zig", ZIG_VERSION, f"https://ziglang.org/download/{ZIG_VERSION}/zig-macos-aarch64-{ZIG_VERSION}.tar.xz", "b71e4b7c4b4be9953657877f7f9e6f7ee89114c716da7c070f4a238220e95d7e", "darwin", "arm64", "tar.xz", f"zig-macos-aarch64-{ZIG_VERSION}"),
    BinaryDep("zig", ZIG_VERSION, f"https://ziglang.org/download/{ZIG_VERSION}/zig-macos-x86_64-{ZIG_VERSION}.tar.xz", "685816166f21f0b8d6fc7aa6a36e91396dcd82ca6556dfbe3e329deffc01fec3", "darwin", "x64", "tar.xz", f"zig-macos-x86_64-{ZIG_VERSION}"),
    BinaryDep("zig", ZIG_VERSION, f"https://ziglang.org/download/{ZIG_VERSION}/zig-linux-x86_64-{ZIG_VERSION}.tar.xz", "473ec26806133cf4d1918caf1a410f8403a13d979726a9045b421b685031a982", "linux", "x64", "tar.xz", f"zig-linux-x86_64-{ZIG_VERSION}"),
    BinaryDep("zig", ZIG_VERSION, f"https://ziglang.org/download/{ZIG_VERSION}/zig-linux-aarch64-{ZIG_VERSION}.tar.xz", "ab64e3ea277f6fc5f3d723dcd95d9ce1ab282c8ed0f431b4de880d30df891e4f", "linux", "arm64", "tar.xz", f"zig-linux-aarch64-{ZIG_VERSION}"),
    BinaryDep("zig", ZIG_VERSION, f"https://ziglang.org/download/{ZIG_VERSION}/zig-windows-x86_64-{ZIG_VERSION}.zip", "f53e5f9011ba20bbc3e0e6d0a9441b31eb227a97bac0e7d24172f1b8b27b4371", "windows", "x64", "zip", f"zig-windows-x86_64-{ZIG_VERSION}"),
]

GOOGLETEST_VERSION = "1.17.0"

SOURCE_DEPS = [
    SourceDep("sqlite", SQLITE_VERSION, f"https://sqlite.org/{SQLITE_YEAR}/sqlite-src-{SQLITE_VERSION}.zip", "e436bb919850445ce5168fb033d2d0d5c53a9d8c9602c7fa62b3e0025541d481", f"sqlite-src-{SQLITE_VERSION}"),
    SourceDep("googletest", GOOGLETEST_VERSION, f"https://github.com/google/googletest/releases/download/v{GOOGLETEST_VERSION}/googletest-{GOOGLETEST_VERSION}.tar.gz", "65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c", f"googletest-{GOOGLETEST_VERSION}", "tar.gz", "sha256"),
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


def sha3_256_file(path):
    h = hashlib.sha3_256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(8192), b""):
            h.update(chunk)
    return h.hexdigest()


def extract(archive_path, dest_dir, fmt):
    if fmt == "zip":
        with zipfile.ZipFile(archive_path) as zf:
            zf.extractall(dest_dir)
    elif fmt == "tar.gz":
        with tarfile.open(archive_path, "r:gz") as tf:
            tf.extractall(dest_dir)
    elif fmt == "tar.xz":
        with tarfile.open(archive_path, "r:xz") as tf:
            tf.extractall(dest_dir)
    else:
        sys.exit(f"Unsupported format: {fmt}")


def install_binary_dep(dep, target_dir):
    """Install a binary dependency. Returns True on success."""
    stamp_path = os.path.join(target_dir, f".{dep.name}.stamp")

    if os.path.exists(stamp_path):
        with open(stamp_path) as f:
            if f.read().strip() == dep.version:
                return True

    print(f"Downloading {dep.name}...")
    os.makedirs(target_dir, exist_ok=True)

    suffix = {"tar.gz": ".tar.gz", "zip": ".zip", "raw": ""}.get(dep.format, ".zip")
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

        if dep.format == "raw":
            exe_path = os.path.join(target_dir, dep.name)
            shutil.copy2(tmp_path, exe_path)
            os.chmod(exe_path, 0o755)
        elif dep.strip_prefix:
            # Directory-structured dep (e.g. Zig): extract, strip prefix, move
            # contents flat into target_dir.
            with tempfile.TemporaryDirectory() as extract_dir:
                extract(tmp_path, extract_dir, dep.format)
                src_dir = os.path.join(extract_dir, dep.strip_prefix)
                for item in os.listdir(src_dir):
                    dest = os.path.join(target_dir, item)
                    if os.path.isdir(dest):
                        shutil.rmtree(dest)
                    elif os.path.exists(dest):
                        os.unlink(dest)
                    shutil.move(os.path.join(src_dir, item), dest)
            exe_path = os.path.join(target_dir, dep.name)
            if os.path.exists(exe_path):
                os.chmod(exe_path, 0o755)
        else:
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


def install_source_dep(dep, target_dir):
    """Install a source dependency. Returns True on success."""
    dest_dir = os.path.join(target_dir, dep.name)
    stamp_path = os.path.join(target_dir, f".{dep.name}.stamp")

    if os.path.exists(stamp_path):
        with open(stamp_path) as f:
            if f.read().strip() == dep.version:
                return True

    print(f"Downloading {dep.name} source...")
    os.makedirs(target_dir, exist_ok=True)

    suffix = ".tar.gz" if dep.format == "tar.gz" else ".zip"
    with tempfile.NamedTemporaryFile(suffix=suffix, delete=False) as tmp:
        tmp_path = tmp.name

    try:
        result = subprocess.run(["curl", "-fsSL", "-o", tmp_path, dep.url], capture_output=True)
        if result.returncode != 0:
            print(f"Download failed: {result.stderr.decode()}", file=sys.stderr)
            return False

        if dep.hash_type == "sha256":
            actual_hash = sha256_file(tmp_path)
        else:
            actual_hash = sha3_256_file(tmp_path)
        if actual_hash != dep.checksum:
            print(f"Checksum mismatch for {dep.name}: expected {dep.checksum}, got {actual_hash}", file=sys.stderr)
            return False

        # Extract to temp dir first, then move stripped prefix to final location
        with tempfile.TemporaryDirectory() as extract_dir:
            extract(tmp_path, extract_dir, dep.format)

            # Remove existing destination if present
            if os.path.exists(dest_dir):
                shutil.rmtree(dest_dir)

            # Move the stripped prefix directory to final location
            src_path = os.path.join(extract_dir, dep.strip_prefix)
            shutil.move(src_path, dest_dir)

        with open(stamp_path, "w") as f:
            f.write(dep.version)

        print(f"Installed {dep.name} to {dest_dir}")
        return True
    finally:
        if os.path.exists(tmp_path):
            os.unlink(tmp_path)


def main():
    host_os, host_arch, platform_dir = get_platform()
    bin_target_dir = os.path.join(THIRD_PARTY_BIN_DIR, platform_dir)

    success = True

    # Install binary dependencies
    for dep in BINARY_DEPS:
        os_match = dep.target_os == "all" or dep.target_os == host_os
        arch_match = dep.target_arch == "all" or dep.target_arch == host_arch
        if os_match and arch_match:
            if not install_binary_dep(dep, bin_target_dir):
                success = False

    # Install source dependencies
    for dep in SOURCE_DEPS:
        if not install_source_dep(dep, THIRD_PARTY_SRC_DIR):
            success = False

    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
