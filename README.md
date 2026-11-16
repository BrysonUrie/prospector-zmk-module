# Prospector ZMK Module: pet status screen

A fork of [carrefinho/prospector-zmk-module](https://github.com/carrefinho/prospector-zmk-module)
(branch `core/zephyr-4-1`) with a reworked status screen for the
[Prospector](https://github.com/carrefinho/prospector) dongle display:

- **A pet that levels up as you type.** Every key press is EXP. It evolves at
  levels 16 and 36 and starts over after 6 hours without typing or a power cycle.
  It hops on each key press and stands on a soft shadow. It ships as a placeholder
  blob; [bring your own sprites](#custom-pet-sprites).
- **Header:** the active layer's name, plus the output (USB, or the Bluetooth profile
  number, colored by connected / disconnected / ready to pair).
- **Level and EXP bar** under the header.
- **Battery bars** for the left and right halves, colored by level (full / mid / low),
  red when a half is disconnected.
- **Tokyo Night colors**, all in one file: [`include/theme.h`](boards/shields/prospector_adapter/include/theme.h).

The modifier indicators and the layer roller from upstream are not used.

## Compatibility

This branch builds on upstream's **Zephyr 4.1** branch. Pin ZMK to a Zephyr 4.1 revision
(it was built and tested with ZMK `958fa788`) and use the matching build image.
Current ZMK `main` uses Zephyr 4.4 and will not build it.

## Installation

Your keyboard needs a dongle as the central. In `config/west.yml`:

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: brysonurie
      url-base: https://github.com/BrysonUrie
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: 958fa788067e18c2228d016fcd826000bb8f0320 # Zephyr 4.1
      import: app/west.yml
    - name: prospector-zmk-module
      remote: brysonurie
      revision: pet
  self:
    path: config
```

Add the `prospector_adapter` shield to the dongle in `build.yaml`:

```yaml
include:
  - board: xiao_ble
    shield: [YOUR KEYBOARD SHIELD]_dongle prospector_adapter
```

After flashing the dongle, pair the left half first, then the right. The battery bars
follow pairing order. If they come out swapped, set `CONFIG_PROSPECTOR_BATTERY_SWAP_SIDES=y`
instead of re-pairing.

Layer names come from each layer's `display-name` in your keymap, falling back to
the layer number:

```dts
base {
    display-name = "Base";
    bindings = < ... >;
};
```

## Custom pet sprites

The module only ships placeholder sprites. To use your own, you need three animated
GIFs, one per stage (`stage1.gif`, `stage2.gif`, `stage3.gif`), and
[Pillow](https://pypi.org/project/pillow/) on your computer:

```sh
python3 scripts/sprites.py --gifs path/to/gifs --out path/to/zmk-config/config/pet_sprites.c
```

Commit `config/pet_sprites.c` to your zmk-config, or keep it local and gitignored. When
that file exists, the build uses it instead of the placeholder. The build log shows which
one was picked (`Prospector pet sprites: ...`).

The script crops each stage to its sprite, scales it to at most 56 px (drawn at 2x),
keeps as many frames as fit in about 80 KB per stage, and pre-renders the shadow.
Sprites are blended onto the background color, so if you change `THEME_BG`, also change
`BG` at the top of `scripts/sprites.py` and regenerate.

Only use artwork you have the rights to share if your zmk-config is public.

## Configuration

Add options to your `config/[YOUR KEYBOARD SHIELD].conf`:

```ini
CONFIG_PROSPECTOR_USE_AMBIENT_LIGHT_SENSOR=n
CONFIG_PROSPECTOR_FIXED_BRIGHTNESS=80
```

| Name | Description | Default |
| --- | --- | --- |
| `CONFIG_PROSPECTOR_USE_AMBIENT_LIGHT_SENSOR` | Use the ambient light sensor for auto brightness; set to `n` if building without one | y |
| `CONFIG_PROSPECTOR_FIXED_BRIGHTNESS` | Display brightness when not using the light sensor | 50 (1-100) |
| `CONFIG_PROSPECTOR_ROTATE_DISPLAY_180` | Rotate the display 180 degrees | n |
| `CONFIG_PROSPECTOR_BATTERY_SWAP_SIDES` | Show the first-paired half's battery on the right, labelled R | n |

Colors, battery thresholds and screen positions are `#define`s in
[`include/theme.h`](boards/shields/prospector_adapter/include/theme.h),
[`src/custom_status_screen.c`](boards/shields/prospector_adapter/src/custom_status_screen.c)
and [`src/widgets/pet.h`](boards/shields/prospector_adapter/src/widgets/pet.h)
(leveling curve, evolution levels, idle reset, hop).

## Credits

Based on [prospector-zmk-module](https://github.com/carrefinho/prospector-zmk-module)
by carrefinho, MIT licensed (see [LICENSE](LICENSE)).
