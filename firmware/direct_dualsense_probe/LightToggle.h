#pragma once

class LightToggle {
 public:
  void reset() {
    on_ = false;
    ready_ = false;
    previous_ = false;
  }
  void input(bool pressed) {
    if (!ready_) {
      if (!pressed) ready_ = true;
    } else if (pressed && !previous_) {
      on_ = !on_;
    }
    previous_ = pressed;
  }
  bool on() const { return on_; }

 private:
  bool on_ = false;
  bool ready_ = false;
  bool previous_ = false;
};
