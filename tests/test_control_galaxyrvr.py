import unittest

from tools.control_galaxyrvr import motor_packet, trigger_power


class TriggerPowerTests(unittest.TestCase):
    def test_released_trigger_maps_to_zero(self) -> None:
        self.assertEqual(trigger_power(-1.0, 100), 0)

    def test_half_pressed_trigger_maps_to_half_maximum(self) -> None:
        self.assertEqual(trigger_power(0.0, 100), 49)

    def test_fully_pressed_trigger_maps_to_maximum(self) -> None:
        self.assertEqual(trigger_power(1.0, 100), 100)

    def test_trigger_deadzone_maps_to_zero(self) -> None:
        self.assertEqual(trigger_power(-0.97, 100), 0)

    def test_maximum_power_caps_trigger_output(self) -> None:
        self.assertEqual(trigger_power(1.0, 10), 10)


class MotorPacketTests(unittest.TestCase):
    def test_stop_packet(self) -> None:
        self.assertEqual(
            motor_packet(0, 0),
            bytes.fromhex("A0 03 01 01 00 00 A1"),
        )

    def test_forward_packet(self) -> None:
        self.assertEqual(
            motor_packet(10, 10),
            bytes.fromhex("A0 03 01 01 0A 0A A1"),
        )

    def test_rejects_out_of_range_power(self) -> None:
        with self.assertRaises(ValueError):
            motor_packet(101, 0)


if __name__ == "__main__":
    unittest.main()
