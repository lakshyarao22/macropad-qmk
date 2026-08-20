#!/usr/bin/env python3
"""
Sets the macropad's stored password over Raw HID, after the firmware
has already been flashed. The password is typed at the prompt below
and sent directly to the keyboard - it is never written to disk and
never touches this repo.

Requires: pip install hidapi

Usage:
    python3 set_password.py
"""

import sys

try:
    import hid
except ImportError:
    sys.exit("Missing dependency. Install it with: pip install hidapi")

# --- Fill these in for your specific keyboard ---------------------------
# Find VID/PID in your keyboard's info.json (usb.vid / usb.pid), or run
# this script with --list to see connected HID devices and their IDs.
VENDOR_ID = 0x1234   # <-- replace with your keyboard's actual VID
PRODUCT_ID = 0x5678  # <-- replace with your keyboard's actual PID
# --------------------------------------------------------------------------

USAGE_PAGE = 0xFF60  # QMK's default Raw HID usage page
USAGE = 0x61         # QMK's default Raw HID usage
REPORT_LENGTH = 32   # must match RAW_EPSIZE in the firmware (default 32)

CMD_SET_PASSWORD = 0x01


def list_devices():
    for d in hid.enumerate():
        print(f"VID={d['vendor_id']:#06x} PID={d['product_id']:#06x} "
              f"usage_page={d.get('usage_page'):#06x} usage={d.get('usage'):#04x} "
              f"product={d.get('product_string')}")


def find_raw_hid_path():
    for d in hid.enumerate(VENDOR_ID, PRODUCT_ID):
        if d.get("usage_page") == USAGE_PAGE and d.get("usage") == USAGE:
            return d["path"]
    raise RuntimeError(
        "Raw HID interface not found. Check VENDOR_ID/PRODUCT_ID above, "
        "confirm RAW_ENABLE = yes in rules.mk, and that the firmware was "
        "reflashed after enabling it."
    )


def main():
    if "--list" in sys.argv:
        list_devices()
        return

    password = input("Enter password to store on the keyboard: ")
    if not password:
        sys.exit("Empty password, nothing sent.")
    if len(password) > 30:  # cmd byte + len byte leave 30 bytes for payload in a 32-byte report
        sys.exit("Password too long (max 30 characters).")

    path = find_raw_hid_path()
    device = hid.device()
    device.open_path(path)

    payload = bytes([CMD_SET_PASSWORD, len(password)]) + password.encode("ascii")
    report = bytes([0]) + payload + bytes(REPORT_LENGTH - len(payload))  # report ID 0 + zero-padded

    device.write(report)
    response = device.read(REPORT_LENGTH, timeout_ms=1000)

    if response and response[0] == CMD_SET_PASSWORD and response[1] == 1:
        print("Password stored on keyboard successfully.")
    else:
        print("No confirmation received - check the connection and try again.")

    device.close()


if __name__ == "__main__":
    main()