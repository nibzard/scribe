# TODO

## Bugs to investigate
- [ ] Cursor drifts left from typed text over time (worse after multiple spaces); TextView uses fixed-width + ASCII-only measurement.
- [ ] Recovery dialog appears on boot, but buttons do nothing; Enter clears to empty screen (no touch handlers?).
- [ ] Wi-Fi can be toggled on, but no networks are visible/connection fails (UI has no scan/connect flow).
- [ ] Settings keyboard layout cycles only 4 options (US/UK/DE/FR); HR exists but is unreachable.
- [ ] Esc menu box height larger than its content/menu list.
- [ ] Font size picker first open shows only a vertical line; reopening shows the list.
- [ ] Editor font size stops increasing beyond ~24–28px; displayed size stays the same after that.
- [ ] “Saved” toast shows a box glyph instead of the expected symbol/character (font/glyph missing).

## UX / Features
- [ ] Add subtle text margins/insets (left + right, not just narrower wrap); provide 3 margin size options in settings.
- [ ] "Switch project" menu: rename to "Projects" and enable delete with Cmd/Win+D (confirm archive vs hard delete).
- [ ] Clarify project model (projects vs notes/pages); adjust management UX as needed.
- [ ] Add Croatian (HR) keyboard: enable in settings + support UTF-8 characters (Croatian diacritics) in input/editor/rendering.
- [ ] Add brightness adjustment in settings + keyboard shortcut to change brightness.