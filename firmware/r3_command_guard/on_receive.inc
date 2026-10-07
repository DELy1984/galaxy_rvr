void enforceMotorGuard() {
  if (!aiCam.ws_connected) motorGuard.disarm();
  if (motorGuard.expire(millis())) {
    Serial.println(F("[R3] Motor command timeout; zero required"));
  }
  leftMotorPower = motorGuard.left();
  rightMotorPower = motorGuard.right();
  if (!motorGuard.armed()) carStop();
}

void onReceive() {
  enforceMotorGuard();
  const uint32_t now = millis();
  const MotorCommandGuard::Result result = motorGuard.accept(
      aiCam.recvBuffer, aiCam.recvBufferLength, now);
  if (result != MotorCommandGuard::Result::Accepted) {
    Serial.println(result == MotorCommandGuard::Result::Invalid
        ? F("[R3] Invalid motor command")
        : F("[R3] Zero command required"));
    return;
  }
  leftMotorPower = motorGuard.left();
  rightMotorPower = motorGuard.right();
  currentMode = MODE_APP_CONTROL;
}
