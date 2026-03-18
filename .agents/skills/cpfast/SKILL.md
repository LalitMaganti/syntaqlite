---
name: cpfast
description: "Quickly commit and push current changes to a feature branch and create a PR, skipping pre-push checks. WARNING: if CI starts failing, use /cp instead."
user_invocable: true
---

# commit-and-push-fast (PR workflow)

Commit all current changes, push to a feature branch, and open a pull request.
Skips pre-push checks for speed — CI will catch issues.

> **Note**: This skips `tools/pre-push`. If CI starts failing after using this,
> switch to `/cp` which runs the full presubmit gate.

## Instructions

1. **Check for changes**:
   ```sh
   git status
   git diff --stat
   git log --oneline -5
   ```

2. **Stage all changes**:
   ```sh
   git add -A
   ```

3. **Write a commit message** following the project convention:
   - Prefix with `synq: ` (lowercase)
   - Concise summary line describing the "why"
   - Add detail in the body for non-trivial changes

4. **Commit using a HEREDOC**:
   ```sh
   git commit -m "$(cat <<'EOF'
   synq: <summary>

   <optional body>
   EOF
   )"
   ```

5. **Create a feature branch if on main**:
   If currently on `main`, create and switch to a descriptive branch:
   ```sh
   git checkout -b <branch-name>
   ```
   Branch naming: use lowercase kebab-case describing the change (e.g.,
   `add-cte-column-validation`, `fix-fmt-trailing-comma`). No prefixes needed.

6. **Push the branch**:
   ```sh
   git push -u origin HEAD
   ```

7. **Create a PR** using `gh pr create`:
   ```sh
   gh pr create --title "<title>" --body "$(cat <<'EOF'
   ## Motivation

   <Why this change is needed — what problem exists, what's missing, what broke>

   ## Changes

   <What this PR does to address the motivation>
   EOF
   )"
   ```
   - Keep the title under 70 characters, prefixed with `synq: `
   - Motivation section: explain the problem/need driving this change
   - Changes section: describe what you're doing about it

8. **Report the PR URL** to the user.
