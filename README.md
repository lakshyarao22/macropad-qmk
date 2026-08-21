# QMK Macropad

![Macropad in its full glory](https://github.com/user-attachments/assets/29cee50d-492b-4ba1-8c7c-517854c629b7)


A versatile 3x3 macro keyboard powered by the RP2040 microcontroller, featuring customizable keymaps and programmable layers for productivity and entertainment.

## Features

- **RP2040 Processor**: Fast, reliable microcontroller with plenty of flash storage
- **9-Key Layout**: 3x3 grid of mechanical switches
- **Multiple Layers**: Access different functions through layer switching
- **RGB Lighting** (v2): 3 addressable WS2812 RGB LEDs with multiple animation modes
- **Full QMK Support**: Comprehensive customization via QMK firmware
- **Bootmagic**: Hold the top-left key while plugging in to enter bootloader mode

## Versions

### v1
- Base model with standard features
- Bootmagic, extrakey, and mousekey support
- No RGB lighting

### v2
- Enhanced version with **RGB lighting**
- 3 WS2812 addressable LEDs with animations:
  - Rainbow mood, rainbow swirl, breathing, knight rider, snake, twinkle, and more
- Improved matrix configuration
- Full NKRO support

## Default Keymap

### Layer 0 (Cyberpunk Cyan)
```
Meh+1     Meh+2     Meh+3
Meh+4     Meh+5     Meh+6
Prev      Base      Next
```

### Layer 1 (Vaporwave Magenta)
```
Vol-      Mute      Vol+
Prev      Play      Next
Prev      Base      Next
```

The volume keys send volume down/up when tapped and previous/next track
when held for 400 ms. The bottom-center key always returns to layer 0;
the bottom-right key advances to the next layer.

### Layer 2 (Amber Gold)
```
F13       F14       F15
F16       F17       F18
Prev      Base      Next
```

Each password key types the password stored in one of six EEPROM slots and then
presses Enter. Passwords are limited to 31 ASCII characters and are stored
unencrypted in the keyboard's EEPROM.

### Layer 3 (Crimson Red)

```
Pass1     Pass2     Pass3
Pass4     Pass5     Pass6
Prev      Base      Next
```

### Layers 4-7

Layers 4 through 6 are currently unassigned and all keys are `KC_NO`.

Layer 7 provides `Meh+G` through `Meh+L` on its upper six keys.

| Layer | Color |
|-------|-------|
| 4 | Acid Green `(50, 255, 20)` |
| 5 | Deep Violet `(120, 0, 255)` |
| 6 | Warm White `(255, 180, 100)` |
| 7 | Ice Blue `(40, 180, 255)` |

## Building & Flashing

### Prerequisites
Ensure you have QMK firmware set up. See [QMK Setup Guide](https://docs.qmk.fm/#/getting_started_build_tools).

### Build
```bash
make macropad:default
```

### Flash
```bash
make macropad:default:flash
```

When prompted, enter bootloader mode by:
- **Bootmagic reset**: Hold the top-left key (matrix position 0,0) and plug in the keyboard
- **Physical reset**: Press the reset button on the PCB
- **Keycode**: Press the key mapped to `QK_BOOT`

## Automated Builds & Releases

This repository uses **GitHub Actions** to automatically build the firmware and generate UF2 releases on every commit.

### How It Works

- **Trigger**: Every push to `main`, `master`, or `develop` branches
- **Build**: Compiles QMK firmware for RP2040 using the official QMK container
- **Output**: Generates a `.uf2` file ready for flashing
- **Release**: Automatically creates a GitHub Release with the UF2 file attached
  - Tagged releases (e.g., `v1.0.0`) are marked as stable releases
  - Main branch commits are marked as pre-release builds

### Downloading Pre-built Firmware

1. Go to the [Releases](../../releases) page
2. Download the latest `macropad_v2_default.uf2` file
3. Flash to your macropad using one of the bootloader methods above

### Build Artifacts

All successful builds create artifacts that can be downloaded from the **Actions** tab for 30 days.

## Hardware

| Component | Specification |
|-----------|---------------|
| Processor | RP2040 |
| Bootloader | RP2040 |
| Switch Matrix | 3x3 Direct Pin |
| USB | VID: 0xFEED, PID: 0x0000 |
| Firmware | QMK |

### Pinout (v1)
```
Row 0: GPIO 15, 14, 6
Row 1: GPIO 11, 12, 5
Row 2: GPIO 27, 10, 4
```

### Pinout (v2)
```
Row 0: GPIO 27, 14, 6
Row 1: GPIO 15, 9, 5
Row 2: GPIO 26, 8, 4
RGB LED: GPIO 13
```

## Customization

Edit `keymaps/default/keymap.c` to customize your key bindings. Each layer can have different key assignments to suit your workflow.

### Provisioning Passwords

Use the included `Set_pass.py` script to store a password in one of the six
slots without writing it to disk:

```bash
pip install hidapi
python3 Set_pass.py
```

The script communicates with the keyboard through its Raw HID interface. Use
`python3 Set_pass.py --list` to inspect available HID devices if the interface
cannot be found.

### Adding Custom Keycodes
The default keymap includes a `PASS` custom keycode that can be expanded for your own macros:

```c
enum custom_keycodes {
    PASS = SAFE_RANGE,
    // Add your custom codes here
};
```

## Bootloader Information

The macropad uses the RP2040 bootloader. Enter bootloader mode in 3 ways:

1. **Bootmagic reset**: Hold down the key at matrix position (0,0) and plug in the keyboard
2. **Physical reset button**: Press the reset button on the PCB
3. **Keycode in layout**: Press `QK_BOOT` if mapped in your keymap

## Documentation

- [QMK Documentation](https://docs.qmk.fm/)
- [QMK Getting Started](https://docs.qmk.fm/#/getting_started_build_tools)
- [Make Instructions](https://docs.qmk.fm/#/getting_started_make_guide)

## Maintainer

**Lakshya Rao**

## License

Firmware released under the GNU Public License v2 or later (GPL v2+).
