#!/usr/bin/env python3

"""
Macropad password provisioning tool.

Stores one of seven password slots on the macropad.

Passwords are sent over Raw HID and stored in the RP2040's
EEPROM. The password itself is never written to disk by this script.

Usage:

    python3 set_password.py

List HID devices:

    python3 set_password.py --list
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


# ============================================================
# MACROPAD USB IDENTIFICATION
# ============================================================

VENDOR_ID = 0xFEED
PRODUCT_ID = 0x0000

USAGE_PAGE = 0xFF60
USAGE = 0x61

REPORT_LENGTH = 32


# ============================================================
# PASSWORD SETTINGS
# ============================================================

PASSWORD_SLOTS = 7
MAX_PASSWORD_LENGTH = 31


# ============================================================
# RAW HID COMMANDS
# ============================================================

CMD_SET_PASSWORD = 0x01


# ============================================================
# LIST HID DEVICES
# ============================================================

def list_devices():

    devices = hid.enumerate()

    if not devices:
        print("No HID devices found.")
        return

    for d in devices:

        usage_page = d.get("usage_page", 0)
        usage = d.get("usage", 0)

        print(
            f"VID={d['vendor_id']:#06x} "
            f"PID={d['product_id']:#06x} "
            f"usage_page={usage_page:#06x} "
            f"usage={usage:#04x} "
            f"product={d.get('product_string')}"
        )


# ============================================================
# FIND RAW HID INTERFACE
# ============================================================

def find_raw_hid_path():

    devices = hid.enumerate(
        VENDOR_ID,
        PRODUCT_ID
    )

    for d in devices:

        usage_page = d.get("usage_page")
        usage = d.get("usage")

        if (
            usage_page == USAGE_PAGE
            and usage == USAGE
        ):
            return d["path"]

    raise RuntimeError(
        "\nRaw HID interface not found.\n\n"
        "Expected:\n"
        f"VID        = {VENDOR_ID:#06x}\n"
        f"PID        = {PRODUCT_ID:#06x}\n"
        f"Usage Page = {USAGE_PAGE:#06x}\n"
        f"Usage      = {USAGE:#04x}\n\n"
        "Make sure:\n"
        "1. RAW_ENABLE = yes is enabled.\n"
        "2. The firmware was reflashed after enabling Raw HID.\n"
        "3. The macropad is connected normally."
    )


# ============================================================
# GET PASSWORD SLOT
# ============================================================

def get_slot():

    while True:

        value = input(
            "Password slot (1-7): "
        ).strip()

        try:
            slot = int(value)

        except ValueError:

            print(
                "Please enter a number from 1 to 7."
            )

            continue

        if 1 <= slot <= PASSWORD_SLOTS:
            return slot - 1

        print(
            "Please enter a number from 1 to 7."
        )


# ============================================================
# MAIN
# ============================================================

def main():

    if "--list" in sys.argv:

        list_devices()
        return


    print()
    print("===================================")
    print("       MACROPAD PASSWORD SETTER")
    print("===================================")
    print()


    slot = get_slot()


    print()

    password = input(
        f"Enter password for slot {slot + 1}: "
    )


    if not password:

        sys.exit(
            "Empty password. Nothing was sent."
        )


    if len(password) > MAX_PASSWORD_LENGTH:

        sys.exit(
            f"Password too long.\n"
            f"Maximum length is "
            f"{MAX_PASSWORD_LENGTH} characters."
        )


    try:

        password_bytes = password.encode("ascii")

    except UnicodeEncodeError:

        sys.exit(
            "Password contains non-ASCII characters.\n"
            "For now, please use ASCII characters only."
        )


    if len(password_bytes) > MAX_PASSWORD_LENGTH:

        sys.exit(
            f"Password is too long.\n"
            f"Maximum length is "
            f"{MAX_PASSWORD_LENGTH} bytes."
        )


    print()
    print("Looking for macropad...")


    try:

        path = find_raw_hid_path()

    except RuntimeError as error:

        sys.exit(str(error))


    device = hid.device()


    try:

        device.open_path(path)

        print("Macropad found.")
        print(
            f"Writing password to slot {slot + 1}..."
        )


        # ----------------------------------------------------
        # Packet:
        #
        # byte 0 = command
        # byte 1 = slot
        # byte 2 = password length
        # byte 3+ = password
        # ----------------------------------------------------

        payload = (
            bytes([
                CMD_SET_PASSWORD,
                slot,
                len(password_bytes)
            ])
            + password_bytes
        )


        if len(payload) > REPORT_LENGTH:

            sys.exit(
                "Internal error: HID packet is too large."
            )


        # ----------------------------------------------------
        # QMK Raw HID expects a report ID byte first.
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


        device.write(report)


        # ----------------------------------------------------
        # Wait for acknowledgement.
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
                f"SUCCESS: Password stored in slot "
                f"{slot + 1}."
            )

        else:

            print()
            print(
                "WARNING: No valid confirmation received."
            )

            print(
                "The password may not have been stored."
            )

    finally:

        device.close()


# ============================================================
# ENTRY POINT
# ============================================================

if __name__ == "__main__":
    main()