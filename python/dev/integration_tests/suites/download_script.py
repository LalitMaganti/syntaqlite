# Copyright 2026 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Offline tests for tools/syntaqlite and the release manifest it reads."""

from __future__ import annotations

import importlib.machinery
import importlib.util
import io
import json
import tarfile
import tempfile
from pathlib import Path
from types import ModuleType

from python.dev.integration_tests.suite import SuiteContext
from python.tools.write_release_manifest import (
    ManifestError,
    build_manifest,
    downloads_table,
    with_downloads_table,
)

NAME = "download-script"
DESCRIPTION = "tools/syntaqlite download script and release manifest (offline)"
NEEDS_BINARY = False

_TARGET = ("x86_64-unknown-linux-gnu", "linux-x64")
_BINARY = b"#!/bin/sh\necho fake syntaqlite\n"


def _load_script(root_dir: Path) -> ModuleType:
    path = root_dir / "tools" / "syntaqlite"
    loader = importlib.machinery.SourceFileLoader("syntaqlite_download_script", str(path))
    spec = importlib.util.spec_from_loader(loader.name, loader)
    module = importlib.util.module_from_spec(spec)
    loader.exec_module(module)
    module._binary_name = lambda: "syntaqlite"
    return module


def _write_archive(path: Path, binary: bytes) -> None:
    buf = io.BytesIO()
    with tarfile.open(fileobj=buf, mode="w:gz") as tf:
        info = tarfile.TarInfo("syntaqlite")
        info.size = len(binary)
        info.mode = 0o755
        tf.addfile(info, io.BytesIO(binary))
    path.write_bytes(buf.getvalue())


def _release(tag: str, files: list[Path]) -> dict:
    return {
        "tag_name": tag,
        "assets": [{"name": f.name, "url": f.resolve().as_uri()} for f in files],
    }


def _write_part(parts: Path, name: str, target: str, archive: str) -> None:
    parts.mkdir(exist_ok=True)
    part = {"target": target, "name": archive, "binary": "syntaqlite"}
    (parts / f"{name}.json").write_text(json.dumps(part))


def _manifest_release(tmp: Path, archive_name: str) -> tuple[dict, Path]:
    cli_dir = tmp / "archives"
    cli_dir.mkdir()
    _write_archive(cli_dir / archive_name, _BINARY)
    _write_part(tmp / "parts", "linux-x64", _TARGET[0], archive_name)
    manifest = build_manifest("9.9.9", tmp / "parts", cli_dir, {})
    manifest_path = tmp / "syntaqlite-release.json"
    manifest_path.write_text(json.dumps(manifest))
    return _release("v9.9.9", [cli_dir / archive_name, manifest_path]), manifest_path


def _test_manifest_picks_renamed_archive(script: ModuleType, tmp: Path) -> None:
    release, _ = _manifest_release(tmp, "syntaqlite-9.9.9-x86_64-unknown-linux-gnu.tar.gz")
    dest = tmp / "out" / "syntaqlite"
    script._download(_TARGET, dest, release)
    assert dest.read_bytes() == _BINARY


def _test_checksum_mismatch_fails(script: ModuleType, tmp: Path) -> None:
    release, manifest_path = _manifest_release(tmp, "syntaqlite-linux-x64.tar.gz")
    manifest = json.loads(manifest_path.read_text())
    manifest["cli"][_TARGET[0]]["sha256"] = "0" * 64
    manifest_path.write_text(json.dumps(manifest))
    dest = tmp / "out" / "syntaqlite"
    try:
        script._download(_TARGET, dest, release)
    except script.DownloadError as e:
        assert "SHA-256" in str(e), e
    else:
        raise AssertionError("checksum mismatch was not detected")
    assert not dest.exists()


def _test_unsupported_manifest_version_fails(script: ModuleType, tmp: Path) -> None:
    release, manifest_path = _manifest_release(tmp, "syntaqlite-linux-x64.tar.gz")
    manifest = json.loads(manifest_path.read_text())
    manifest["manifest_version"] = 2
    manifest_path.write_text(json.dumps(manifest))
    try:
        script._download(_TARGET, tmp / "out" / "syntaqlite", release)
    except script.DownloadError as e:
        assert "manifest version 2" in str(e), e
    else:
        raise AssertionError("unsupported manifest version was accepted")


def _test_release_without_manifest_uses_legacy_name(script: ModuleType, tmp: Path) -> None:
    archive = tmp / "syntaqlite-linux-x64.tar.gz"
    _write_archive(archive, _BINARY)
    dest = tmp / "out" / "syntaqlite"
    script._download(_TARGET, dest, _release("v0.13.5", [archive]))
    assert dest.read_bytes() == _BINARY


def _test_manifest_rejects_archive_without_part(script: ModuleType, tmp: Path) -> None:
    cli_dir = tmp / "archives"
    cli_dir.mkdir()
    _write_archive(cli_dir / "syntaqlite-linux-x64.tar.gz", _BINARY)
    _write_archive(cli_dir / "syntaqlite-macos-arm64.tar.gz", _BINARY)
    _write_part(tmp / "parts", "linux-x64", _TARGET[0], "syntaqlite-linux-x64.tar.gz")
    try:
        build_manifest("9.9.9", tmp / "parts", cli_dir, {})
    except ManifestError as e:
        assert "syntaqlite-macos-arm64.tar.gz" in str(e), e
    else:
        raise AssertionError("archive without a part file was accepted")


def _test_downloads_table_lists_every_file(script: ModuleType, tmp: Path) -> None:
    lib = tmp / "libsyntaqlite.tar.gz"
    lib.write_bytes(b"lib")
    cli_dir = tmp / "archives"
    cli_dir.mkdir()
    _write_archive(cli_dir / "syntaqlite-linux-x64.tar.gz", _BINARY)
    _write_part(tmp / "parts", "linux-x64", _TARGET[0], "syntaqlite-linux-x64.tar.gz")
    manifest = build_manifest("9.9.9", tmp / "parts", cli_dir, {"libsyntaqlite": lib})
    table = downloads_table(manifest, "OWNER/REPO")
    base = "https://github.com/OWNER/REPO/releases/download/v9.9.9"
    cli = manifest["cli"][_TARGET[0]]
    lib_entry = manifest["files"]["libsyntaqlite"]
    assert (
        f"| [syntaqlite-linux-x64.tar.gz]({base}/syntaqlite-linux-x64.tar.gz) "
        f"| x64 Linux | `{cli['sha256']}` |"
    ) in table, table
    assert (
        f"| [libsyntaqlite.tar.gz]({base}/libsyntaqlite.tar.gz) "
        f"| C library, all platforms | `{lib_entry['sha256']}` |"
    ) in table, table
    noted = downloads_table(manifest, "OWNER/REPO", ["Old names are going away."])
    assert noted.endswith("\n\nOld names are going away.\n<!-- /downloads -->"), noted


def _test_downloads_table_replaces_previous_table(script: ModuleType, tmp: Path) -> None:
    old_table = "<!-- downloads -->\n### Downloads\nold\n<!-- /downloads -->"
    notes = with_downloads_table(f"**Formatter**\n- Fixed a bug.\n\n{old_table}\n", "NEW")
    assert notes == "**Formatter**\n- Fixed a bug.\n\nNEW\n", repr(notes)
    assert with_downloads_table("", "NEW") == "NEW\n"


_TESTS = [
    _test_manifest_picks_renamed_archive,
    _test_checksum_mismatch_fails,
    _test_unsupported_manifest_version_fails,
    _test_release_without_manifest_uses_legacy_name,
    _test_manifest_rejects_archive_without_part,
    _test_downloads_table_lists_every_file,
    _test_downloads_table_replaces_previous_table,
]


def run(ctx: SuiteContext) -> int:
    script = _load_script(ctx.root_dir)
    failures = 0
    for test in _TESTS:
        name = test.__name__.removeprefix("_test_")
        with tempfile.TemporaryDirectory(prefix="syntaqlite-download-test-") as tmp:
            try:
                test(script, Path(tmp))
            except Exception as e:
                failures += 1
                print(f"FAIL {name}: {type(e).__name__}: {e}")
                continue
        if ctx.verbose >= 0:
            print(f"PASS {name}")
    print(f"{len(_TESTS) - failures}/{len(_TESTS)} passed")
    return 1 if failures else 0
