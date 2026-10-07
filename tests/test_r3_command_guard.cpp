#include "../firmware/r3_command_guard/MotorCommandGuard.h"
#include "../firmware/r3_command_guard/R3InputParser.h"
#include "../firmware/direct_dualsense_probe/TimeoutProbe.h"
#include "../firmware/direct_dualsense_probe/DriveControl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
  fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); exit(1); \
} } while (0)

static const uint8_t ZERO[] = {1, 0, 0};
static const uint8_t DRIVE[] = {1, 100, 156};

static void testGuard() {
  MotorCommandGuard guard;
  CHECK(!guard.armed());
  CHECK(guard.accept(DRIVE, 3, 0) == MotorCommandGuard::Result::NeedsZero);
  CHECK(guard.accept(ZERO, 3, 1) == MotorCommandGuard::Result::Accepted);
  CHECK(guard.accept(DRIVE, 3, 100) == MotorCommandGuard::Result::Accepted);
  CHECK(guard.left() == 100 && guard.right() == -100);
  CHECK(!guard.expire(599));
  CHECK(guard.expire(600));
  CHECK(!guard.armed() && guard.left() == 0 && guard.right() == 0);
  CHECK(!guard.expire(601));
  CHECK(guard.accept(DRIVE, 3, 602) == MotorCommandGuard::Result::NeedsZero);
  CHECK(guard.accept(ZERO, 3, 603) == MotorCommandGuard::Result::Accepted);
  CHECK(guard.accept(DRIVE, 3, 604) == MotorCommandGuard::Result::Accepted);
  guard.disarm();
  CHECK(guard.left() == 0 && guard.right() == 0 && !guard.armed());
  CHECK(guard.accept(DRIVE, 3, 605) == MotorCommandGuard::Result::NeedsZero);

  const uint8_t badEntity[] = {2, 0, 0};
  const uint8_t badLeft[] = {1, 101, 0};
  const uint8_t badRight[] = {1, 0, 155};
  CHECK(guard.accept(ZERO, 3, 1000) == MotorCommandGuard::Result::Accepted);
  CHECK(guard.accept(badEntity, 3, 1200) == MotorCommandGuard::Result::Invalid);
  CHECK(guard.accept(badLeft, 3, 1300) == MotorCommandGuard::Result::Invalid);
  CHECK(guard.accept(badRight, 3, 1400) == MotorCommandGuard::Result::Invalid);
  CHECK(guard.accept(ZERO, 2, 1499) == MotorCommandGuard::Result::Invalid);
  CHECK(guard.expire(1500));
  CHECK(guard.accept(nullptr, 0, 1501) == MotorCommandGuard::Result::Invalid);
  CHECK(guard.accept(nullptr, 3, 1501) == MotorCommandGuard::Result::Invalid);

  CHECK(guard.accept(ZERO, 3, 2000) == MotorCommandGuard::Result::Accepted);
  CHECK(guard.accept(DRIVE, 3, 2500) == MotorCommandGuard::Result::NeedsZero);
  CHECK(guard.left() == 0 && guard.right() == 0);
  CHECK(guard.accept(ZERO, 3, 3000) == MotorCommandGuard::Result::Accepted);
  for (uint32_t now = 3100; now < 10000; now += 100) {
    CHECK(guard.accept(DRIVE, 3, now) == MotorCommandGuard::Result::Accepted);
    CHECK(!guard.expire(now + 99));
  }
  CHECK(guard.accept(ZERO, 3, UINT32_MAX - 100) == MotorCommandGuard::Result::Accepted);
  CHECK(!guard.expire(398));
  CHECK(guard.expire(399));
}

static R3InputParser::Result feed(R3InputParser& parser,
                                  const uint8_t* bytes, size_t size,
                                  uint32_t now) {
  R3InputParser::Result result = R3InputParser::Result::None;
  for (size_t i = 0; i < size; ++i) result = parser.consume(bytes[i], now);
  return result;
}

static void testParser() {
  R3InputParser parser;
  const uint8_t frame[] = {'W', 'S', 'B', '+', 0xA0, 3, 1, 1, 0, 0, 0xA1};
  CHECK(feed(parser, frame, sizeof(frame), 0) == R3InputParser::Result::Motor);
  CHECK(memcmp(parser.payload(), ZERO, 3) == 0);
  CHECK(parser.consume('\r', 0) == R3InputParser::Result::None);
  CHECK(parser.consume('\n', 0) == R3InputParser::Result::Text);
  CHECK(strcmp(parser.text(), "") == 0);
  const char reply[] = "[OK] 1.5.4\r\n";
  CHECK(feed(parser, reinterpret_cast<const uint8_t*>(reply),
             sizeof(reply) - 1, 1) == R3InputParser::Result::Text);
  CHECK(strcmp(parser.text(), "[OK] 1.5.4") == 0);
  CHECK(feed(parser, frame, 8, 2) == R3InputParser::Result::None);
  CHECK(!parser.expire(101));
  CHECK(parser.expire(102));
  CHECK(feed(parser, frame, sizeof(frame), 103) == R3InputParser::Result::Motor);

  uint8_t bad[sizeof(frame)];
  memcpy(bad, frame, sizeof(frame));
  bad[6] = 0;
  CHECK(feed(parser, bad, sizeof(bad), 104) == R3InputParser::Result::Rejected);
  memcpy(bad, frame, sizeof(frame));
  bad[10] = 0;
  CHECK(feed(parser, bad, sizeof(bad), 105) == R3InputParser::Result::Rejected);
  CHECK(feed(parser, frame, 5, 106) == R3InputParser::Result::None);
  CHECK(parser.consume(200, 106) == R3InputParser::Result::Rejected);
  for (int i = 0; i < 63; ++i) CHECK(parser.consume('x', 107) == R3InputParser::Result::None);
  CHECK(parser.consume('x', 107) == R3InputParser::Result::Rejected);
  CHECK(feed(parser, frame, sizeof(frame), 108) == R3InputParser::Result::Motor);

  MotorCommandGuard guard;
  CHECK(guard.accept(parser.payload(), 3, 108) == MotorCommandGuard::Result::Accepted);
  CHECK(feed(parser, bad, sizeof(bad), 600) == R3InputParser::Result::Rejected);
  CHECK(guard.expire(608));
  CHECK(!guard.armed());
}

static uint32_t fakeTime = 0;
static uint32_t stops = 0;
static uint32_t logs = 0;
static uint32_t millis() { return fakeTime; }
static void carStop() { ++stops; }
#define F(text) text
static struct {
  void println(const char*) { ++logs; }
} Serial;
static struct {
  bool ws_connected = true;
  uint8_t recvBuffer[3] = {};
  size_t recvBufferLength = 3;
} aiCam;
static MotorCommandGuard motorGuard;
static int8_t leftMotorPower = 0;
static int8_t rightMotorPower = 0;
static uint8_t currentMode = 0;
static constexpr uint8_t MODE_APP_CONTROL = 1;
#include "../firmware/r3_command_guard/on_receive.inc"

static void receive(const uint8_t* payload, uint32_t now) {
  fakeTime = now;
  memcpy(aiCam.recvBuffer, payload, 3);
  onReceive();
}

static void testIntegration() {
  receive(DRIVE, 0);
  CHECK(leftMotorPower == 0 && rightMotorPower == 0);
  CHECK(stops != 0 && logs != 0);
  receive(ZERO, 1);
  receive(DRIVE, 2);
  CHECK(leftMotorPower == 100 && rightMotorPower == -100);
  CHECK(currentMode == MODE_APP_CONTROL);
  fakeTime = 501;
  enforceMotorGuard();
  CHECK(leftMotorPower == 100);
  const uint32_t stopsBefore = stops;
  fakeTime = 502;
  enforceMotorGuard();
  CHECK(stops == stopsBefore + 1);
  CHECK(leftMotorPower == 0 && rightMotorPower == 0);
  receive(DRIVE, 503);
  CHECK(leftMotorPower == 0 && rightMotorPower == 0);
  receive(ZERO, 504);
  receive(DRIVE, 505);
  aiCam.ws_connected = false;
  fakeTime = 506;
  enforceMotorGuard();
  CHECK(!motorGuard.armed() && leftMotorPower == 0 && rightMotorPower == 0);
  aiCam.ws_connected = true;
  receive(DRIVE, 507);
  CHECK(leftMotorPower == 0);
}

static void testDriveControl() {
  DriveControl drive;
  drive.check(0, true, true, false);
  CHECK(!drive.armed() && drive.left() == 0);
  CHECK(drive.input(1, 1020, 0, false));
  CHECK(!drive.armed());
  CHECK(drive.input(2, 0, 0, true));
  CHECK(!drive.armed());
  CHECK(drive.input(3, 0, 0, false));
  CHECK(drive.armed() && !drive.reverse());
  CHECK(drive.input(4, 1020, 0, false));
  CHECK(drive.left() == 100 && drive.right() == 0);
  CHECK(drive.input(5, 1020, 1020, true));
  CHECK(drive.reverse() && drive.left() == -100 && drive.right() == -100);
  CHECK(drive.input(6, 1020, 1020, true));
  CHECK(drive.reverse());
  CHECK(drive.input(7, 1020, 1020, false));
  CHECK(drive.input(8, 1020, 1020, true));
  CHECK(!drive.reverse() && drive.left() == 100);
  drive.check(257, true, true, false);
  CHECK(drive.armed());
  drive.check(258, true, true, false);
  CHECK(!drive.armed() && drive.left() == 0 && drive.right() == 0);
  CHECK(drive.input(259, 1020, 1020, false));
  CHECK(!drive.armed());
  CHECK(drive.input(260, 0, 0, false));
  CHECK(drive.armed() && !drive.reverse());
  CHECK(drive.input(261, 0, 1020, false));
  CHECK(drive.left() == 0 && drive.right() == 100);
  drive.check(262, false, true, false);
  CHECK(!drive.armed() && drive.right() == 0);
  CHECK(drive.input(263, 0, 0, false));
  drive.check(264, true, false, false);
  CHECK(!drive.armed());
  CHECK(drive.input(265, 0, 0, false));
  drive.check(266, true, true, true);
  CHECK(!drive.armed());
  CHECK(drive.input(267, -1, 0, false) == false);
  CHECK(drive.input(268, 0, 1024, false) == false);
  CHECK(!drive.armed());
  CHECK(drive.input(269, 0, 0, false));
  CHECK(drive.input(519, 1020, 1020, false));
  CHECK(!drive.armed() && drive.left() == 0);
  CHECK(drive.input(UINT32_MAX - 100, 0, 0, false));
  drive.check(148, true, true, false);
  CHECK(drive.armed());
  drive.check(149, true, true, false);
  CHECK(!drive.armed());
  CHECK(DriveControl::power(0) == 0);
  CHECK(DriveControl::power(20) == 0);
  CHECK(DriveControl::power(21) == 2);
  CHECK(DriveControl::power(306) == 30);
  CHECK(DriveControl::power(510) == 50);
  CHECK(DriveControl::power(765) == 75);
  CHECK(DriveControl::power(1020) == 100);
  CHECK(DriveControl::power(1023) == 100);
  CHECK(drive.input(150, 0, 0, false));
  CHECK(drive.input(151, 306, 765, false));
  CHECK(drive.left() == 30 && drive.right() == 75);
  CHECK(drive.input(152, 306, 765, true));
  CHECK(drive.left() == -30 && drive.right() == -75);
  for (int32_t raw = 0; raw <= 1023; ++raw) {
    CHECK(DriveControl::power(raw) >= 0 && DriveControl::power(raw) <= 100);
    if (raw > DriveControl::DEADZONE) {
      const int32_t clamped = raw > 1020 ? 1020 : raw;
      CHECK(DriveControl::power(raw) == (clamped * 100 + 510) / 1020);
    }
    if (raw != 0) CHECK(DriveControl::power(raw) >= DriveControl::power(raw - 1));
  }
}

int main() {
  testGuard();
  testParser();
  testIntegration();
  testDriveControl();
  TimeoutProbe probe;
  CHECK(probe.step(10000) == TimeoutProbe::Action::None);
  CHECK(probe.start(10000));
  CHECK(!probe.start(10001));
  CHECK(probe.step(10099) == TimeoutProbe::Action::None);
  CHECK(probe.step(10100) == TimeoutProbe::Action::Drive);
  CHECK(probe.step(12099) == TimeoutProbe::Action::None);
  CHECK(probe.step(12100) == TimeoutProbe::Action::Stop);
  CHECK(!probe.active() && !probe.start(12101));
  CHECK(probe.step(12102) == TimeoutProbe::Action::None);
  TimeoutProbe delayed;
  CHECK(delayed.start(0));
  CHECK(delayed.step(500) == TimeoutProbe::Action::Stop);
  CHECK(delayed.state() == TimeoutProbe::State::Aborted);
  TimeoutProbe cancelled;
  cancelled.abort();
  CHECK(!cancelled.start(0));
  CHECK(cancelled.step(1000) == TimeoutProbe::Action::None);
  TimeoutProbe wrapped;
  CHECK(wrapped.start(UINT32_MAX - 50));
  CHECK(wrapped.step(49) == TimeoutProbe::Action::Drive);
  CHECK(wrapped.step(2049) == TimeoutProbe::Action::Stop);
  puts("PASS: R3 guard/parser/integration, timeout probe and direct drive safety");
}
