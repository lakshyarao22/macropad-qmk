#!/usr/bin/env python3

import sys
import hid


# ============================================================
# Macropad USB / Raw HID configuration
# ============================================================

VENDOR_ID = 0xFEED
PRODUCT_ID = 0x0000

USAGE_PAGE = 0xFF60
USAGE = 0x61

REPORT_LENGTH = 32

CMD_SET_PASSWORD = 0x01

MAX_PASSWORD_LENGTH = 31


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

def find_raw_hid():

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

            return d

    return None


# ============================================================
# Send password
# ============================================================

def set_password(slot, password):

    if slot < 1 or slot > 7:

        print("Slot must be between 1 and 7.")
        return False


    if not password:

        print("Password cannot be empty.")
        return False


    encoded = password.encode("ascii")


    if len(encoded) > MAX_PASSWORD_LENGTH:

        print(
            f"Password too long. "
            f"Maximum is {MAX_PASSWORD_LENGTH} ASCII characters."
        )

        return False


    interface = find_raw_hid()


    if interface is None:

        print()
        print("Raw HID interface not found.")
        print()
        print(
            "Expected:"
        )
        print(
            f"VID=0x{VENDOR_ID:04X} "
            f"PID=0x{PRODUCT_ID:04X} "
            f"usage_page=0x{USAGE_PAGE:04X} "
            f"usage=0x{USAGE:02X}"
        )

        return False


    print()
    print(
        f"Using Raw HID interface: "
        f"VID=0x{interface['vendor_id']:04X} "
        f"PID=0x{interface['product_id']:04X}"
    )

    print(
        f"Usage Page: 0x{interface['usage_page']:04X}"
    )

    print(
        f"Usage: 0x{interface['usage']:02X}"
    )


    device = hid.device()


    try:

        device.open_path(interface["path"])


        # ====================================================
        # QMK Raw HID packet
        #
        # byte 0 = command
        # byte 1 = slot
        # byte 2 = password length
        # byte 3+ = password
        #
        # HID report itself has a leading Report ID byte = 0
        #
        # Total report sent to hidapi:
        #   1 byte report ID
        #   32 bytes Raw HID payload
        # ====================================================

        payload = (
            bytes([
                CMD_SET_PASSWORD,
                slot - 1,
                len(encoded)
            ])
            + encoded
        )


        payload += bytes(
            REPORT_LENGTH - len(payload)
        )


        report = bytes([0]) + payload


        print()
        print(
            f"Sending password to slot {slot}..."
        )


        device.write(report)


        # ====================================================
        # Wait for QMK acknowledgement
        # ====================================================

        response = device.read(
            REPORT_LENGTH,
            timeout_ms=1500
        )


        print(
            f"Raw response: {response}"
        )


        if not response:

            print()
            print(
                "No response received from the macropad."
            )

            print(
                "Check that RAW_ENABLE = yes is present "
                "in rules.mk and that the firmware was reflashed."
            )

            return False


        # ----------------------------------------------------
        # Depending on the HID backend, the returned data may
        # or may not include a Report ID byte.
        #
        # Therefore support both:
        #
        #   [CMD, SLOT, ACK, ...]
        #
        # and
        #
        #   [REPORT_ID, CMD, SLOT, ACK, ...]
        # ----------------------------------------------------

        if len(response) >= 3:

            # Normal QMK Raw HID response
            if (
                response[0] == CMD_SET_PASSWORD
                and
                response[1] == slot - 1
                and
                response[2] == 1
            ):

                print()
                print(
                    f"Password stored successfully in slot {slot}."
                )

                return True


        if len(response) >= 4:

            # Response containing Report ID
            if (
                response[0] == 0
                and
                response[1] == CMD_SET_PASSWORD
                and
                response[2] == slot - 1
                and
                response[3] == 1
            ):

                print()
                print(
                    f"Password stored successfully in slot {slot}."
                )

                return True


        print()
        print(
            "Invalid confirmation received."
        )

        print(
            "The macropad responded, but the response "
            "did not match the expected acknowledgement."
        )

        return False


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
    print("======================================")
    print("       Macropad Password Manager")
    print("======================================")
    print()


    try:

        slot = int(
            input(
                "Password slot (1-7): "
            )
        )

    except ValueError:

        print("Invalid slot.")
        return


    if slot < 1 or slot > 7:

        print("Slot must be between 1 and 7.")
        return


    # Use getpass so the password isn't displayed
    # while typing.

    try:

        from getpass import getpass

        password = getpass(
            f"Enter password for slot {slot}: "
        )

    except Exception:

        password = input(
            f"Enter password for slot {slot}: "
        )


    if set_password(slot, password):

        print()
        print("Done.")

    else:

        print()
        print("Password was NOT confirmed as stored.")


if __name__ == "__main__":

    main()