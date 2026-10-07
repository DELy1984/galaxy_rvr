"""Drive GalaxyRVR with DualSense L2/R2 triggers through the PC."""

from __future__ import annotations

import argparse
import asyncio
import sys

try:
    import pygame
    import websockets
except ImportError as error:
    print(
        "Missing dependency. Install project requirements with:\n"
        "  python -m pip install -r requirements.txt",
        file=sys.stderr,
    )
    raise SystemExit(1) from error


LEFT_TRIGGER_AXIS = 4
RIGHT_TRIGGER_AXIS = 5
SEND_INTERVAL_SECONDS = 0.05
TRIGGER_DEADZONE = 0.02
DEFAULT_ROVER_URI = "ws://192.168.4.1:30102/"


def trigger_power(axis_value: float, max_power: int) -> int:
    """Convert a trigger axis (-1 released, +1 pressed) to forward power."""
    normalized = min(1.0, max(0.0, (axis_value + 1.0) / 2.0))
    if normalized <= TRIGGER_DEADZONE:
        return 0
    return round(
        (normalized - TRIGGER_DEADZONE)
        / (1.0 - TRIGGER_DEADZONE)
        * max_power
    )


def motor_packet(left_power: int, right_power: int) -> bytes:
    """Build a SunFounder binary motor-control frame for forward-only power."""
    if not 0 <= left_power <= 100 or not 0 <= right_power <= 100:
        raise ValueError("Forward motor power must be between 0 and 100.")

    payload = bytes((0x01, left_power, right_power))
    checksum = 0
    for value in payload:
        checksum ^= value
    return bytes((0xA0, len(payload), checksum)) + payload + bytes((0xA1,))


def select_controller(device_index: int | None) -> pygame.joystick.JoystickType:
    pygame.joystick.init()
    devices = [
        pygame.joystick.Joystick(index)
        for index in range(pygame.joystick.get_count())
    ]
    if not devices:
        raise RuntimeError("No controller detected. Connect the DualSense first.")

    for index, device in enumerate(devices):
        print(f"{index}: {device.get_name()}")

    if device_index is None:
        matches = [
            index
            for index, device in enumerate(devices)
            if "dualsense" in device.get_name().lower()
            or "wireless controller" in device.get_name().lower()
        ]
        if len(matches) == 1:
            device_index = matches[0]
        elif len(devices) == 1:
            device_index = 0
        else:
            raise RuntimeError(
                "Select the DualSense with --device and one of the listed indexes."
            )

    if device_index < 0 or device_index >= len(devices):
        raise RuntimeError(f"Controller index {device_index} does not exist.")

    controller = devices[device_index]
    controller.init()
    if controller.get_numaxes() <= RIGHT_TRIGGER_AXIS:
        raise RuntimeError(
            "The selected controller does not expose expected L2/R2 axes A5/A6."
        )
    print(
        f"Using {controller.get_name()} "
        f"({controller.get_numaxes()} axes, {controller.get_numbuttons()} buttons)"
    )
    return controller


async def consume_rover_messages(
    socket: websockets.ClientConnection,
) -> None:
    async for message in socket:
        if isinstance(message, str):
            print(f"Rover: {message}")
        else:
            print(f"Rover binary telemetry: {message.hex(' ')}")


async def send_stop(socket: websockets.ClientConnection) -> None:
    if socket.state is websockets.protocol.State.OPEN:
        await socket.send(motor_packet(0, 0))


async def drive(args: argparse.Namespace) -> None:
    pygame.init()
    controller = select_controller(args.device)

    print(f"Connecting to rover at {args.uri}")
    async with websockets.connect(args.uri, ping_interval=None) as socket:
        receiver: asyncio.Task[None] = asyncio.create_task(
            consume_rover_messages(socket)
        )
        print(
            f"Driving enabled; maximum forward power: {args.max_power}%. "
            "L2=left track, R2=right track. Release both triggers to arm; "
            "Ctrl+C to stop."
        )

        try:
            while True:
                pygame.event.pump()
                if not controller.get_init():
                    raise RuntimeError("The controller was disconnected.")
                await socket.send(motor_packet(0, 0))
                triggers_released = (
                    controller.get_axis(LEFT_TRIGGER_AXIS) <= -0.95
                    and controller.get_axis(RIGHT_TRIGGER_AXIS) <= -0.95
                )
                if triggers_released:
                    break
                print(
                    "\rRelease both triggers to arm; motors are held at zero. ",
                    end="",
                    flush=True,
                )
                await asyncio.sleep(SEND_INTERVAL_SECONDS)
            print("\nArmed.")

            while True:
                pygame.event.pump()
                if not controller.get_init():
                    raise RuntimeError("The controller was disconnected.")

                left_power = trigger_power(
                    controller.get_axis(LEFT_TRIGGER_AXIS), args.max_power
                )
                right_power = trigger_power(
                    controller.get_axis(RIGHT_TRIGGER_AXIS), args.max_power
                )
                await socket.send(motor_packet(left_power, right_power))
                print(
                    f"\rL2/left={left_power:3d}%  "
                    f"R2/right={right_power:3d}%   ",
                    end="",
                    flush=True,
                )
                await asyncio.sleep(SEND_INTERVAL_SECONDS)
        finally:
            print("\nSending stop.")
            try:
                await asyncio.wait_for(send_stop(socket), timeout=1.0)
            except (TimeoutError, websockets.ConnectionClosed):
                print(
                    "Stop packet could not be confirmed over the network; "
                    "the rover firmware timeout may take about 3 seconds.",
                    file=sys.stderr,
                )
            receiver.cancel()
            try:
                await receiver
            except asyncio.CancelledError:
                pass
    pygame.quit()


def confirm_drive(max_power: int) -> bool:
    print(
        f"WARNING: this connects to the rover and enables motors up to "
        f"{max_power}% power. Test with the wheels lifted and keep clear."
    )
    try:
        return input("Type DRIVE to continue: ").strip() == "DRIVE"
    except EOFError:
        return False


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--device", type=int, help="zero-based gamepad index")
    parser.add_argument("--uri", default=DEFAULT_ROVER_URI, help="Rover WebSocket URI")
    parser.add_argument(
        "--max-power",
        type=int,
        default=100,
        choices=range(1, 101),
        metavar="1..100",
        help="maximum forward power percentage (default: 100)",
    )
    args = parser.parse_args()

    if not confirm_drive(args.max_power):
        print("Drive test cancelled; no rover connection was opened.")
        return 0

    try:
        asyncio.run(drive(args))
    except KeyboardInterrupt:
        print("\nDrive test interrupted.")
    except (OSError, RuntimeError, websockets.WebSocketException) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1
    finally:
        pygame.quit()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
