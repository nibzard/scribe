---
name: doc-maintainer
description: Maintain and expand DOCS.MD to cover the full Scribe codebase. Use when asked to update documentation, add missing modules/flows, improve coverage maps, or align docs with code changes in this repo.
---

# Doc Maintainer

## Goal
Keep `DOCS.MD` accurate, comprehensive, and aligned with the current codebase.

## Workflow
1. Scan the repo for new or changed areas (prefer `rg --files` + targeted file reads).
2. Compare code areas to `DOCS.MD` sections and the Table of Contents.
3. Add or update documentation so each major subsystem and user-facing flow is covered.
4. Keep changes concise, factual, and tied to the actual implementation.

## Coverage Rules
- Ensure every top-level folder with code or assets is represented in `DOCS.MD`.
- For each subsystem, list:
  - Responsibilities (1-3 bullets).
  - Key entry files (headers + sources).
- If new developer sections are added, update the Table of Contents to include them.
- Prefer a single “Codebase Map” section for breadth, and add focused sections only for complex features.

## DOCS.MD Style
- Use short paragraphs and bullet lists; avoid long prose.
- Keep headings numbered to match the Table of Contents.
- Use inline code for file paths.
- Mention specific files for claims; avoid vague wording like “some files.”

## When Updating After Code Changes
- If a new service or screen is added, document it under the relevant subsystem.
- If behavior changes (e.g., keybindings, UI flow), update both the behavior summary and key files.
- If a file is deprecated or unused, note it briefly rather than deleting documentation.

## Do Not
- Do not add unrelated documentation files; only update `DOCS.MD`.
- Do not describe managed components in depth; note them at a high level.
