#pragma once

#include <stddef.h>
#include <stdint.h>

class MotorCommandGuard {
 public:
  static constexpr uint32_t TIMEOUT_MS = 500;
  enum class Result { Accepted, Invalid, NeedsZero };

  bool expire(uint32_t now) {
    if (armed_ && static_cast<uint32_t>(now - lastCommand_) >= TIMEOUT_MS) {
      disarm();
      return true;
    }
    return false;
  }

  void disarm() {
    armed_ = false;
    left_ = 0;
    right_ = 0;
  }

  Result accept(const uint8_t* payload, size_t length, uint32_t now) {
    expire(now);
    if (payload == nullptr || length != 3 || payload[0] != 0x01) return Result::Invalid;
    const int16_t left = payload[1] <= 127 ? payload[1] : payload[1] - 256;
    const int16_t right = payload[2] <= 127 ? payload[2] : payload[2] - 256;
    if (left < -100 || left > 100 || right < -100 || right > 100) {
      return Result::Invalid;
    }
    if (!armed_ && (left != 0 || right != 0)) return Result::NeedsZero;
    armed_ = true;
    lastCommand_ = now;
    left_ = static_cast<int8_t>(left);
    right_ = static_cast<int8_t>(right);
    return Result::Accepted;
  }

  int8_t left() const { return left_; }
  int8_t right() const { return right_; }
  bool armed() const { return armed_; }

 private:
  bool armed_ = false;
  uint32_t lastCommand_ = 0;
  int8_t left_ = 0;
  int8_t right_ = 0;
};
