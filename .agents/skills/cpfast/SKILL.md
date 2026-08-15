---
name: cpfast
description: "Quickly commit and push current changes to a feature branch and create a PR, skipping pre-push checks. WARNING: if CI starts failing, use /cp instead."
user_invocable: true
---

# commit-and-push-fast (PR workflow)

Commit all current changes, push to a feature branch, and open a pull request.
Skips pre-push checks for speed; CI will catch issues.

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
   - For non-trivial changes, add a structured body: a short lead-in
     paragraph explaining the motivation, then a bullet list covering
     the specifics (e.g. one bullet per file / module / subsystem
     touched). This body also becomes the PR description in step 7,
     so invest in it once.

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

7. **Create a PR** using `gh pr create`. For single-commit PRs the
   body should be the commit body verbatim; the lead-in + bullets
   structure from step 3 is exactly the structure a PR wants. Don't
   rewrite it into a single paragraph, don't add section headings,
   don't restate the title. The shortcut:
   ```sh
   git log -1 --format=%b HEAD | gh pr create --title "<title>" --body-file -
   ```
   For multi-commit PRs, write the body in the same shape (short
   lead-in + bullets covering the important changes across commits).
   One-liners are fine only when the title and diff genuinely speak
   for themselves.
   - Keep the title under 70 characters, prefixed with `synq: `

8. **Report the PR URL** to the user.
