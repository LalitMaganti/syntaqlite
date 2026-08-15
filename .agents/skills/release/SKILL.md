# release

Bump version, commit, tag, and create a draft GitHub Release. Use when the user
asks to "release", "bump version", "tag a release", or "publish".

## Instructions

1. **Determine the new version.** Read the current version from
   `python/pyproject.toml` and increment the patch number automatically.
   If the user specifies a version explicitly, use that instead.

2. **Run the bump script:**
   ```sh
   python3 tools/bump-version <new_version> --check
   ```
   This updates all Cargo.toml files, pyproject.toml, CHANGELOG, README,
   docs, and lib.rs references. `--check` verifies `cargo check` passes.

   The bump script adds a `## <new_version>` section to `CHANGELOG.md`
   with a `*No changes yet.*` placeholder. Do NOT edit the CHANGELOG
   before running bump-version; it will create a duplicate section.

3. **Write the CHANGELOG.** After bump-version runs, replace the
   `*No changes yet.*` placeholder in `CHANGELOG.md` with real entries.
   Use `git log` to find commits since the previous release tag.

   Focus on **user-visible changes only**:
   - New features, new CLI flags, new API surface
   - Bug fixes that affected users
   - Breaking changes or renamed options
   - Documentation improvements (briefly)

   Do NOT include:
   - Internal refactors, code cleanup, clippy fixes
   - CI/release pipeline plumbing
   - Changes to dev tooling or build scripts

   Keep entries concise, with one line per change and no sub-bullets.

4. **Commit on a release branch and open a PR.** `main` is protected and
   rejects direct pushes, so the bump must land via PR.
   ```sh
   git checkout -b release-<new_version>
   git add -A
   git commit -m "$(cat <<'EOF'
   synq: bump version to <new_version>
   EOF
   )"
   git push -u origin HEAD
   gh pr create --title "synq: bump version to <new_version>" --body "$(cat <<'EOF'
   ## Motivation

   Cut the <new_version> release.

   ## Changes

   - Bumps version to <new_version> across all Cargo.toml / pyproject.toml /
     integration manifests / README / docs.
   - Adds <new_version> CHANGELOG entry.

   Release notes:

   <paste the CHANGELOG entries for this version here>
   EOF
   )"
   ```
   **Stop and wait for the user to merge the PR** before continuing. Do not
   attempt to merge it yourself; the user decides when the release goes out.

5. **After the PR merges**, fast-forward local main and tag the merge commit:
   ```sh
   git checkout main
   git pull origin main
   git tag v<new_version>
   git push origin v<new_version>
   ```

6. **Create a draft GitHub Release.** Read CHANGELOG.md to get the release
   notes for this version. Ask the user if they want to edit the notes or
   if the CHANGELOG content is sufficient.
   ```sh
   gh release create v<new_version> --draft --title "v<new_version>" \
     --notes "$(release notes here)"
   ```
   The tag must already exist on the remote before `gh release create`
   will work, which is why we push it first.

   Pushing the tag triggers build workflows that upload artifacts to the
   draft release:
   - `release.yml`: cargo-dist builds CLI binaries, Homebrew tap, installers
   - `publish-crates.yml`: publishes to crates.io
   - `vscode-extension.yml`: builds VS Code .vsix artifacts
   - `release-amalgamation.yml`: C source amalgamation archive
   - `release-clib.yml`: prebuilt C shared library archive

7. **Report** the draft release URL to the user and remind them to publish
   the release manually once the builds complete and look good.
