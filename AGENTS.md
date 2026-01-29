## Git Commits
- Use [Conventional Commits](https://www.conventionalcommits.org/) format: `<type>(<scope>): <description>`
- Common types: `feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`, `chore`
- Examples: `feat(editor): add autosave`, `fix(ui): resolve display crash`, `docs: update README`

## Releases
Version is defined in `CMakeLists.txt` as `PROJECT_VER`. To create a release:
1. Update `PROJECT_VER` in CMakeLists.txt (use semantic versioning: major.minor.patch)
2. Commit: `git add CMakeLists.txt && git commit -m "chore: bump version to X.Y.Z"`
3. Tag: `git tag -a vX.Y.Z -m "Release vX.Y.Z"`
4. Push tag: `git push origin vX.Y.Z`
5. Create GitHub release: `gh release create vX.Y.Z --title "Release X.Y.Z" --notes "Release notes here"`

## Notes
- Lessons learned for bring-up, flashing, logging, and debugging are tracked in `LESSONS_LEARNED.md`. Keep it updated as you make progress.
- Check `LESSONS_LEARNED.md` first when picking a display/driver strategy or debugging black-screen issues.
- Add a short entry to `LESSONS_LEARNED.md` after each debugging session so handoff is painless.
- If storage/SD issues recur, capture the root cause and mitigation steps in `LESSONS_LEARNED.md` before closing the task.
- Task lists live as GitHub Issues for this repo; create/access them with `gh issue create` and `gh issue list` (or the GitHub Issues page).
- Keep `DOCS.md` updated via the `todo-maintainer` skill when refactoring, fixing, or adding features.
- When starting a todo item, use the `feature-workflow` skill to branch, implement, and open a PR with `gh`.
