# Cygnus-M Maintenance Notes

This repository is the working home for Ren's Cygnus-M ZMK firmware. The old
`torabo-tsuki` workspace is historical only; do not use it as the current
source of truth.

## Repository

- Path: `/Users/ren/workspace/private/zmk-keyboard-cygnus-m`
- GitHub: `RenMunetsuna/Cygnus-M`
- Main keymap: `config/Cygnus.keymap`
- Right shield config: `config/boards/shields/Test/Cygnus_R.conf`
- Left shield config: `config/boards/shields/Test/Cygnus_L.conf`
- Flash helper: `scripts/flash-uf2.sh`

Generated Actions artifacts are downloaded under `artifacts/` and should stay
untracked.

## Build Flow

Use GitHub Actions for firmware builds.

```sh
git status --short --branch
git diff --check
git push origin main
gh workflow run build.yml --repo RenMunetsuna/Cygnus-M --ref main
gh run watch RUN_ID --repo RenMunetsuna/Cygnus-M --exit-status
gh run download RUN_ID --repo RenMunetsuna/Cygnus-M --dir artifacts/run-RUN_ID
```

For doc-only changes, a firmware build is not required.

## Keymap Validation

Cygnus-M has 51 positions per layer:

```text
12 + 14 + 14 + 11
```

Before committing keymap changes, count every layer:

```sh
python3 - <<'PY'
from pathlib import Path
import re
s = Path('config/Cygnus.keymap').read_text()
for m in re.finditer(r'([A-Za-z0-9_]+) \{\s*bindings = <(.*?)>;', s, re.S):
    rows = [r.strip() for r in m.group(2).strip().splitlines() if r.strip()]
    print(m.group(1), [r.count('&') for r in rows], 'total', m.group(2).count('&'))
PY
```

Expected output for each real layer is `[12, 14, 14, 11] total 51`.

## Flash Flow

The right half is central. Most keymap and custom behavior changes only need
the right firmware flashed.

```sh
scripts/flash-uf2.sh --side right --run latest
scripts/flash-uf2.sh --side right --run RUN_ID
scripts/flash-uf2.sh --side left --run RUN_ID
scripts/flash-uf2.sh --side reset --run RUN_ID
```

Put the target half into UF2 bootloader mode, then run the helper. The helper
waits for one UF2 volume under `/Volumes` and copies the selected UF2.

If one side stops responding after a split/BLE-sensitive change:

```text
left:  settings_reset -> Cygnus_L
right: settings_reset -> Cygnus_R
```

Then reset the right central once and wait a few seconds for the split link.

## Current Behavior Invariants

Preserve these unless the user explicitly changes the design.

- `K` is the AML scroll trigger. It is not middle click.
- Fusion orbit uses `Shift + middle click + trackball`.
- `K -> J`: hold Cmd while wheel events are emitted for pinch zoom.
- `J -> K`: keep left click held while wheel events are emitted.
- Do not make `K -> J` and `J -> K` order-independent.
- `bt_toggle_01` toggles BT profile `0 <-> 1`.
- Returning to BT profile 0 deactivates layer 6.

## OS-Aware Keys

BT profile 0 is treated as Mac. Other profiles are treated as Windows/game.

- Far-right bottom key on base:
  - Mac/profile 0: middle click.
  - Windows/profile 1+: toggle layer 6.
- H-left extra key:
  - Mac/profile 0: Command + left click.
  - Windows/profile 1+: Control + left click.
  - It has a small modifier settle delay for Windows BLE.

If the H-left new-tab click behaves like a normal click on Windows, check host
key remapping first. The user has used KeySwap/Scancode Map-style Ctrl/Win
remapping before, and that can make firmware-level OS shortcuts look wrong.

## Layer 6: Windows Gaming

Layer 6 is the Windows gaming layer. It is toggled from Windows/profile 1 by
the far-right bottom key and exits with the same physical key while layer 6 is
active.

Current left side:

```text
ESC    Q    W    E    R    T
LCTRL  A    S    D    F    G
LSHFT  Z    X    C    V    B    SPACE
N      M    K    L    1    2    3
```

Current right side keeps host switching and a few utility keys:

- `bt_toggle_01` is available on the right half.
- The right thumb cluster includes `ESC`, `RET`, `SPACE`, and layer 6 toggle.

RGB layer colors are intentionally disabled for layers 1/2/3/4/5/7 and enabled
only for layer 6, so the keyboard visibly indicates game mode without lighting
every transient layer.

## PMW3610 And BLE Notes

Current right-side pointing settings:

```conf
CONFIG_PMW3610_CPI=450
CONFIG_PMW3610_CPI_DIVIDOR=1
CONFIG_PMW3610_SNIPE_CPI=600
CONFIG_PMW3610_SNIPE_CPI_DIVIDOR=4
CONFIG_PMW3610_POLLING_RATE_125_SW=y
CONFIG_PMW3610_MOVEMENT_THRESHOLD=5
CONFIG_PMW3610_SMART_ALGORITHM=y
```

Known Windows BLE host issue:

- I-O DATA `USB-BT50LE` was unstable when plugged directly into a USB 3.0 port.
- Moving it to USB 2.0 or a USB 2.0 extension cable stabilized the connection.
- If wired USB is smooth but BLE feels unstable, inspect host-side Bluetooth,
  USB port, extension cable, and Windows power management before changing
  firmware.

## Keymap Design Notes

When rethinking the layout:

- Avoid putting layer-tap or mod-tap behavior on ordinary alpha keys such as
  `A/S/D`. It can break normal Japanese romaji sequences such as `SA`.
- Prefer thumbs, outer columns, Cygnus extra keys, and layer-only keys for
  layer switching.
- Add missing high-frequency keys deliberately, especially function keys used
  for Japanese IME conversion such as `F6` to `F10`.
- Preserve user-tested AML gestures before adding new pointer behavior.
