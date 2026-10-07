#pragma once

#include <stdint.h>

class DriveControl {
 public:
  static constexpr uint32_t INPUT_TIMEOUT_MS = 250;
  static constexpr int32_t TRIGGER_MAX = 1020;
  static constexpr int32_t DEADZONE = 20;
  static constexpr int8_t MAX_POWER = 100;
  static constexpr int32_t STICK_DEADZONE = 40;

  void stop(const char* reason) {
    armed_ = false;
    reverse_ = false;
    previousL1_ = false;
    left_ = right_ = 0;
    stickActive_ = false;
    reason_ = reason;
  }

  void check(uint32_t now, bool connected, bool r3Ready, bool locked) {
    if (locked) stop("Locked until reboot");
    else if (!r3Ready) stop("R3 not ready or stale");
    else if (!connected) stop("DualSense disconnected");
    else if (!haveInput_ || now - lastInput_ >= INPUT_TIMEOUT_MS) {
      stop("Controller input stale");
    }
  }

  bool input(uint32_t now, int32_t brake, int32_t throttle, bool l1,
             int32_t stickX = 0, int32_t stickY = 0) {
    if (brake < 0 || throttle < 0 || brake > 1023 || throttle > 1023) {
      stop("Invalid trigger values");
      return false;
    }
    if (stickX < -512 || stickX > 512 || stickY < -512 || stickY > 512) {
      stop("Invalid stick values");
      return false;
    }
    if (haveInput_ && now - lastInput_ >= INPUT_TIMEOUT_MS) stop("Controller input gap");
    haveInput_ = true;
    lastInput_ = now;
    if (!armed_) {
      if (brake > DEADZONE || throttle > DEADZONE || l1 ||
          stickX < -STICK_DEADZONE || stickX > STICK_DEADZONE ||
          stickY < -STICK_DEADZONE || stickY > STICK_DEADZONE) {
        reason_ = "Center stick and release L1, L2 and R2 to arm";
        return true;
      }
      armed_ = true;
      reason_ = "Armed";
      return true;
    }
    if (l1 && !previousL1_) reverse_ = !reverse_;
    previousL1_ = l1;
    left_ = power(brake);
    right_ = power(throttle);
    if (reverse_) {
      left_ = -left_;
      right_ = -right_;
    }
    const int32_t turn = stickAxis(stickX);
    const int32_t forward = -stickAxis(stickY);
    stickActive_ = stickX < -STICK_DEADZONE || stickX > STICK_DEADZONE ||
        stickY < -STICK_DEADZONE || stickY > STICK_DEADZONE;
    if (stickActive_) {
      int32_t left = forward + turn;
      int32_t right = forward - turn;
      const int32_t absLeft = left < 0 ? -left : left;
      const int32_t absRight = right < 0 ? -right : right;
      const int32_t peak = absLeft > absRight ? absLeft : absRight;
      if (peak > MAX_POWER) {
        left = left * MAX_POWER / peak;
        right = right * MAX_POWER / peak;
      }
      left_ = static_cast<int8_t>(left);
      right_ = static_cast<int8_t>(right);
    }
    return true;
  }

  static int32_t stickAxis(int32_t raw) {
    const int32_t magnitude = raw < 0 ? -raw : raw;
    if (magnitude <= STICK_DEADZONE) return 0;
    const int32_t scaled = ((magnitude - STICK_DEADZONE) * MAX_POWER +
        (512 - STICK_DEADZONE) / 2) / (512 - STICK_DEADZONE);
    return raw < 0 ? -scaled : scaled;
  }

  static int8_t power(int32_t raw) {
    if (raw <= DEADZONE) return 0;
    if (raw > TRIGGER_MAX) raw = TRIGGER_MAX;
    return static_cast<int8_t>((raw * MAX_POWER + TRIGGER_MAX / 2) / TRIGGER_MAX);
  }

  bool armed() const { return armed_; }
  bool reverse() const { return reverse_; }
  bool stickActive() const { return stickActive_; }
  int8_t left() const { return left_; }
  int8_t right() const { return right_; }
  const char* reason() const { return reason_; }
  bool haveInput() const { return haveInput_; }
  uint32_t inputAge(uint32_t now) const { return now - lastInput_; }

 private:
  bool armed_ = false;
  bool reverse_ = false;
  bool previousL1_ = false;
  bool haveInput_ = false;
  bool stickActive_ = false;
  uint32_t lastInput_ = 0;
  int8_t left_ = 0;
  int8_t right_ = 0;
  const char* reason_ = "Waiting for released controls";
};
