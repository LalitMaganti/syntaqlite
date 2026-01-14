# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Updates the GN version and all platform SHA256 hashes in install_build_deps.py."""

import hashlib
import os
import re
import sys
import tempfile
import subprocess

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
INSTALL_DEPS_PATH = os.path.join(ROOT_DIR, "python", "tools", "install_build_deps.py")

# All GN platforms we need to update
GN_PLATFORMS = [
    ("mac-amd64", "darwin", "x64"),
    ("mac-arm64", "darwin", "arm64"),
    ("linux-amd64", "linux", "x64"),
    ("linux-arm64", "linux", "arm64"),
    ("windows-amd64", "windows", "x64"),
]


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(8192), b""):
            h.update(chunk)
    return h.hexdigest()


def download_and_hash(version, platform):
    """Download gn for a platform and return its SHA256 hash."""
    url = f"https://chrome-infra-packages.appspot.com/dl/gn/gn/{platform}/+/git_revision:{version}"

    with tempfile.NamedTemporaryFile(suffix=".zip", delete=False) as tmp:
        tmp_path = tmp.name

    try:
        print(f"  Downloading {platform}...")
        result = subprocess.run(
            ["curl", "-fsSL", "-o", tmp_path, url],
            capture_output=True
        )
        if result.returncode != 0:
            print(f"    Failed to download: {result.stderr.decode()}", file=sys.stderr)
            return None

        return sha256_file(tmp_path)
    finally:
        if os.path.exists(tmp_path):
            os.unlink(tmp_path)


def update_install_deps(version, hashes):
    """Update install_build_deps.py with new version and hashes."""
    with open(INSTALL_DEPS_PATH, "r") as f:
        content = f.read()

    # Update GN_VERSION
    content = re.sub(
        r'GN_VERSION = "[a-f0-9]+"',
        f'GN_VERSION = "{version}"',
        content
    )

    # Update each platform's hash
    for platform, target_os, target_arch in GN_PLATFORMS:
        if platform not in hashes:
            continue

        # Match the BinaryDep line for this platform
        # Pattern: BinaryDep("gn", GN_VERSION, f".../{platform}/+/...", "HASH", "os", "arch")
        pattern = (
            rf'(BinaryDep\("gn", GN_VERSION, f"[^"]*/{re.escape(platform)}/[^"]*", ")[a-f0-9]+(" *, *"{target_os}" *, *"{target_arch}"\))'
        )
        replacement = rf'\g<1>{hashes[platform]}\g<2>'
        content = re.sub(pattern, replacement, content)

    with open(INSTALL_DEPS_PATH, "w") as f:
        f.write(content)


def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <gn-git-revision>")
        print(f"Example: {sys.argv[0]} 5550ba0f4053c3cbb0bff3d60ded9d867b6fa371")
        sys.exit(1)

    version = sys.argv[1]

    # Validate version looks like a git hash
    if not re.match(r'^[a-f0-9]{40}$', version):
        print(f"Error: Version should be a 40-character git revision hash", file=sys.stderr)
        sys.exit(1)

    print(f"Updating GN to version {version}")
    print("Downloading and computing hashes for all platforms...")

    hashes = {}
    for platform, _, _ in GN_PLATFORMS:
        h = download_and_hash(version, platform)
        if h is None:
            print(f"Error: Failed to get hash for {platform}", file=sys.stderr)
            sys.exit(1)
        hashes[platform] = h
        print(f"    {platform}: {h}")

    print("\nUpdating install_build_deps.py...")
    update_install_deps(version, hashes)

    print("Done!")
    print("\nNext steps:")
    print("  1. Run: tools/dev/install-build-deps")
    print("  2. Test: tools/dev/gn gen out/debug")


if __name__ == "__main__":
    main()
