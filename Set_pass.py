```python
#!/usr/bin/env python3

"""
Set one of the seven password slots on the macropad.

The password is entered locally and sent directly to the keyboard
over Raw HID. It is never written to disk.

Requires:
    pip install hidapi

Usage:

    python3 set_pass.py

List HID devices:

    python3 set_pass.py --list
"""

import sys
from getpass import getpass


try:
    import hid
except ImportError:
    sys.exit(
        "Missing dependency.\n"
        "Install it with:\n"
        "    pip install hidapi"
    )


# ============================================================
# Keyboard USB IDs
# ============================================================

# Replace these with the VID/PID from your QMK keyboard.
#
# Your previous keymap.c used:
#
#   VENDOR_ID  = 0xFEED
#   PRODUCT_ID = 0x0000
#
# If you have changed these in config.h, use the new values.

VENDOR_ID = 0xFEED
PRODUCT_ID = 0x0000


# ============================================================
# QMK Raw HID configuration
# ============================================================

USAGE_PAGE = 0xFF60
USAGE = 0x61

REPORT_LENGTH = 32


# ============================================================
# Password configuration
# ============================================================

PASS_SLOT_COUNT = 7

# The Raw HID packet contains:
#
#   byte 0 = command
#   byte 1 = slot
#   byte 2 = password length
#   byte 3+ = password
#
# With a 32-byte Raw HID report:
#
#   32 - 3 = 29 bytes available for password data.
#
# Therefore the provisioning script allows 29 ASCII characters.

MAX_PASSWORD_LENGTH = 29


# ============================================================
# Commands
# ============================================================

CMD_SET_PASSWORD = 0x01


# ============================================================
# List HID devices
# ============================================================

def list_devices():

    devices = hid.enumerate()

    if not devices:
        print("No HID devices found.")
        return

    print()

    for d in devices:

        usage_page = d.get("usage_page")
        usage = d.get("usage")

        if usage_page is not None:
            usage_page_text = f"{usage_page:#06x}"
        else:
            usage_page_text = "N/A"

        if usage is not None:
            usage_text = f"{usage:#04x}"
        else:
            usage_text = "N/A"

        print(
            f"VID={d['vendor_id']:#06x} "
            f"PID={d['product_id']:#06x} "
            f"usage_page={usage_page_text} "
            f"usage={usage_text} "
            f"product={d.get('product_string')}"
        )

        print(f"  path={d.get('path')}")
        print()


# ============================================================
# Find QMK Raw HID interface
# ============================================================

def find_raw_hid_path():

    devices = hid.enumerate(
        VENDOR_ID,
        PRODUCT_ID
    )

    for d in devices:

        if (
            d.get("usage_page") == USAGE_PAGE
            and d.get("usage") == USAGE
        ):
            return d["path"]

    raise RuntimeError(
        "Raw HID interface not found.\n\n"
        "Check the following:\n"
        "  1. VENDOR_ID and PRODUCT_ID are correct.\n"
        "  2. RAW_ENABLE = yes is present in rules.mk.\n"
        "  3. The firmware was rebuilt after enabling Raw HID.\n"
        "  4. The new firmware has been flashed to the macropad.\n\n"
        "Run:\n"
        "    python3 set_pass.py --list\n"
        "to inspect connected HID devices."
    )


# ============================================================
# Ask which password slot to use
# ============================================================

def select_slot():

    print()
    print("Password slots:")
    print()

    for slot in range(1, PASS_SLOT_COUNT + 1):
        print(f"  {slot}. Password {slot}")

    print()

    while True:

        value = input(
            f"Select slot (1-{PASS_SLOT_COUNT}): "
        ).strip()

        try:
            slot = int(value)
        except ValueError:
            print("Please enter a number.")
            continue

        if 1 <= slot <= PASS_SLOT_COUNT:
            return slot - 1

        print(
            f"Please enter a number between "
            f"1 and {PASS_SLOT_COUNT}."
        )


# ============================================================
# Read password
# ============================================================

def read_password():

    password = getpass(
        "Enter password to store: "
    )

    if not password:
        raise ValueError(
            "Empty password. Nothing was sent."
        )

    try:
        password_bytes = password.encode("ascii")
    except UnicodeEncodeError:
        raise ValueError(
            "Password contains non-ASCII characters.\n"
            "Please use ASCII characters only."
        )

    if len(password_bytes) > MAX_PASSWORD_LENGTH:
        raise ValueError(
            f"Password is too long.\n"
            f"Maximum length is "
            f"{MAX_PASSWORD_LENGTH} characters."
        )

    return password_bytes


# ============================================================
# Send password to macropad
# ============================================================

def set_password(slot, password_bytes):

    path = find_raw_hid_path()

    device = hid.device()

    try:

        device.open_path(path)

        # ----------------------------------------------------
        # Raw HID payload
        #
        # byte 0 = command
        # byte 1 = slot
        # byte 2 = password length
        # byte 3+ = password
        # ----------------------------------------------------

        payload = bytes(
            [
                CMD_SET_PASSWORD,
                slot,
                len(password_bytes),
            ]
        ) + password_bytes


        # ----------------------------------------------------
        # hidapi expects the report ID as the first byte.
        #
        # Report ID = 0
        # ----------------------------------------------------

        report = (
            bytes([0])
            + payload
            + bytes(
                REPORT_LENGTH - len(payload)
            )
        )


        if len(report) != REPORT_LENGTH + 1:
            raise RuntimeError(
                f"Unexpected HID report size: "
                f"{len(report)}"
            )


        device.write(report)


        # ----------------------------------------------------
        # Wait for acknowledgement
        # ----------------------------------------------------

        response = device.read(
            REPORT_LENGTH,
            timeout_ms=1000
        )


        if (
            response
            and len(response) >= 3
            and response[0] == CMD_SET_PASSWORD
            and response[1] == slot
            and response[2] == 1
        ):

            print()
            print(
                f"Password {slot + 1} stored "
                f"successfully."
            )

        else:

            print()
            print(
                "No confirmation received from "
                "the macropad."
            )

            print(
                "Check the connection and make sure "
                "the firmware is running the updated "
                "Raw HID code."
            )

    finally:

        device.close()


# ============================================================
# Main
# ============================================================

def main():

    if "--list" in sys.argv:

        list_devices()
        return


    print()
    print("===================================")
    print("       Macropad Password Setup")
    print("===================================")


    try:

        slot = select_slot()

        print()

        password_bytes = read_password()

        print()
        print(
            f"Writing password to slot "
            f"{slot + 1}..."
        )

        set_password(
            slot,
            password_bytes
        )

    except KeyboardInterrupt:

        print()
        print("Cancelled.")

    except Exception as exc:

        sys.exit(
            f"\nError: {exc}"
        )


if __name__ == "__main__":
    main()
```
