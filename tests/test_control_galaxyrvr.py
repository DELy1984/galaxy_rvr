import unittest

from tools.control_galaxyrvr import motor_packet, trigger_power, update_direction


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

    def test_reverse_packet_encodes_negative_motor_values(self) -> None:
        self.assertEqual(
            motor_packet(-10, -10),
            bytes.fromhex("A0 03 01 01 F6 F6 A1"),
        )

    def test_independent_reverse_side_packet(self) -> None:
        self.assertEqual(
            motor_packet(-10, 0),
            bytes.fromhex("A0 03 F7 01 F6 00 A1"),
        )

    def test_rejects_out_of_range_power(self) -> None:
        with self.assertRaises(ValueError):
            motor_packet(101, 0)

    def test_rejects_out_of_range_reverse_power(self) -> None:
        with self.assertRaises(ValueError):
            motor_packet(-101, 0)


class DirectionToggleTests(unittest.TestCase):
    def test_new_l1_press_toggles_direction(self) -> None:
        self.assertEqual(update_direction(False, True, False), (True, True))

    def test_held_l1_does_not_toggle_repeatedly(self) -> None:
        self.assertEqual(update_direction(True, True, True), (True, True))

    def test_releasing_l1_preserves_direction_and_rearms_toggle(self) -> None:
        self.assertEqual(update_direction(True, False, True), (True, False))


if __name__ == "__main__":
    unittest.main()
