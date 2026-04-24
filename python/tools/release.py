# Copyright 2026 The syntaqlite Authors. All rights reserved.
# Licensed under the Apache License, Version 2.0.

"""Cut a release: bump version, open a PR, tag, draft a GitHub Release.

Flow:
  tools/release [VERSION]        bump version on a release branch and open a PR.
                                 If VERSION is omitted, the patch number in
                                 python/pyproject.toml is incremented.
                                 Drafts CHANGELOG entries with `claude -p`
                                 (if available), then pauses so you can review
                                 and edit before committing.
  tools/release [VERSION] --manual
                                 Same as above but skips the Claude draft
                                 and leaves the `*No changes yet.*` placeholder
                                 for you to fill in by hand.
  tools/release publish          After the PR merges: pull main, tag v<VERSION>,
                                 push the tag, and create a draft GitHub
                                 Release seeded with the CHANGELOG entry.
"""

from __future__ import annotations

import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
PYPROJECT = ROOT / "python" / "pyproject.toml"
CHANGELOG = ROOT / "CHANGELOG.md"
PLACEHOLDER = "*No changes yet.*"


def run(cmd: list[str], *, check: bool = True, capture: bool = False) -> subprocess.CompletedProcess:
    return subprocess.run(
        cmd,
        cwd=ROOT,
        check=check,
        text=True,
        stdout=subprocess.PIPE if capture else None,
        stderr=subprocess.PIPE if capture else None,
    )


def read_pyproject_version() -> str:
    for line in PYPROJECT.read_text().splitlines():
        m = re.match(r'^version\s*=\s*"([^"]+)"\s*$', line)
        if m:
            return m.group(1)
    sys.exit(f"could not find version in {PYPROJECT}")


def bump_patch(version: str) -> str:
    parts = version.split(".")
    if len(parts) != 3 or not all(p.isdigit() for p in parts):
        sys.exit(f"cannot auto-bump non-semver version {version!r}")
    parts[-1] = str(int(parts[-1]) + 1)
    return ".".join(parts)


def latest_release_tag() -> str | None:
    r = run(
        ["git", "describe", "--tags", "--abbrev=0", "--match", "v*"],
        check=False,
        capture=True,
    )
    return r.stdout.strip() if r.returncode == 0 else None


def commits_since(tag: str | None) -> str:
    spec = f"{tag}..HEAD" if tag else "HEAD"
    r = run(["git", "log", "--oneline", spec], capture=True)
    return r.stdout.strip()


def changelog_section(version: str) -> str:
    """Return the body of the `## <version>` section, or '' if missing."""
    text = CHANGELOG.read_text()
    header = f"\n## {version}\n"
    start = text.find(header)
    if start == -1:
        return ""
    start += len(header)
    end = text.find("\n## ", start)
    if end == -1:
        end = len(text)
    return text[start:end].strip()


def draft_changelog_via_claude(tag: str | None, version: str) -> str | None:
    """Ask `claude -p` to draft CHANGELOG entries from the commit log.

    Returns the generated section body (no `## <version>` header) on success,
    or None if claude isn't available / fails / produces empty output.
    """
    if shutil.which("claude") is None:
        return None

    spec = f"{tag}..HEAD" if tag else "HEAD"
    log = run(
        ["git", "log", "--no-merges", "--pretty=format:%h %s", spec],
        capture=True,
    ).stdout.strip()
    if not log:
        return ""

    # Grab the 3 most recent release sections as a style reference.
    full = CHANGELOG.read_text()
    body_start = full.find("\n## ")
    style_ref = ""
    if body_start != -1:
        chunk = full[body_start:]
        # Split into sections and keep the first few non-empty ones.
        sections = re.split(r"(?m)^## ", chunk)
        # sections[0] is '' (from the leading split), so take [1:4].
        picked = [f"## {s.rstrip()}" for s in sections[1:4] if s.strip()]
        style_ref = "\n\n".join(picked)

    prompt = f"""Draft CHANGELOG.md entries for syntaqlite {version} from the commits below.

Output rules:
- User-visible changes only: new features, user-facing bug fixes, breaking changes, notable doc updates.
- Exclude internal refactors, code cleanup, clippy fixes, CI/release plumbing, dev tooling.
- Group entries under **Category:** headers (Parser, Formatter, CLI, C API, Build, Analysis and validator, JavaScript package, Breaking, etc.) matching the style reference.
- One concise line per entry, past-tense or imperative voice matching the style reference.
- Every entry ends with a PR link in the form ([#NNN](https://github.com/LalitMaganti/syntaqlite/pull/NNN)). Extract the number from commit subjects that end in `(#NNN)`; skip entries whose commits have no PR number.
- Output ONLY the section body — no `## {version}` header, no code fences, no commentary. The output will be inserted verbatim under the header.
- If nothing user-visible happened, output exactly: Internal improvements only.

Style reference (recent CHANGELOG sections — match this format):
---
{style_ref}
---

Commits since {tag or 'project start'}:
---
{log}
---
"""

    print("Drafting CHANGELOG entries via `claude -p`...")
    try:
        r = subprocess.run(
            ["claude", "-p", prompt],
            cwd=ROOT,
            check=True,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
    except FileNotFoundError:
        return None
    except subprocess.CalledProcessError as exc:
        print(f"claude -p failed (exit {exc.returncode}):", file=sys.stderr)
        if exc.stderr:
            print(exc.stderr.strip(), file=sys.stderr)
        return None

    draft = r.stdout.strip()
    return draft or None


def apply_draft_to_changelog(version: str, draft: str) -> bool:
    """Replace the `*No changes yet.*` placeholder under `## <version>` with draft."""
    text = CHANGELOG.read_text()
    header = f"\n## {version}\n"
    start = text.find(header)
    if start == -1:
        return False
    start += len(header)
    end = text.find("\n## ", start)
    if end == -1:
        end = len(text)
    section = text[start:end]
    if PLACEHOLDER not in section:
        return False
    new_section = section.replace(PLACEHOLDER, draft, 1).rstrip() + "\n"
    CHANGELOG.write_text(text[:start] + new_section + text[end:])
    return True


def ensure_clean_worktree() -> None:
    r = run(["git", "status", "--porcelain"], capture=True)
    if r.stdout.strip():
        sys.exit(
            "working tree has uncommitted changes; commit or stash them first:\n"
            + r.stdout
        )


def prepare(version: str | None, *, manual: bool) -> int:
    ensure_clean_worktree()

    current = read_pyproject_version()
    new_version = version or bump_patch(current)
    if new_version == current:
        sys.exit(f"new version {new_version} equals current version")

    print(f"Bumping {current} -> {new_version}")
    run(["python3", "tools/bump-version", new_version, "--check"])

    tag = latest_release_tag()
    log = commits_since(tag)
    span = f"since {tag}" if tag else "(no prior tag found)"
    print()
    print(f"Commits {span}:")
    print(log if log else "  (none)")
    print()

    drafted = False
    if not manual:
        draft = draft_changelog_via_claude(tag, new_version)
        if draft:
            if apply_draft_to_changelog(new_version, draft):
                drafted = True
                print("\nDraft CHANGELOG entries written under "
                      f"## {new_version}:\n")
                print(draft)
                print()
            else:
                print("(could not splice Claude draft into CHANGELOG.md — "
                      "falling back to manual edit)")
        else:
            print("(claude -p unavailable or returned no content — "
                  "falling back to manual edit)")

    if drafted:
        print("Review the drafted entries — Claude can hallucinate, drop user-")
        print("visible changes, or keep internal ones. Edit CHANGELOG.md to fix")
        print("anything wrong before continuing.")
    else:
        print(f"Edit CHANGELOG.md — replace '{PLACEHOLDER}' under "
              f"'## {new_version}' with user-visible changes only (new features,")
        print("fixes that hit users, breaking changes, notable doc updates).")
        print("Skip internal refactors, CI plumbing, and dev tooling.")
    print()

    editor = os.environ.get("EDITOR")
    if editor and sys.stdin.isatty():
        try:
            input(f"Press Enter to open CHANGELOG.md in $EDITOR ({editor}), or Ctrl-C to abort: ")
            subprocess.run([*editor.split(), str(CHANGELOG)], check=True)
        except KeyboardInterrupt:
            print("\naborted")
            return 1
    else:
        try:
            input("Edit CHANGELOG.md now, then press Enter to continue (Ctrl-C to abort): ")
        except KeyboardInterrupt:
            print("\naborted")
            return 1

    section = changelog_section(new_version)
    if not section or PLACEHOLDER in section:
        sys.exit(
            f"CHANGELOG.md still has the '{PLACEHOLDER}' placeholder under "
            f"## {new_version}; edit it and re-run, or delete the section "
            "and re-run with a different version."
        )

    branch = f"release-{new_version}"
    print(f"\nCreating branch {branch} and committing bump")
    run(["git", "checkout", "-b", branch])
    run(["git", "add", "-A"])
    run(["git", "commit", "-m", f"synq: bump version to {new_version}"])
    run(["git", "push", "-u", "origin", "HEAD"])

    body = (
        f"## Motivation\n\nCut the {new_version} release.\n\n"
        f"## Changes\n\n- Bumps version to {new_version} across all manifests / "
        f"README / docs.\n- Adds {new_version} CHANGELOG entry.\n\n"
        f"## Release notes\n\n{section}\n"
    )
    run([
        "gh", "pr", "create",
        "--title", f"synq: bump version to {new_version}",
        "--body", body,
    ])
    print()
    print(f"PR opened for {branch}. After it merges, run:")
    print("  tools/release publish")
    return 0


def publish() -> int:
    ensure_clean_worktree()

    print("Pulling main")
    run(["git", "checkout", "main"])
    run(["git", "pull", "origin", "main"])

    version = read_pyproject_version()
    tag = f"v{version}"

    head_msg = run(["git", "log", "-1", "--pretty=%s"], capture=True).stdout.strip()
    expected = f"synq: bump version to {version}"
    if expected not in head_msg:
        sys.exit(
            f"refusing to tag: HEAD commit is {head_msg!r}, expected to contain "
            f"{expected!r}. Make sure the release PR has merged and main is "
            "up to date."
        )

    existing = run(["git", "tag", "--list", tag], capture=True).stdout.strip()
    if existing:
        sys.exit(f"tag {tag} already exists locally")

    print(f"Tagging {tag} and pushing")
    run(["git", "tag", tag])
    run(["git", "push", "origin", tag])

    section = changelog_section(version)
    if not section:
        sys.exit(f"could not find CHANGELOG.md section '## {version}'")

    print(f"Creating draft GitHub release {tag}")
    run([
        "gh", "release", "create", tag,
        "--draft",
        "--title", tag,
        "--notes", section,
    ])
    print()
    print(f"Draft release {tag} created. Once build workflows finish and")
    print("artifacts look good, publish the release from the GitHub UI.")
    return 0


USAGE = """\
usage: tools/release [VERSION] [--manual]   bump version, draft CHANGELOG via
                                            `claude -p`, open a PR
       tools/release publish                after merge: tag main and draft
                                            GitHub release
       tools/release --help

If VERSION is omitted from the bump form, the patch number in
python/pyproject.toml is incremented.

--manual skips the Claude draft and leaves the `*No changes yet.*` placeholder
for you to fill in by hand.
"""


def main(argv: list[str]) -> int:
    if shutil.which("gh") is None:
        sys.exit("gh CLI not found on PATH; install it to use tools/release")

    if argv and argv[0] in {"-h", "--help"}:
        print(USAGE)
        return 0
    if argv and argv[0] == "publish":
        if len(argv) != 1:
            sys.exit("`tools/release publish` takes no extra arguments")
        return publish()

    manual = False
    positional: list[str] = []
    for arg in argv:
        if arg == "--manual":
            manual = True
        elif arg.startswith("-"):
            sys.exit(f"unknown flag {arg!r}\n{USAGE}")
        else:
            positional.append(arg)
    if len(positional) > 1:
        sys.exit(USAGE)
    version = positional[0] if positional else None
    return prepare(version, manual=manual)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
