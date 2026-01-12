# Contributing

Thanks for your interest in contributing to Scribe.

## Getting started
- Requires ESP-IDF v5.0+.
- Build with `idf.py build` from the repo root.

## Development guidelines
- Keep changes focused and well-scoped.
- Prefer small, reviewable pull requests.
- Avoid blocking UI paths on I/O; background work should stay off the UI task.
- Keep user-facing copy aligned with `SPECS.md` and `assets/strings/en.json`.

## Testing
- Run `idf.py build` for compile validation.
- Add or update tests when behavior changes are non-trivial.

## Reporting issues
- Include device/firmware context, steps to reproduce, and logs if available.

## Code of Conduct
By participating, you agree to follow the Code of Conduct in `CODE_OF_CONDUCT.md`.
