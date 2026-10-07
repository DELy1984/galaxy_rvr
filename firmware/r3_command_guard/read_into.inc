void AiCamera::readInto(uint8_t* buffer) {
  static R3InputParser parser;
  recvBufferType = WS_BUFFER_TYPE_NONE;
  recvBufferLength = 0;
  buffer[0] = 0;
  if (parser.expire(millis())) {
    DebugSerial.println(F("[R3] Incomplete UART packet timed out"));
  }
  uint16_t budget = 256;
  while (DataSerial.available() && budget-- > 0) {
    const R3InputParser::Result result = parser.consume(
        static_cast<uint8_t>(DataSerial.read()), millis());
    if (result == R3InputParser::Result::Rejected) {
      DebugSerial.println(F("[R3] Malformed UART packet rejected"));
    } else if (result == R3InputParser::Result::Text) {
      const size_t length = strlen(parser.text());
      memcpy(buffer, parser.text(), length + 1);
      recvBufferType = WS_BUFFER_TYPE_TEXT;
      recvBufferLength = length;
      return;
    } else if (result == R3InputParser::Result::Motor) {
      memcpy(buffer, parser.payload(), 3);
      buffer[3] = 0;
      recvBufferType = WS_BUFFER_TYPE_BINARY;
      recvBufferLength = 3;
      return;
    }
  }
}
