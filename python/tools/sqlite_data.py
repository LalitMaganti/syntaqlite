#!/usr/bin/env python3
# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Download SQLite sources/amalgamations and extract data catalogs.

Subcommands:
  download-sources       Download SQLite source files from the GitHub mirror.
  download-amalgamations Download SQLite amalgamation archives from sqlite.org.
  update-data            Download amalgamations, audit cflags, and extract the
                         built-in catalogs in one shot. This is the main command
                         for keeping sqlite-vendored/data/ up to date.

Usage:
    tools/sqlite-data download-sources
    tools/sqlite-data download-amalgamations
    tools/sqlite-data update-data
    tools/sqlite-data update-data --amalgamation-dir /path/to/amalgamations
"""

from __future__ import annotations

import argparse
import io
import subprocess
import sys
import urllib.request
import zipfile
from pathlib import Path

PROJECT_ROOT: Path = Path(__file__).parent.parent.parent

# ---------------------------------------------------------------------------
# Version data
# ---------------------------------------------------------------------------

# Versions for source file downloads (version analysis, covers 3.12.2+).
SOURCE_VERSIONS: list[str] = [
    "3.12.2", "3.13.0", "3.14.2", "3.15.2", "3.16.2", "3.17.0", "3.18.2",
    "3.19.4", "3.20.1", "3.21.0", "3.22.0", "3.23.2", "3.24.0", "3.25.3",
    "3.26.0", "3.27.2", "3.28.0", "3.29.0", "3.30.1", "3.31.1", "3.32.3",
    "3.33.0", "3.34.1", "3.35.5", "3.36.0", "3.37.2", "3.38.5", "3.39.4",
    "3.40.1", "3.41.2", "3.42.0", "3.43.2", "3.44.2", "3.45.3", "3.46.1",
    "3.47.2", "3.48.0", "3.49.2", "3.50.4", "3.51.2",
]

# Source files to download per version (from GitHub mirror).
SOURCE_FILES: list[str] = [
    "src/tokenize.c",
    "src/global.c",
    "src/sqliteInt.h",
    "src/parse.y",
    "tool/mkkeywordhash.c",
]

# Probe every supported version. Versions before PRAGMA function_list was
# introduced simply yield no function rows, while the same probe still extracts
# their built-in relations and cflag behavior.
AMALGAMATION_VERSIONS: list[str] = [
    "3.12.2", "3.13.0", "3.14.2", "3.15.2", "3.16.2", "3.17.0", "3.18.0",
    "3.19.3", "3.20.1", "3.21.0", "3.22.0", "3.23.1", "3.24.0", "3.25.3",
    "3.26.0", "3.27.2", "3.28.0", "3.29.0", "3.30.1", "3.31.1", "3.32.3",
    "3.33.0", "3.34.1", "3.35.5", "3.36.0", "3.37.2", "3.38.5", "3.39.4",
    "3.40.1", "3.41.2", "3.42.0", "3.43.2", "3.44.2", "3.45.3", "3.46.1",
    "3.47.2", "3.48.0", "3.49.2", "3.50.4", "3.51.2",
]

# Version → release year mapping (from sqlite.org/changes.html).
# Needed to construct the amalgamation download URL.
VERSION_YEAR: dict[str, int] = {
    "3.12.2": 2016, "3.13.0": 2016, "3.14.2": 2016, "3.15.2": 2016,
    "3.16.2": 2017, "3.17.0": 2017, "3.18.0": 2017, "3.19.3": 2017,
    "3.20.1": 2017, "3.21.0": 2017, "3.22.0": 2018, "3.23.1": 2018,
    "3.24.0": 2018, "3.25.3": 2018, "3.26.0": 2018, "3.27.2": 2019,
    "3.28.0": 2019, "3.29.0": 2019,
    "3.30.0": 2019, "3.30.1": 2019,
    "3.31.0": 2020, "3.31.1": 2020,
    "3.32.0": 2020, "3.32.1": 2020, "3.32.2": 2020, "3.32.3": 2020,
    "3.33.0": 2020,
    "3.34.0": 2020, "3.34.1": 2021,
    "3.35.0": 2021, "3.35.1": 2021, "3.35.2": 2021, "3.35.3": 2021,
    "3.35.4": 2021, "3.35.5": 2021,
    "3.36.0": 2021,
    "3.37.0": 2021, "3.37.1": 2021, "3.37.2": 2022,
    "3.38.0": 2022, "3.38.1": 2022, "3.38.2": 2022, "3.38.3": 2022,
    "3.38.4": 2022, "3.38.5": 2022,
    "3.39.0": 2022, "3.39.1": 2022, "3.39.2": 2022, "3.39.3": 2022,
    "3.39.4": 2022,
    "3.40.0": 2022, "3.40.1": 2022,
    "3.41.0": 2023, "3.41.1": 2023, "3.41.2": 2023,
    "3.42.0": 2023,
    "3.43.0": 2023, "3.43.1": 2023, "3.43.2": 2023,
    "3.44.0": 2023, "3.44.1": 2023, "3.44.2": 2023,
    "3.45.0": 2024, "3.45.1": 2024, "3.45.2": 2024, "3.45.3": 2024,
    "3.46.0": 2024, "3.46.1": 2024,
    "3.47.0": 2024, "3.47.1": 2024, "3.47.2": 2024,
    "3.48.0": 2025,
    "3.49.0": 2025, "3.49.1": 2025, "3.49.2": 2025,
    "3.50.0": 2025, "3.50.1": 2025, "3.50.2": 2025, "3.50.3": 2025,
    "3.50.4": 2025,
    "3.51.0": 2025, "3.51.1": 2025, "3.51.2": 2026,
}


def version_int(ver: str) -> int:
    """Convert version string to SQLite version integer (3.X.Y -> 3XXYY00)."""
    parts = ver.split(".")
    major, minor, patch = int(parts[0]), int(parts[1]), int(parts[2])
    sub = int(parts[3]) if len(parts) > 3 else 0
    return major * 1_000_000 + minor * 10_000 + patch * 100 + sub


# ---------------------------------------------------------------------------
# download-sources
# ---------------------------------------------------------------------------

def cmd_download_sources(args: argparse.Namespace) -> int:
    """Download SQLite source files from the GitHub mirror."""
    versions = args.versions or SOURCE_VERSIONS
    output_dir = Path(args.output_dir)
    base_url = "https://raw.githubusercontent.com/sqlite/sqlite"

    print(f"Downloading {len(versions)} SQLite source versions to {output_dir}/")

    failed = 0
    for ver in versions:
        tag = f"version-{ver}"
        ver_dir = output_dir / ver
        (ver_dir / "src").mkdir(parents=True, exist_ok=True)
        (ver_dir / "tool").mkdir(parents=True, exist_ok=True)

        all_ok = True
        for f in SOURCE_FILES:
            dest = ver_dir / f
            if dest.exists():
                continue
            url = f"{base_url}/{tag}/{f}"
            try:
                urllib.request.urlretrieve(url, dest)
            except Exception as e:
                print(f"  FAILED: {ver}/{f} ({e})", file=sys.stderr)
                dest.unlink(missing_ok=True)
                all_ok = False
                failed += 1

        if all_ok:
            print(f"  {ver}: ok")

    if failed > 0:
        print(f"{failed} file(s) failed to download", file=sys.stderr)
        return 1

    print(f"Done. Sources in {output_dir}/")
    return 0


# ---------------------------------------------------------------------------
# download-amalgamations
# ---------------------------------------------------------------------------

def cmd_download_amalgamations(args: argparse.Namespace) -> int:
    """Download SQLite amalgamation archives from sqlite.org."""
    versions = args.versions or AMALGAMATION_VERSIONS
    output_dir = Path(args.output_dir)
    base_url = "https://www.sqlite.org"

    print(f"Downloading {len(versions)} SQLite amalgamations to {output_dir}/")

    failed = 0
    skipped = 0
    for ver in versions:
        ver_dir = output_dir / ver

        # Skip if already downloaded.
        if (ver_dir / "sqlite3.c").exists() and (ver_dir / "sqlite3.h").exists():
            print(f"  {ver}: cached")
            continue

        year = VERSION_YEAR.get(ver)
        if year is None:
            print(f"  {ver}: SKIP (no year mapping)", file=sys.stderr)
            skipped += 1
            continue

        vint = version_int(ver)
        zip_name = f"sqlite-amalgamation-{vint}.zip"
        url = f"{base_url}/{year}/{zip_name}"

        try:
            data = urllib.request.urlopen(url).read()
        except Exception as e:
            print(f"  {ver}: FAILED ({url}: {e})", file=sys.stderr)
            failed += 1
            continue

        # Extract sqlite3.c and sqlite3.h from the zip.
        try:
            with zipfile.ZipFile(io.BytesIO(data)) as zf:
                # Find the amalgamation directory inside the zip.
                prefix = None
                for name in zf.namelist():
                    if name.endswith("/sqlite3.c"):
                        prefix = name[: name.rfind("/sqlite3.c")]
                        break

                if prefix is None:
                    print(f"  {ver}: FAILED (no sqlite3.c in zip)", file=sys.stderr)
                    failed += 1
                    continue

                ver_dir.mkdir(parents=True, exist_ok=True)
                for filename in ("sqlite3.c", "sqlite3.h"):
                    member = f"{prefix}/{filename}"
                    content = zf.read(member)
                    (ver_dir / filename).write_bytes(content)

            print(f"  {ver}: ok")
        except Exception as e:
            print(f"  {ver}: FAILED (extract: {e})", file=sys.stderr)
            failed += 1

    print()
    print(
        f"Done. {len(versions)} versions attempted, "
        f"{failed} failed, {skipped} skipped."
    )
    print(f"Amalgamations in {output_dir}/")

    return 1 if failed > 0 else 0


# ---------------------------------------------------------------------------
# update-data: download + audit + extract in one shot
# ---------------------------------------------------------------------------

DATA_DIR: Path = PROJECT_ROOT / "syntaqlite-buildtools" / "sqlite-vendored" / "data"


def cmd_update_data(args: argparse.Namespace) -> int:
    """Download amalgamations, audit cflags, and extract built-in catalogs."""
    amalgamation_dir = Path(args.amalgamation_dir)
    versions = args.versions or AMALGAMATION_VERSIONS

    # Step 1: Download amalgamations (skip if all versions already cached).
    missing = [
        v for v in versions
        if not (amalgamation_dir / v / "sqlite3.c").exists()
        or not (amalgamation_dir / v / "sqlite3.h").exists()
    ]
    if not missing:
        print(f"Step 1: All {len(versions)} amalgamation(s) already cached, skipping download.")
    else:
        print(f"Step 1: Downloading {len(missing)} amalgamation(s) to {amalgamation_dir}/")
        rc = cmd_download_amalgamations(argparse.Namespace(
            output_dir=str(amalgamation_dir),
            versions=missing,
        ))
        if rc != 0:
            return rc

    # Step 2: Build syntaqlite-buildtools.
    print()
    print("Step 2: Building syntaqlite-buildtools...")
    result = subprocess.run(
        ["cargo", "build", "--release", "-p", "syntaqlite-buildtools"],
        cwd=PROJECT_ROOT,
    )
    if result.returncode != 0:
        print("Build failed", file=sys.stderr)
        return result.returncode

    cli_bin = PROJECT_ROOT / "target" / "release" / "syntaqlite-buildtools"
    audit_output = str(DATA_DIR / "version_cflags.json")
    functions_output = str(DATA_DIR / "functions.json")
    relations_output = str(DATA_DIR / "relations.json")

    # Step 3: Audit cflags and extract built-in catalogs in one shot.
    print()
    print("Step 3: Updating sqlite data...")
    result = subprocess.run([
        str(cli_bin), "update-data",
        "--amalgamation-dir", str(amalgamation_dir),
        "--version-cflags-output", audit_output,
        "--functions-output", functions_output,
        "--relations-output", relations_output,
    ])
    if result.returncode != 0:
        print("update-data failed", file=sys.stderr)
        return result.returncode

    print()
    print("Done. Updated files:")
    print(f"  {audit_output}")
    print(f"  {functions_output}")
    print(f"  {relations_output}")
    return 0


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Download SQLite sources/amalgamations and extract data catalogs.",
    )
    sub = parser.add_subparsers(dest="command", required=True)

    # download-sources
    p_src = sub.add_parser(
        "download-sources",
        help="Download SQLite source files from the GitHub mirror.",
    )
    p_src.add_argument(
        "--output-dir", default="sqlite-sources",
        help="Output directory (default: sqlite-sources)",
    )
    p_src.add_argument(
        "--versions", nargs="+", default=None,
        help="Specific versions to download (default: built-in list)",
    )

    # download-amalgamations
    p_amal = sub.add_parser(
        "download-amalgamations",
        help="Download SQLite amalgamation archives from sqlite.org.",
    )
    p_amal.add_argument(
        "--output-dir", default="sqlite-amalgamations",
        help="Output directory (default: sqlite-amalgamations)",
    )
    p_amal.add_argument(
        "--versions", nargs="+", default=None,
        help="Specific versions to download (default: built-in list)",
    )

    # update-data: the main command -- download + audit + extract
    p_update = sub.add_parser(
        "update-data",
        help="Download amalgamations, audit cflags, and extract built-in catalogs.",
    )
    p_update.add_argument(
        "--amalgamation-dir", default="sqlite-amalgamations",
        help="Directory for amalgamations (default: sqlite-amalgamations)",
    )
    p_update.add_argument(
        "--versions", nargs="+", default=None,
        help="Specific versions to process (default: built-in list)",
    )

    args = parser.parse_args()

    if args.command == "download-sources":
        return cmd_download_sources(args)
    elif args.command == "download-amalgamations":
        return cmd_download_amalgamations(args)
    elif args.command == "update-data":
        return cmd_update_data(args)
    else:
        parser.print_help()
        return 1


if __name__ == "__main__":
    sys.exit(main())
