# Copyright 2026 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Write syntaqlite-release.json, the manifest describing a release's files.

Clients such as tools/syntaqlite read the manifest instead of hard-coding
release file names, so the files can be renamed without breaking them.

Each CLI build job records a part file naming its Rust target, archive, and
binary. This tool merges those parts with the archives' SHA-256 digests:

    {
      "manifest_version": 1,
      "version": "0.13.6",
      "cli": {
        "x86_64-unknown-linux-gnu": {
          "name": "syntaqlite-linux-x64.tar.gz",
          "sha256": "...",
          "binary": "syntaqlite"
        }
      },
      "files": {
        "libsyntaqlite": {"name": "libsyntaqlite.tar.gz", "sha256": "..."}
      }
    }

Bump MANIFEST_VERSION only for changes that existing clients cannot read.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path

MANIFEST_NAME = "syntaqlite-release.json"
MANIFEST_VERSION = 1


class ManifestError(RuntimeError):
    """Inputs that cannot produce a complete manifest."""


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def build_manifest(
    version: str, parts_dir: Path, cli_dir: Path, files: dict[str, Path]
) -> dict:
    cli: dict[str, dict] = {}
    for part_path in sorted(parts_dir.glob("*.json")):
        part = json.loads(part_path.read_text())
        target, name = part["target"], part["name"]
        if target in cli:
            raise ManifestError(f"duplicate CLI target {target} in {part_path}")
        archive = cli_dir / name
        if not archive.is_file():
            raise ManifestError(f"{part_path} names {name}, missing from {cli_dir}")
        cli[target] = {"name": name, "sha256": _sha256(archive), "binary": part["binary"]}
    if not cli:
        raise ManifestError(f"no CLI part files in {parts_dir}")

    listed = {entry["name"] for entry in cli.values()}
    unlisted = sorted(p.name for p in cli_dir.iterdir() if p.is_file() and p.name not in listed)
    if unlisted:
        raise ManifestError(f"CLI archives without a part file: {', '.join(unlisted)}")

    manifest_files = {}
    for kind, path in sorted(files.items()):
        if not path.is_file():
            raise ManifestError(f"{kind} file {path} does not exist")
        manifest_files[kind] = {"name": path.name, "sha256": _sha256(path)}

    return {
        "manifest_version": MANIFEST_VERSION,
        "version": version,
        "cli": dict(sorted(cli.items())),
        "files": manifest_files,
    }


def _parse_file(value: str) -> tuple[str, Path]:
    kind, sep, path = value.partition("=")
    if not sep or not kind or not path:
        raise argparse.ArgumentTypeError(f"expected KIND=PATH, got {value!r}")
    return kind, Path(path)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--version", required=True, help="Release version, without the leading v.")
    parser.add_argument("--parts", type=Path, required=True, help="Directory of CLI part files.")
    parser.add_argument("--cli-dir", type=Path, required=True, help="Directory of CLI archives.")
    parser.add_argument(
        "--file", type=_parse_file, action="append", default=[], metavar="KIND=PATH",
        help="Another release file to list under `files` (repeatable).",
    )
    parser.add_argument("--output", type=Path, default=Path(MANIFEST_NAME))
    args = parser.parse_args(argv)

    try:
        manifest = build_manifest(args.version, args.parts, args.cli_dir, dict(args.file))
    except ManifestError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1
    args.output.write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"wrote {args.output} ({len(manifest['cli'])} CLI targets)")
    return 0
