# AGENTS.md for zmk-keyboard-cygnus-m

This repository is the source of truth for Ren's Cygnus-M ZMK firmware.

## Scope

- Use this repository for Cygnus-M firmware, keymap, AML, PMW3610, build, and
  flash work.
- Do not use `/Users/ren/workspace/private/torabo-tsuki` as the working
  directory or source of truth. It is only historical reference if the user
  explicitly asks for archaeology.
- Prefer Cygnus-specific docs in this repository over older migration notes.

## Skills

- Use `cygnus-zmk-maintenance` for Cygnus-M maintenance, builds, flashing,
  Windows gaming layer work, BLE issues, and keymap changes.
- Use `porting-zmk-aml` only when comparing or porting AML behavior between
  keyboard repositories.
- If updating skills or shared agent assets, edit the source of truth under
  `~/claude-dotfiles/common/` and sync; do not edit generated Codex/Claude
  copies directly.

## Working Rules

- Check `git status --short --branch` before editing.
- Keep `artifacts/` untracked unless the user explicitly asks to commit build
  outputs.
- Validate each keymap layer has the Cygnus physical shape:
  `12 + 14 + 14 + 11 = 51` bindings.
- Preserve AML gesture order:
  - `K -> J`: Cmd + wheel pinch zoom.
  - `J -> K`: left-click-held + wheel click-scroll.
- Do not make those gestures order-independent.
- For keymap/central behavior changes, flashing the right half is usually
  enough. Flash the left half only for left/peripheral changes or split
  recovery.

## Docs

- Start with `docs/cygnus-maintenance.md` for current operation notes.
- `docs/aml-port.md` is kept for migration history and AML details.
