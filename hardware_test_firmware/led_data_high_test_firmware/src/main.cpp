#include <Arduino.h>

namespace {

constexpr uint8_t kLedDataGpio = 9;
constexpr uint32_t kSerialBaud = 115200;
constexpr uint32_t kReportIntervalMs = 1000;

uint32_t lastReportMs = 0;

}  // namespace

void setup() {
  Serial.begin(kSerialBaud);
  delay(1000);

  pinMode(kLedDataGpio, OUTPUT);
  digitalWrite(kLedDataGpio, HIGH);

  Serial.println();
  Serial.println("TongDou LED_DATA HIGH test");
  Serial.println("LED_DATA / GPIO9 is configured as OUTPUT and held HIGH.");
  Serial.println("Measure R9 LED_DATA side to GND. Expected voltage is about 3.3V.");
}

void loop() {
  digitalWrite(kLedDataGpio, HIGH);

  const uint32_t nowMs = millis();
  if (nowMs - lastReportMs >= kReportIntervalMs) {
    lastReportMs = nowMs;
    Serial.println("LED_DATA / GPIO9 held HIGH");
  }
}
