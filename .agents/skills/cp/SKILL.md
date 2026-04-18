---
name: cp
description: Commit all current changes, push to a feature branch, and create a PR. Use when the user asks to commit, push, save progress, or ship changes.
user_invocable: true
---

# commit-and-push (PR workflow)

Commit all current changes, push to a feature branch, and open a pull request.
All pre-push checks must pass before pushing.

## Instructions

1. **Run pre-push checks with auto-fix** (quiet mode suppresses output on success):
   ```sh
   tools/pre-push --fix -q
   ```
   If this fails, fix the issues and re-run until it passes.
   Do NOT skip this step — it is the project's only gate against broken code.

2. **Check for changes**:
   ```sh
   git status
   git diff --stat
   git log --oneline -5
   ```

3. **Stage all changes** (including any fixes from step 1):
   ```sh
   git add -A
   ```

4. **Write a commit message** following the project convention:
   - Prefix with `synq: ` (lowercase)
   - Concise summary line describing the "why"
   - For non-trivial changes, add a structured body: a short lead-in
     paragraph explaining the motivation, then a bullet list covering
     the specifics (e.g. one bullet per file / module / subsystem
     touched). This body also becomes the PR description in step 8,
     so invest in it once.

5. **Commit using a HEREDOC**:
   ```sh
   git commit -m "$(cat <<'EOF'
   synq: <summary>

   <optional body>
   EOF
   )"
   ```

6. **Create a feature branch if on main**:
   If currently on `main`, create and switch to a descriptive branch:
   ```sh
   git checkout -b <branch-name>
   ```
   Branch naming: use lowercase kebab-case describing the change (e.g.,
   `add-cte-column-validation`, `fix-fmt-trailing-comma`). No prefixes needed.

7. **Push the branch**:
   ```sh
   git push -u origin HEAD
   ```

8. **Create a PR** using `gh pr create`. For single-commit PRs the
   body should be the commit body verbatim — the lead-in + bullets
   structure from step 4 is exactly the structure a PR wants. Don't
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

9. **Report the PR URL** to the user.
