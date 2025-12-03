# macropad-qmk

> QMK Firmware for my 3x3 macropad. with Underglow support (WS2812B)

## Overview

This repository contains the custom [QMK Firmware](https://qmk.fm/) for a 3x3 macropad designed and built by me, featuring WS2812B addressable RGB underglow support.

- **Board:** Custom 3x3 macropad (9 keys)
- **Firmware:** Based on [QMK](https://github.com/qmk/qmk_firmware)
- **Features:**  
  - Fully customizable layouts  
  - RGB underglow (WS2812B)  
  - Multi-layer support  

## Features

- **Custom 9-key Layout:** Adapted for productivity and shortcuts.
- **WS2812B Underglow:** Customizable RGB lighting patterns and effects.
- **Easy Keymap Customization:** Modify your macropad keymap to suit your workflow.
- **QMK Advantage:** Benefit from the rich QMK ecosystem and features.

## Hardware

- **MCU:** RP2040-Zero
- **LEDs:** WS2812B addressable RGB LEDs
- **Switches:** Cherry MX switches (3x3 grid)
- **Case/Pcb:** Custom design

## Getting Started

### Prerequisites

- [QMK Firmware](https://github.com/qmk/qmk_firmware) and dependencies (Python, dfu-programmer, avr-gcc, etc.)
- [QMK Toolbox](https://github.com/qmk/qmk_toolbox) (optional, for flashing binaries)
- Supported macropad hardware

### Setup

1. **Clone this repository:**
   ```bash
   git clone https://github.com/lakshyarao22/macropad-qmk.git
   cd macropad-qmk
   ```

2. **Install QMK:**
   ```
   python3 -m pip install qmk
   qmk setup
   ```

3. **Customize Keymap (Optional):**
   - Edit `keymap.c` to customize your layout and macros.

4. **Build Firmware:**
   ```
   qmk compile -kb <keyboard_folder> -km <keymap_folder>
   ```

5. **Flash Firmware:**
   ```
   qmk flash -kb <keyboard_folder> -km <keymap_folder>
   ```
   Or use QMK Toolbox for a simpler graphical flashing.

### Lighting Customization

- Lighting effects are enabled and can be customized in the keymap or `config.h`.  
- See the [QMK RGB Lighting documentation](https://docs.qmk.fm/#/feature_rgblight) for more info.

## Repository Structure

```
.
├── <keyboard_folder>/
│   ├── keymaps/
│   │   └── <keymap_folder>/
│   │       ├── keymap.c
│   │       └── ...
│   ├── config.h
│   └── ...
├── rules.mk
└── ...
```

## Credits

- QMK Firmware: [https://qmk.fm/](https://qmk.fm/)
- Based on open-source QMK community contributions


---

**Enjoy your customizable 3x3 macropad!**  
Feel free to open issues or submit PRs for feature requests and improvements.
