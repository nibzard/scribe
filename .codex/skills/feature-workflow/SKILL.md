---
name: feature-workflow
description: Work on a new feature or fix by creating a branch, implementing changes, and submitting a PR with GH CLI. Use when picking up a new todo item.
---

# Feature Workflow

## Overview

Create a branch per task, implement the change, and open a pull request using `gh`. Keep the workflow lightweight and consistent.

## Workflow (PowerShell)

### 1) Identify the task

- Confirm the todo item or issue number.
- If needed, note acceptance criteria and key touchpoints.

### 2) Create a branch

- Pull latest default branch:
  - `git fetch origin`
  - `git checkout main`
  - `git pull`
- Create a branch named after the task:
  - `git checkout -b fix/<short-slug>`
  - or `git checkout -b feat/<short-slug>`

### 3) Implement the change

- Make focused changes tied to the task.
- Update docs or tests if required by the task.

### 4) Commit changes

- Use Conventional Commits:
  - `feat(scope): ...`
  - `fix(scope): ...`
- Include only relevant files.

### 5) Push and open PR

- Push the branch:
  - `git push -u origin <branch>`
- Create PR with GH CLI:
  - `gh pr create --title "..." --body "..."`

## PR body template

```
## Summary
<what changed and why>

## Testing
- <commands or "not run (reason)">

## Checklist
- [ ] Meets acceptance criteria
- [ ] Docs/strings updated if needed
```

## Quality checklist

- Branch name matches task intent.
- Commit message follows Conventional Commits.
- PR references the issue number if applicable.
- PR body includes summary and testing.
