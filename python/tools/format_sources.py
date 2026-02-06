# Copyright 2025 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Formats C/C++ source files using clang-format."""

import argparse
import os
import subprocess
import sys

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
EXTS = frozenset(('.c', '.h', '.cc'))


def find_clang_format():
    """Locate the clang-format binary in third_party/bin/."""
    from python.tools.run_third_party_binary import get_platform_dir
    os_dir, ext = get_platform_dir()
    if os_dir is None:
        return None
    return os.path.join(ROOT_DIR, 'third_party', 'bin', os_dir, 'clang-format' + ext)


def build_file_list(args):
    """Build the list of files to format."""
    if args.files:
        return args.files

    if args.all:
        cmd = ['git', 'ls-files']
        output = subprocess.check_output(cmd, text=True, cwd=ROOT_DIR)
        return output.splitlines()

    # Default: changed files vs upstream.
    upstream = args.upstream
    if upstream is None:
        try:
            cmd = ['git', 'rev-parse', '--abbrev-ref', '--symbolic-full-name', '@{u}']
            upstream = subprocess.check_output(
                cmd, text=True, stderr=subprocess.DEVNULL, cwd=ROOT_DIR).strip()
        except subprocess.CalledProcessError:
            upstream = 'origin/main'
    cmd = ['git', 'diff', '--name-only', '--diff-filter=crd', upstream]
    output = subprocess.check_output(cmd, text=True, cwd=ROOT_DIR).strip()
    return output.splitlines() if output else []


def filter_files(files):
    """Keep only formattable source files, excluding third_party and generated."""
    result = []
    for f in files:
        if not any(f.endswith(ext) for ext in EXTS):
            continue
        if f.startswith(('third_party/', 'out/')):
            continue
        if f.endswith('_gen.c') or f.endswith('_gen.h'):
            continue
        result.append(f)
    return result


def main():
    parser = argparse.ArgumentParser(description='Format C/C++ sources with clang-format.')
    parser.add_argument('--check-only', action='store_true',
                        help='Check formatting without modifying files (exit 1 if unformatted).')
    parser.add_argument('--all', action='store_true',
                        help='Format all versioned source files, not just changed ones.')
    parser.add_argument('--upstream', type=str, default=None,
                        help='Upstream branch to diff against (default: auto-detect or origin/main).')
    parser.add_argument('files', nargs='*',
                        help='Explicit list of files to format.')
    args = parser.parse_args()

    clang_format = find_clang_format()
    if clang_format is None or not os.path.exists(clang_format):
        print('Cannot find clang-format.', file=sys.stderr)
        print('Run tools/dev/install-build-deps to install it.', file=sys.stderr)
        return 127

    os.chdir(ROOT_DIR)
    files = filter_files(build_file_list(args))
    if not files:
        print('No files to format.')
        return 0

    print(f'Formatting {len(files)} file(s)...')
    cmd = [clang_format, '--Werror']
    cmd += ['--dry-run' if args.check_only else '-i']
    cmd += files

    try:
        subprocess.check_call(cmd)
    except subprocess.CalledProcessError as ex:
        if args.check_only:
            print('Run tools/dev/format-sources to fix.', file=sys.stderr)
        return ex.returncode
    return 0


if __name__ == '__main__':
    sys.exit(main())
