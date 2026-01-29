---
name: release-runbook
description: Prepare and publish a GitHub release with semantic versioning, updating PROJECT_VER and CHANGELOG, tagging, and using GH CLI to publish. Use after major changes or when asked to cut a release.
---

# Release Runbook

## Overview

Create a new release for this repo by updating semantic versions, maintaining `CHANGELOG`, tagging the release, and publishing it to GitHub via `gh`.

## Workflow (PowerShell)

### 1) Pre-flight checks

- Confirm repo and auth:
  - `gh repo view --json nameWithOwner,url`
  - `gh auth status`
- Ensure working tree is clean or changes are intentional.

### 2) Decide the version (semantic versioning)

- Use `MAJOR.MINOR.PATCH`.
  - **MAJOR**: breaking changes
  - **MINOR**: new features, backward compatible
  - **PATCH**: fixes only
- If unsure, summarize changes and pick the smallest correct bump.

### 3) Update version and changelog

- Update `PROJECT_VER` in `CMakeLists.txt`.
- Update `CHANGELOG` (add a new section for `vX.Y.Z` with date and highlights).
- Keep entries concise and user-facing; group by Added/Changed/Fixed/Breaking.

### 4) Commit the release prep

- Commit `CMakeLists.txt` and `CHANGELOG` together.
- Use Conventional Commits, e.g.:
  - `chore: bump version to X.Y.Z`

### 5) Tag the release

- Create an annotated tag:
  - `git tag -a vX.Y.Z -m "Release vX.Y.Z"`
- Push the tag:
  - `git push origin vX.Y.Z`

### 6) Create the GitHub release

- Use the changelog section as release notes:
  - `gh release create vX.Y.Z --title "Release X.Y.Z" --notes "Release notes here"`
- If the repo uses a template, follow it exactly.

## Quality checklist

- `PROJECT_VER` matches the tag `vX.Y.Z`.
- `CHANGELOG` contains the new version section and date.
- Tag is pushed and visible on GitHub.
- Release notes match the changelog highlights.

## Troubleshooting

- If tag exists, increment the version and retry.
- If release creation fails, re-run `gh release create` with the same tag.
