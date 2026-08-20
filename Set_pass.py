#!/usr/bin/env python3

"""
Store one of seven passwords on the macropad over Raw HID.

The password is entered at the prompt and sent directly
to the keyboard. It is never written to disk.

Usage:

    python3 set_password.py

    python3 set_password.py --list
"""

import sys

try:
    import hid
except ImportError:
    sys.exit("Missing dependency. Install it with: pip install hidapi")


# ============================================================
# Keyboard USB IDs
# ============================================================

VENDOR_ID = 0xFEED
PRODUCT_ID = 0x0000


# ============================================================
# QMK Raw HID configuration
# ============================================================

USAGE_PAGE = 0xFF60
USAGE = 0x61

REPORT_LENGTH = 32

CMD_SET_PASSWORD = 0x01

PASSWORD_SLOTS = 7

# [command][slot][length] = 3 bytes
# 32-byte report leaves 29 bytes for password.
MAX_PASSWORD_LENGTH = 29


# ============================================================
# List HID devices
# ============================================================

def list_devices():

    devices = hid.enumerate()

    if not devices:
        print("No HID devices found.")
        return

    for d in devices:

        print(
            f"VID={d['vendor_id']:#06x} "
            f"PID={d['product_id']:#06x} "
            f"usage_page={d.get('usage_page', 0):#06x} "
            f"usage={d.get('usage', 0):#04x} "
            f"product={d.get('product_string')}"
        )


# ============================================================
# Find QMK Raw HID interface
# ============================================================

def find_raw_hid_path():

    for d in hid.enumerate(VENDOR_ID, PRODUCT_ID):

        if (
            d.get("usage_page") == USAGE_PAGE
            and d.get("usage") == USAGE
        ):

            return d["path"]

    raise RuntimeError(
        "Raw HID interface not found.\n\n"
        "Check VENDOR_ID/PRODUCT_ID in this script,\n"
        "make sure RAW_ENABLE = yes is in rules.mk,\n"
        "and make sure the firmware was reflashed."
    )


# ============================================================
# Ask for password slot
# ============================================================

def get_slot():

    while True:

        try:
            slot = int(
                input(
                    "Enter password slot (1-7): "
                )
            )

        except ValueError:

            print("Please enter a number from 1 to 7.")
            continue


        if 1 <= slot <= PASSWORD_SLOTS:
            return slot - 1


        print("Invalid slot. Choose 1-7.")


# ============================================================
# Main
# ============================================================

def main():

    # --------------------------------------------------------
    # Device listing
    # --------------------------------------------------------

    if "--list" in sys.argv:

        list_devices()
        return


    # --------------------------------------------------------
    # Select slot
    # --------------------------------------------------------

    slot = get_slot()


    # --------------------------------------------------------
    # Enter password
    # --------------------------------------------------------

    password = input(
        f"Enter password for slot {slot + 1}: "
    )


    if not password:

        sys.exit(
            "Empty password, nothing sent."
        )


    # --------------------------------------------------------
    # Check length
    # --------------------------------------------------------

    if len(password) > MAX_PASSWORD_LENGTH:

        sys.exit(
            f"Password too long. "
            f"Maximum is {MAX_PASSWORD_LENGTH} characters."
        )


    # --------------------------------------------------------
    # ASCII check
    # --------------------------------------------------------

    try:

        password_bytes = password.encode("ascii")

    except UnicodeEncodeError:

        sys.exit(
            "Password contains non-ASCII characters. "
            "The current firmware uses send_string(), "
            "so use ASCII characters only."
        )


    # --------------------------------------------------------
    # Find Raw HID interface
    # --------------------------------------------------------

    try:

        path = find_raw_hid_path()

    except RuntimeError as e:

        sys.exit(str(e))


    # --------------------------------------------------------
    # Open device
    # --------------------------------------------------------

    device = hid.device()

    try:

        device.open_path(path)


        # ----------------------------------------------------
        # Build Raw HID packet
        #
        # Byte 0 = command
        # Byte 1 = slot
        # Byte 2 = password length
        # Byte 3+ = password
        # ----------------------------------------------------

        payload = (
            bytes([
                CMD_SET_PASSWORD,
                slot,
                len(password_bytes)
            ])
            + password_bytes
        )


        # ----------------------------------------------------
        # Pad to 32 bytes.
        #
        # First byte is report ID = 0.
        # ----------------------------------------------------

        report = (
            bytes([0])
            + payload
            + bytes(REPORT_LENGTH - len(payload))
        )


        # ----------------------------------------------------
        # Send
        # ----------------------------------------------------

        device.write(report)


        # ----------------------------------------------------
        # Wait for acknowledgement
        # ----------------------------------------------------

        response = device.read(
            REPORT_LENGTH,
            timeout_ms=1000
        )


        # ----------------------------------------------------
        # Check acknowledgement
        #
        # Firmware sends:
        #
        # response[0] = CMD_SET_PASSWORD
        # response[1] = slot
        # response[2] = 1
        # ----------------------------------------------------

        if (
            response
            and len(response) >= 3
            and response[0] == CMD_SET_PASSWORD
            and response[1] == slot
            and response[2] == 1
        ):

            print(
                f"Password successfully stored "
                f"in slot {slot + 1}."
            )

        else:

            print(
                "No confirmation received from the macropad."
            )

            print(
                "Check the connection and firmware."
            )


    finally:

        device.close()


# ============================================================
# Entry point
# ============================================================

if __name__ == "__main__":
    main()