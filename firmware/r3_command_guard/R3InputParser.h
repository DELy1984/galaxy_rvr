#pragma once

#include <stddef.h>
#include <stdint.h>

class R3InputParser {
 public:
  enum class Result { None, Text, Motor, Rejected };

  bool expire(uint32_t now) {
    if ((binary_ || textLength_ != 0) &&
        static_cast<uint32_t>(now - lastByte_) >= 100) {
      reset();
      return true;
    }
    return false;
  }

  Result consume(uint8_t value, uint32_t now) {
    lastByte_ = now;
    if (binary_) {
      frame_[frameLength_++] = value;
      if ((frameLength_ == 1 && value != 0xA0) ||
          (frameLength_ == 2 && value != 3)) {
        reset();
        return Result::Rejected;
      }
      if (frameLength_ == sizeof(frame_)) {
        const bool valid = value == 0xA1 &&
            frame_[2] == (frame_[3] ^ frame_[4] ^ frame_[5]);
        reset();
        return valid ? Result::Motor : Result::Rejected;
      }
      return Result::None;
    }
    if (value == '\r') return Result::None;
    if (value == '\n') {
      text_[textLength_] = '\0';
      textLength_ = 0;
      return Result::Text;
    }
    if (value < 32 || value > 126 || textLength_ == sizeof(text_) - 1) {
      reset();
      return Result::Rejected;
    }
    text_[textLength_++] = static_cast<char>(value);
    if (textLength_ == 4 && text_[0] == 'W' && text_[1] == 'S' &&
        text_[2] == 'B' && text_[3] == '+') {
      textLength_ = 0;
      frameLength_ = 0;
      binary_ = true;
    }
    return Result::None;
  }

  const char* text() const { return text_; }
  const uint8_t* payload() const { return frame_ + 3; }

 private:
  void reset() {
    textLength_ = 0;
    frameLength_ = 0;
    binary_ = false;
  }

  char text_[64] = {};
  uint8_t frame_[7] = {};
  size_t textLength_ = 0;
  size_t frameLength_ = 0;
  bool binary_ = false;
  uint32_t lastByte_ = 0;
};
