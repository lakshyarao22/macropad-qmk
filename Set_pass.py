#!/usr/bin/env python3

"""
Set one of the seven password slots on the QMK macropad.

Passwords are entered interactively and are NOT written to disk.

Usage:

    python3 set_password.py

You can also list HID devices:

    python3 set_password.py --list

Slots:

    1 = PASS1
    2 = PASS2
    3 = PASS3
    4 = PASS4
    5 = PASS5
    6 = PASS6
    7 = PASS7

Requires:

    pip install hidapi
"""

import sys

try:
    import hid
except ImportError:
    sys.exit(
        "Missing dependency.\n"
        "Install it with:\n\n"
        "    pip install hidapi"
    )


# =========================================================
# Keyboard USB identification
# =========================================================

VENDOR_ID = 0xFEED
PRODUCT_ID = 0x0000


# =========================================================
# QMK Raw HID
# =========================================================

USAGE_PAGE = 0xFF60
USAGE = 0x61

REPORT_LENGTH = 32

CMD_SET_PASSWORD = 0x01

PASS_SLOT_COUNT = 7

# 32 byte HID report:
#
# byte 0 = report ID
# byte 1 = command
# byte 2 = slot
# byte 3 = password length
#
# Therefore:
#
# 32 - 4 = 28 bytes would seem available if using
# a report-ID byte separately.
#
# However hidapi/QMK handling varies by platform.
# We use 29 characters because QMK's receive buffer
# itself has 32 bytes and the command packet needs
# three bytes inside it.
#
# The firmware safely truncates anything longer.
#
MAX_PASSWORD_LENGTH = 29


# =========================================================
# List HID devices
# =========================================================

def list_devices():

    devices = hid.enumerate()

    if not devices:
        print("No HID devices found.")
        return

    for d in devices:

        usage_page = d.get("usage_page")
        usage = d.get("usage")

        if usage_page is None:
            usage_page_text = "N/A"
        else:
            usage_page_text = f"{usage_page:#06x}"

        if usage is None:
            usage_text = "N/A"
        else:
            usage_text = f"{usage:#04x}"

        print(
            f"VID={d['vendor_id']:#06x} "
            f"PID={d['product_id']:#06x} "
            f"usage_page={usage_page_text} "
            f"usage={usage_text} "
            f"product={d.get('product_string')}"
        )


# =========================================================
# Find QMK Raw HID interface
# =========================================================

def find_raw_hid_path():

    devices = hid.enumerate(
        VENDOR_ID,
        PRODUCT_ID
    )

    for d in devices:

        if (
            d.get("usage_page") == USAGE_PAGE
            and
            d.get("usage") == USAGE
        ):

            return d["path"]

    raise RuntimeError(
        "\nRaw HID interface not found.\n\n"
        "Expected:\n"
        f"  VID        = {VENDOR_ID:#06x}\n"
        f"  PID        = {PRODUCT_ID:#06x}\n"
        f"  usage_page = {USAGE_PAGE:#06x}\n"
        f"  usage      = {USAGE:#04x}\n\n"
        "Make sure:\n"
        "  1. The macropad is connected.\n"
        "  2. RAW is enabled in keyboard.json.\n"
        "  3. The new firmware has been flashed.\n"
    )


# =========================================================
# Select password slot
# =========================================================

def get_slot():

    while True:

        try:
            slot = int(
                input(
                    "Password slot (1-7): "
                )
            )

        except ValueError:
            print("Please enter a number from 1 to 7.")
            continue

        if 1 <= slot <= PASS_SLOT_COUNT:
            return slot - 1

        print("Please enter a number from 1 to 7.")


# =========================================================
# Main
# =========================================================

def main():

    if "--list" in sys.argv:

        list_devices()
        return


    print()
    print("==============================")
    print(" QMK Macropad Password Setup")
    print("==============================")
    print()


    slot = get_slot()


    print()
    print(
        f"Setting password slot {slot + 1}"
    )
    print()


    password = input(
        "Enter password: "
    )


    if not password:

        sys.exit(
            "Empty password. Nothing was sent."
        )


    if len(password) > MAX_PASSWORD_LENGTH:

        sys.exit(
            f"Password is too long.\n"
            f"Maximum length: {MAX_PASSWORD_LENGTH} characters."
        )


    # =====================================================
    # ASCII check
    #
    # QMK send_string() expects the password to be
    # represented using the keyboard's keycode system.
    # =====================================================

    try:

        password_bytes = password.encode("ascii")

    except UnicodeEncodeError:

        sys.exit(
            "Password contains non-ASCII characters.\n"
            "Please use ASCII characters only."
        )


    # =====================================================
    # Find Raw HID interface
    # =====================================================

    try:

        path = find_raw_hid_path()

    except RuntimeError as error:

        sys.exit(str(error))


    # =====================================================
    # Open keyboard
    # =====================================================

    device = hid.device()

    try:

        device.open_path(path)

        # =================================================
        # Packet:
        #
        # [command]
        # [slot]
        # [length]
        # [password bytes]
        # =================================================

        payload = (
            bytes([
                CMD_SET_PASSWORD,
                slot,
                len(password_bytes)
            ])
            +
            password_bytes
        )


        # HID report:
        #
        # First byte = report ID 0
        #
        report = (
            bytes([0])
            +
            payload
            +
            bytes(
                max(
                    0,
                    REPORT_LENGTH - len(payload)
                )
            )
        )


        # Make absolutely sure the report is the expected
        # size.

        report = report[:REPORT_LENGTH]


        print()
        print(
            f"Sending password to slot {slot + 1}..."
        )


        device.write(report)


        # =================================================
        # Wait for acknowledgement
        # =================================================

        response = device.read(
            REPORT_LENGTH,
            timeout_ms=1000
        )


        if (
            response
            and
            len(response) >= 3
            and
            response[0] == CMD_SET_PASSWORD
            and
            response[1] == slot
            and
            response[2] == 1
        ):

            print()
            print(
                f"Password slot {slot + 1} "
                "stored successfully."
            )

        else:

            print()
            print(
                "No valid confirmation received."
            )

            print(
                "Check the keyboard connection "
                "and firmware."
            )


    finally:

        device.close()


# =========================================================
# Entry point
# =========================================================

if __name__ == "__main__":

    main()