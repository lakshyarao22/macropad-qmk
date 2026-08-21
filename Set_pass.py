#!/usr/bin/env python3

"""
QMK Macropad Password Provisioning Tool

Stores passwords in one of six EEPROM slots.

Slots:

    1 = PASS1
    2 = PASS2
    3 = PASS3
    4 = PASS4
    5 = PASS5
    6 = PASS6

Passwords are entered interactively and are NOT written to disk.

Usage:

    python3 Set_pass.py

List HID devices:

    python3 Set_pass.py --list

Requires:

    pip install hidapi
"""

import getpass
import sys

try:
    import hid
except ImportError:
    sys.exit(
        "Missing dependency.\n\n"
        "Install it with:\n\n"
        "    pip install hidapi"
    )


# =========================================================
# USB identification
# =========================================================

VENDOR_ID = 0xFEED
PRODUCT_ID = 0x0000

RAW_USAGE_PAGE = 0xFF60
RAW_USAGE = 0x61


# =========================================================
# Raw HID configuration
# =========================================================

REPORT_LENGTH = 32

CMD_SET_PASSWORD = 0x01

PASS_SLOT_COUNT = 6

PASS_MAX_LEN = 31


# =========================================================
# List HID devices
# =========================================================

def list_devices():

    devices = hid.enumerate()

    if not devices:
        print("No HID devices found.")
        return

    print()

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
            d.get("usage_page") == RAW_USAGE_PAGE
            and
            d.get("usage") == RAW_USAGE
        ):

            return d["path"]
    raise RuntimeError(
        "\nRaw HID interface not found.\n\n"
        "Expected:\n"
        f"  VID        = {VENDOR_ID:#06x}\n"
        f"  PID        = {PRODUCT_ID:#06x}\n"
        f"  usage_page = {RAW_USAGE_PAGE:#06x}\n"
        f"  usage      = {RAW_USAGE:#04x}\n\n"
        "Run:\n"
        "    python3 Set_pass.py --list\n\n"
        "and check that the macropad Raw HID interface "
        "appears."
    )


# =========================================================
# Select password slot
# =========================================================

def get_slot():

    while True:

        try:

            slot = int(
                input(f"Password slot (1-{PASS_SLOT_COUNT}): ")
            )

        except ValueError:

            print(
                f"Please enter a number from 1 to {PASS_SLOT_COUNT}."
            )

            continue

        if 1 <= slot <= PASS_SLOT_COUNT:

            return slot - 1

        print(
            f"Please enter a number from 1 to {PASS_SLOT_COUNT}."
        )


# =========================================================
# Read password
# =========================================================

def get_password():

    password = getpass.getpass(
        "Enter password: "
    )


    if not password:

        sys.exit(
            "Empty password. Nothing was sent."
        )


    if len(password) > PASS_MAX_LEN:

        sys.exit(
            f"Password is too long.\n"
            f"Maximum length: {PASS_MAX_LEN} characters."
        )


    try:

        password_bytes = password.encode(
            "ascii"
        )

    except UnicodeEncodeError:

        sys.exit(
            "Password contains non-ASCII characters.\n"
            "Please use ASCII characters only."
        )


    return password_bytes


# =========================================================
# Build Raw HID packet
# =========================================================

def build_packet(slot, password_bytes):

    payload = (
        bytes([
            CMD_SET_PASSWORD,
            slot,
            len(password_bytes)
        ])
        +
        password_bytes
    )


    if len(payload) > REPORT_LENGTH:

        raise RuntimeError(
            "Password packet is too large."
        )


    # hidapi on macOS expects the report ID as the
    # first byte of the report.
    #
    # QMK receives the remaining 32-byte Raw HID
    # payload as data[].
        password_bytes = get_password()
    report = (
        bytes([0])
        +
        payload
        +
        bytes(
            REPORT_LENGTH - len(payload)
        )
    )


    return report


# =========================================================
# Validate acknowledgement
# =========================================================

def check_response(
    response,
    slot
):

    if not response:

        return False


    # Convert to normal Python bytes.

    response = bytes(response)


    # -----------------------------------------------------
    # Case 1:
    #
    # hidapi returns:
    #
    # [command, slot, ACK, ...]
    # -----------------------------------------------------

    if len(response) >= 3:

        if (
            response[0] == CMD_SET_PASSWORD
            and
            response[1] == slot
            and
            response[2] == 1
        ):

            return True


    # -----------------------------------------------------
    # Case 2:
    #
    # Some HID backends return:
    #
    # [report_id, command, slot, ACK, ...]
    # -----------------------------------------------------

    if len(response) >= 4:

        if (
            response[0] == 0
            and
            response[1] == CMD_SET_PASSWORD
            and
            response[2] == slot
            and
            response[3] == 1
        ):

            return True


    return False


# =========================================================
# Main
# =========================================================

def main():

    # -----------------------------------------------------
    # Device listing
    # -----------------------------------------------------

    if "--list" in sys.argv:

        list_devices()

        return


    print()

    print("==============================")
    print(" QMK Macropad Password Setup")
    print("==============================")

    print()


    # -----------------------------------------------------
    # Select slot
    # -----------------------------------------------------

    slot = get_slot()


    print()

    print(
        f"Setting password slot {slot + 1}"
    )

    print()


    # -----------------------------------------------------
    # Get password
    # -----------------------------------------------------

    password_bytes = get_password()


    # -----------------------------------------------------
    # Find Raw HID
    # -----------------------------------------------------

    try:

        path = find_raw_hid_path()

    except RuntimeError as error:

        sys.exit(str(error))


    # -----------------------------------------------------
    # Open device
    # -----------------------------------------------------

    device = hid.device()


    try:

        device.open_path(path)


        # -------------------------------------------------
        # Build packet
        # -------------------------------------------------

        report = build_packet(
            slot,
            password_bytes
        )


        print()

        print(
            f"Sending password to slot "
            f"{slot + 1}..."
        )


        # -------------------------------------------------
        # Send packet
        # -------------------------------------------------

        bytes_written = device.write(
            report
        )


        if bytes_written <= 0:

            print()

            print(
                "Failed to send Raw HID packet."
            )

            return


        # -------------------------------------------------
        # Wait for acknowledgement
        # -------------------------------------------------

        response = device.read(
            REPORT_LENGTH,
            timeout_ms=2000
        )


        # -------------------------------------------------
        # Validate response
        # -------------------------------------------------

        if check_response(
            response,
            slot
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


            if response:

                print()

                print(
                    "Raw response:"
                )

                print(
                    " ".join(
                        f"{b:02X}"
                        for b in response
                    )
                )


            else:

                print(
                    "The keyboard did not return "
                    "an acknowledgement."
                )


            print()

            print(
                "Check the keyboard firmware "
                "and Raw HID interface."
            )


    finally:

        device.close()


# =========================================================
# Entry point
# =========================================================

if __name__ == "__main__":

    main()