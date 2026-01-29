---
name: todo-github
description: Create, update, and access task lists as GitHub issues for this repo using GH CLI; use when asked to research repo context and turn a checklist into issues.
---

# Todo GitHub

## Overview

Turn a task checklist into GitHub issues for the current repository using the GH CLI. Include a quick repo scan, dedupe against existing issues, and clear issue bodies with acceptance criteria and likely touchpoints.

## Workflow (PowerShell)

### 1) Confirm repo and auth

- Run `gh repo view --json nameWithOwner,url` to confirm the target repository.
- If needed, set default repo with `gh repo set-default OWNER/REPO`.
- Verify login with `gh auth status`.

### 2) Quick context scan

- Read `README.md` and `DOCS.MD` to understand architecture and current behavior.
- Read `AGENTS.md` for local workflow rules.
- If tasks mention specific subsystems, skim those folders and files with `rg --files` and `rg "keyword"`.

### 3) Dedupe and plan issues

- List open issues: `gh issue list --state open --limit 200`.
- Search for close matches: `gh issue list --search "keyword"`.
- To pick what to work on, list issues with details:
  - `gh issue list --state open --limit 200 --json number,title,labels,updatedAt,url`
- Create one issue per checklist item unless the user says to consolidate.
- Preserve the checklist title text and punctuation exactly (including any prefix).

### 4) Compose issue content

Use this body template (adjust as needed):

```
## Summary
<1-2 lines on the problem and desired outcome>

## Done when
- <acceptance criteria>
- ...

## Likely touchpoints
- `path/`
- `path/`

## Notes
- <repro hints, edge cases, constraints>
```

Guidelines:
- Copy "Done when" criteria verbatim from the user when provided.
- Keep "Likely touchpoints" focused on concrete paths in the repo.
- Include safety constraints (for example, "save raw Markdown text") in Summary or Notes.

### 5) Create issues safely (avoid PowerShell quoting errors)

Preferred pattern:

```
@'
## Summary
...
'@ | gh issue create --title "Exact issue title" --body-file -
```

Notes:
- The `--body-file -` pipeline avoids quoting errors and preserves newlines.
- If the title includes single quotes, wrap the title in double quotes.
- If the title includes double quotes, wrap the title in single quotes and escape single quotes by doubling them.

### 6) Update issues with comments (default) and update ISSUES.md

- When adding new information to an existing issue, **append it as a comment** using `gh issue comment`.
- Do **not** rewrite or edit the original issue body unless the user explicitly asks to do so.
- If the user requests a body edit, use `gh issue edit` and mention what changed.

- Keep `ISSUES.md` in the repo root as the source-of-truth list of issues.
- After creating each issue, append/update a single line with: `#<number> - <title> - <url>`.
- On issue resolution, remove the line (or move to a resolved section if the user requests).
- Make sure `ISSUES.md` always matches the current GitHub issue state.

Example line:

```
#9 - FIX — Markdown support + Settings switch (write with Markdown elements) - https://github.com/nibzard/scribe/issues/9
```

### 7) Confirm and report

- Capture issue URLs from the `gh issue create` output.
- Report the issue numbers and titles back to the user.

## Troubleshooting

- If you see `unknown arguments ...`, it is a quoting issue. Re-run with `--body-file -` and a here-string.
- If you see `not logged in`, run `gh auth login` and retry.
- If the repo is wrong, run `gh repo set-default OWNER/REPO`, then re-run.

## Quality checklist

- Ensure the issue title matches the checklist item exactly.
- Ensure the body includes Summary, Done when, and Likely touchpoints.
- Ensure acceptance criteria are explicit and testable.
- Ensure each issue is independent unless the user asked to group items.

## Optional enhancements

- Use labels: `--label "bug"` or `--label "fix"`.
- Use milestones: `--milestone "vX.Y.Z"`.
- Use projects: `--project "Scribe"`.
