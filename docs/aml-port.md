# AML Port Notes

This repository carries Ren's AML behavior from the torabo-tsuki ZMK port
onto Cygnus-M.

## What was ported

- `clk_or_key`: J/K/L act as normal keys in typing mode, and as mouse
  actions in pointer mode.
- `vowel_auto`: A/I/U/E/O can complete the recent J/K/L consonant for
  Japanese romaji input.
- `typing_mode_exit`: trackball motion exits typing mode.
- `scroll_mode`: drag-scroll mode driven by the `clk_or_key K 255`
  binding.
- `bt_toggle_01`: one key toggles Bluetooth profile 0 and 1.
- `os_aware`: picks a Mac or non-Mac shortcut based on the active
  Bluetooth profile.

## Cygnus key position mapping

torabo-tsuki M has 50 positions per layer:

```text
12 + 12 + 12 + 14
```

Cygnus-M has 51 positions per layer:

```text
12 + 14 + 14 + 11
```

The port keeps torabo's existing positions and only inserts Cygnus-local
extra keys where Cygnus has them:

- Row 0: torabo row copied directly.
- Row 1: torabo left 6, Cygnus extra 2, torabo right 6.
- Row 2: torabo left 6, Cygnus extra 2, torabo right 6.
- Row 3: Cygnus has fewer bottom-row keys, so the useful torabo bottom
  positions are kept and the duplicate/trailing bottom entries are
  dropped.

Base-layer Cygnus extra keys keep their local defaults:

- Row 1 extras: middle click, up arrow.
- Row 2 extras: delete, down arrow.

On higher layers, those extra positions are transparent.

## Trackball layers

Cygnus automouse and PMW3610 scroll-layers are disabled for this port,
because torabo's AML owns pointer-mode and drag-scroll behavior.

Layer 7 is reserved for PMW3610 snipe mode:

```dts
snipe-layers = <7>;
```

Sensitivity is matched to torabo-tsuki:

```conf
CONFIG_PMW3610_CPI=450
CONFIG_PMW3610_SNIPE_CPI=600
CONFIG_PMW3610_SNIPE_CPI_DIVIDOR=4
```
