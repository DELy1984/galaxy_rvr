"""Read and display DualSense inputs without sending any rover commands."""

from __future__ import annotations

import argparse
import sys
import time

try:
    import pygame
except ImportError:
    print(
        "Missing dependency. Install project requirements with:\n"
        "  python -m pip install -r requirements.txt",
        file=sys.stderr,
    )
    raise SystemExit(1) from None


def select_device(requested_index: int | None) -> pygame.joystick.JoystickType:
    pygame.joystick.init()
    devices = [
        pygame.joystick.Joystick(index)
        for index in range(pygame.joystick.get_count())
    ]

    if not devices:
        raise RuntimeError(
            "No game controller detected. Connect the DualSense and try again."
        )

    for index, device in enumerate(devices):
        print(f"{index}: {device.get_name()}")

    if requested_index is not None:
        if requested_index < 0 or requested_index >= len(devices):
            raise RuntimeError(f"Controller index {requested_index} does not exist.")
        selected = devices[requested_index]
    else:
        selected = next(
            (
                device
                for device in devices
                if "dualsense" in device.get_name().lower()
                or "wireless controller" in device.get_name().lower()
            ),
            None,
        )
        if selected is None and len(devices) == 1:
            selected = devices[0]
        if selected is None:
            raise RuntimeError(
                "Could not select the DualSense automatically. "
                "Run again with --device and the desired index."
            )

    selected.init()
    print(
        f"\nReading: {selected.get_name()} "
        f"({selected.get_numaxes()} axes, {selected.get_numbuttons()} buttons)"
    )
    print("Move sticks and press/release L2 and R2. Press Ctrl+C to exit.")
    print("This utility only reads inputs; it does not connect to or control the rover.")
    return selected


def describe_inputs(device: pygame.joystick.JoystickType) -> str:
    axes = ", ".join(
        f"A{index + 1}={device.get_axis(index):+.2f}"
        for index in range(device.get_numaxes())
    )
    pressed = [
        str(index + 1)
        for index in range(device.get_numbuttons())
        if device.get_button(index)
    ]
    buttons = ", ".join(pressed) if pressed else "none"
    hats = ", ".join(
        f"H{index + 1}={device.get_hat(index)}"
        for index in range(device.get_numhats())
    )
    return f"Axes: [{axes}] | Buttons (1-based): [{buttons}] | Hats: [{hats}]"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--device",
        type=int,
        help="zero-based controller index shown in the device list",
    )
    args = parser.parse_args()

    pygame.init()
    try:
        device = select_device(args.device)
        previous = ""
        while True:
            pygame.event.pump()
            if not device.get_init():
                raise RuntimeError("The controller was disconnected.")
            current = describe_inputs(device)
            if current != previous:
                print(current, flush=True)
                previous = current
            time.sleep(0.05)
    except KeyboardInterrupt:
        print("\nInput test stopped.")
    except RuntimeError as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1
    finally:
        pygame.quit()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
