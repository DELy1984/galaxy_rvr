#pragma once

#include <stdint.h>

class TimeoutProbe {
 public:
  enum class State { Idle, Preparing, Silent, Completed, Aborted };
  enum class Action { None, Drive, Stop };

  bool start(uint32_t now) {
    if (state_ != State::Idle) return false;
    state_ = State::Preparing;
    phaseStart_ = now;
    return true;
  }

  Action step(uint32_t now) {
    const uint32_t age = now - phaseStart_;
    if (state_ == State::Preparing && age >= 100) {
      // A delayed loop must not unexpectedly begin a movement test.
      if (age >= 500) {
        state_ = State::Aborted;
        return Action::Stop;
      }
      state_ = State::Silent;
      phaseStart_ = now;
      return Action::Drive;
    }
    if (state_ == State::Silent && age >= 2000) {
      state_ = State::Completed;
      return Action::Stop;
    }
    return Action::None;
  }

  void abort() { state_ = State::Aborted; }
  bool active() const {
    return state_ == State::Preparing || state_ == State::Silent;
  }
  State state() const { return state_; }
  uint32_t driveTime() const { return phaseStart_; }

 private:
  State state_ = State::Idle;
  uint32_t phaseStart_ = 0;
};
