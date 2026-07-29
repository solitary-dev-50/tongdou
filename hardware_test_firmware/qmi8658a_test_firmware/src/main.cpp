#include <Arduino.h>
#include <Wire.h>
#include <driver/i2s.h>

namespace {

constexpr int kI2cSda = 5;
constexpr int kI2cScl = 6;
constexpr int kLedPin = 9;
constexpr int kPdmClockPin = 1;
constexpr int kPdmDataPin = 2;
constexpr int kSpeakerLrclkPin = 12;
constexpr int kSpeakerBclkPin = 13;
constexpr int kSpeakerDataPin = 14;
constexpr int kSpeakerCtrlPin = 15;
constexpr int kUsbDetectPin = 17;
constexpr int kBatteryAdcPin = 8;
constexpr int kChargePin = 35;
constexpr int kStandbyPin = 48;
constexpr int kMotorAin1Pin = 38;
constexpr int kMotorAin2Pin = 39;
constexpr int kMotorBin2Pin = 40;
constexpr int kMotorBin1Pin = 41;
constexpr int kMotorSleepPin = 42;
constexpr uint32_t kSerialBaud = 115200;
constexpr uint32_t kI2cClockHz = 50000;
constexpr uint32_t kMicSampleRateHz = 16000;
constexpr i2s_port_t kMicI2sPort = I2S_NUM_0;
constexpr uint32_t kSpeakerSampleRateHz = 16000;
constexpr i2s_port_t kSpeakerI2sPort = I2S_NUM_1;
constexpr uint8_t kMotorPwmAin1Channel = 0;
constexpr uint8_t kMotorPwmAin2Channel = 1;
constexpr uint8_t kMotorPwmBin1Channel = 2;
constexpr uint8_t kMotorPwmBin2Channel = 3;
constexpr uint32_t kMotorPwmHz = 20000;
constexpr uint8_t kMotorPwmBits = 8;

constexpr uint8_t kQmiAddrA = 0x6A;
constexpr uint8_t kQmiAddrB = 0x6B;
constexpr uint8_t kQmiWhoAmI = 0x00;
constexpr uint8_t kQmiExpectedWho = 0x05;
constexpr uint8_t kQmiCtrl1 = 0x02;
constexpr uint8_t kQmiCtrl2 = 0x03;
constexpr uint8_t kQmiCtrl3 = 0x04;
constexpr uint8_t kQmiCtrl5 = 0x06;
constexpr uint8_t kQmiCtrl7 = 0x08;
constexpr uint8_t kQmiAccelXoutL = 0x35;
constexpr uint8_t kQmiGyroXoutL = 0x3B;

uint8_t activeQmiAddress = 0;
uint32_t lastReportMs = 0;
uint32_t lastLedMs = 0;
uint32_t lastMicReportMs = 0;
uint32_t lastBatteryReportMs = 0;
uint8_t ledStep = 0;
bool ledAutoTest = true;
bool micReady = false;
bool micReportEnabled = true;
bool batteryReportEnabled = true;
bool speakerInstalled = false;

void setTestLed(uint8_t red, uint8_t green, uint8_t blue) {
  neopixelWrite(kLedPin, red, green, blue);
}

void printLedHelp() {
  Serial.println("commands: scan | led auto | led red | led green | led blue | led white | led off");
  Serial.println("commands: mic on | mic off | motor wake | motor sleep | motor test | motor a+ | motor a- | motor b+ | motor b-");
  Serial.println("commands: amp on | amp off | amp beep");
  Serial.println("commands: battery on | battery off | battery once");
}

void updateLedAutoTest() {
  if (!ledAutoTest || millis() - lastLedMs < 700) {
    return;
  }

  lastLedMs = millis();
  switch (ledStep) {
    case 0:
      setTestLed(32, 0, 0);
      Serial.println("led test: red");
      break;
    case 1:
      setTestLed(0, 32, 0);
      Serial.println("led test: green");
      break;
    case 2:
      setTestLed(0, 0, 32);
      Serial.println("led test: blue");
      break;
    case 3:
      setTestLed(24, 24, 24);
      Serial.println("led test: white");
      break;
    default:
      setTestLed(0, 0, 0);
      Serial.println("led test: off");
      break;
  }
  ledStep = (ledStep + 1) % 5;
}

void setupMotorPins() {
  pinMode(kMotorSleepPin, OUTPUT);
  digitalWrite(kMotorSleepPin, LOW);

  ledcSetup(kMotorPwmAin1Channel, kMotorPwmHz, kMotorPwmBits);
  ledcSetup(kMotorPwmAin2Channel, kMotorPwmHz, kMotorPwmBits);
  ledcSetup(kMotorPwmBin1Channel, kMotorPwmHz, kMotorPwmBits);
  ledcSetup(kMotorPwmBin2Channel, kMotorPwmHz, kMotorPwmBits);
  ledcAttachPin(kMotorAin1Pin, kMotorPwmAin1Channel);
  ledcAttachPin(kMotorAin2Pin, kMotorPwmAin2Channel);
  ledcAttachPin(kMotorBin1Pin, kMotorPwmBin1Channel);
  ledcAttachPin(kMotorBin2Pin, kMotorPwmBin2Channel);

  ledcWrite(kMotorPwmAin1Channel, 0);
  ledcWrite(kMotorPwmAin2Channel, 0);
  ledcWrite(kMotorPwmBin1Channel, 0);
  ledcWrite(kMotorPwmBin2Channel, 0);
  Serial.println("motor: pins ready, nSLEEP LOW");
}

void motorWake() {
  digitalWrite(kMotorSleepPin, HIGH);
  delay(10);
  Serial.println("motor: nSLEEP HIGH, AT8833CT awake");
}

void motorStop(bool sleepAfterStop) {
  ledcWrite(kMotorPwmAin1Channel, 0);
  ledcWrite(kMotorPwmAin2Channel, 0);
  ledcWrite(kMotorPwmBin1Channel, 0);
  ledcWrite(kMotorPwmBin2Channel, 0);
  if (sleepAfterStop) {
    digitalWrite(kMotorSleepPin, LOW);
    Serial.println("motor: stop, nSLEEP LOW");
  } else {
    Serial.println("motor: stop, nSLEEP kept HIGH");
  }
}

void driveMotorA(int pwm) {
  pwm = constrain(pwm, -255, 255);
  ledcWrite(kMotorPwmAin1Channel, pwm > 0 ? pwm : 0);
  ledcWrite(kMotorPwmAin2Channel, pwm < 0 ? -pwm : 0);
}

void driveMotorB(int pwm) {
  pwm = constrain(pwm, -255, 255);
  ledcWrite(kMotorPwmBin1Channel, pwm > 0 ? pwm : 0);
  ledcWrite(kMotorPwmBin2Channel, pwm < 0 ? -pwm : 0);
}

void motorSinglePulse(char motor, int pwm, uint16_t durationMs) {
  motorWake();
  if (motor == 'A') {
    Serial.print("motor: A pulse pwm=");
    Serial.println(pwm);
    driveMotorA(pwm);
  } else {
    Serial.print("motor: B pulse pwm=");
    Serial.println(pwm);
    driveMotorB(pwm);
  }
  delay(durationMs);
  motorStop(true);
}

void runMotorTest() {
  Serial.println("motor test: short AT8833CT wake and output pulse sequence");
  Serial.println("motor test: A+ A- B+ B-, pwm=180, each 350ms");
  motorWake();

  driveMotorA(180);
  Serial.println("motor test: A+");
  delay(350);
  motorStop(false);
  delay(250);

  driveMotorA(-180);
  Serial.println("motor test: A-");
  delay(350);
  motorStop(false);
  delay(250);

  driveMotorB(180);
  Serial.println("motor test: B+");
  delay(350);
  motorStop(false);
  delay(250);

  driveMotorB(-180);
  Serial.println("motor test: B-");
  delay(350);
  motorStop(true);
  Serial.println("motor test: done");
}

void setupSpeakerCtrl() {
  pinMode(kSpeakerCtrlPin, OUTPUT);
  digitalWrite(kSpeakerCtrlPin, LOW);
  Serial.println("amp: CTRL LOW");
}

bool beginSpeakerI2s() {
  if (speakerInstalled) {
    return true;
  }

  i2s_config_t config = {};
  config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX);
  config.sample_rate = kSpeakerSampleRateHz;
  config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  config.dma_buf_count = 4;
  config.dma_buf_len = 256;
  config.use_apll = false;
  config.tx_desc_auto_clear = true;
  config.fixed_mclk = 0;

  const esp_err_t installResult = i2s_driver_install(kSpeakerI2sPort, &config, 0, nullptr);
  if (installResult != ESP_OK) {
    Serial.print("amp: i2s_driver_install failed err=");
    Serial.println(static_cast<int>(installResult));
    return false;
  }

  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = kSpeakerBclkPin;
  pins.ws_io_num = kSpeakerLrclkPin;
  pins.data_out_num = kSpeakerDataPin;
  pins.data_in_num = I2S_PIN_NO_CHANGE;

  const esp_err_t pinResult = i2s_set_pin(kSpeakerI2sPort, &pins);
  if (pinResult != ESP_OK) {
    Serial.print("amp: i2s_set_pin failed err=");
    Serial.println(static_cast<int>(pinResult));
    i2s_driver_uninstall(kSpeakerI2sPort);
    return false;
  }

  i2s_zero_dma_buffer(kSpeakerI2sPort);
  speakerInstalled = true;
  Serial.print("amp: I2S begin ok LRCLK=IO");
  Serial.print(kSpeakerLrclkPin);
  Serial.print(" BCLK=IO");
  Serial.print(kSpeakerBclkPin);
  Serial.print(" DATA=IO");
  Serial.println(kSpeakerDataPin);
  return true;
}

void stopSpeakerI2s() {
  if (speakerInstalled) {
    i2s_zero_dma_buffer(kSpeakerI2sPort);
    i2s_driver_uninstall(kSpeakerI2sPort);
    speakerInstalled = false;
  }
}

void ampOff() {
  stopSpeakerI2s();
  digitalWrite(kSpeakerCtrlPin, LOW);
  Serial.println("amp: CTRL LOW, I2S stopped");
}

void ampOn() {
  digitalWrite(kSpeakerCtrlPin, HIGH);
  Serial.println("amp: CTRL HIGH");
}

void runAmpBeep() {
  Serial.println("amp test: CTRL HIGH + 1kHz square beep, 600ms");
  ampOn();
  if (!beginSpeakerI2s()) {
    Serial.println("amp test: failed");
    return;
  }

  constexpr size_t kFrames = 128;
  int16_t buffer[kFrames * 2] = {};
  const uint32_t startMs = millis();
  uint32_t sampleIndex = 0;
  while (millis() - startMs < 600) {
    for (size_t i = 0; i < kFrames; ++i) {
      const int16_t sample = ((sampleIndex / 8) % 2 == 0) ? 5000 : -5000;
      buffer[i * 2] = sample;
      buffer[i * 2 + 1] = sample;
      ++sampleIndex;
    }
    size_t bytesWritten = 0;
    const esp_err_t result = i2s_write(kSpeakerI2sPort, buffer, sizeof(buffer), &bytesWritten, pdMS_TO_TICKS(100));
    if (result != ESP_OK) {
      Serial.print("amp: i2s_write failed err=");
      Serial.println(static_cast<int>(result));
      break;
    }
  }

  delay(30);
  ampOff();
  Serial.println("amp test: done");
}

void setupBatteryPins() {
  pinMode(kUsbDetectPin, INPUT);
  pinMode(kBatteryAdcPin, INPUT);
  pinMode(kChargePin, INPUT_PULLUP);
  pinMode(kStandbyPin, INPUT_PULLUP);
  analogReadResolution(12);
  analogSetPinAttenuation(kBatteryAdcPin, ADC_11db);
  analogSetPinAttenuation(kUsbDetectPin, ADC_11db);
  Serial.print("battery: pins ready VBAT_ADC=IO");
  Serial.print(kBatteryAdcPin);
  Serial.print(" USB_CHG=IO");
  Serial.print(kUsbDetectPin);
  Serial.print(" CHRG#=IO");
  Serial.print(kChargePin);
  Serial.print(" STBY#=IO");
  Serial.println(kStandbyPin);
}

void reportBattery() {
  const int vbatRaw = analogRead(kBatteryAdcPin);
  const int vbatAdcMv = analogReadMilliVolts(kBatteryAdcPin);
  const int usbRaw = analogRead(kUsbDetectPin);
  const int usbAdcMv = analogReadMilliVolts(kUsbDetectPin);
  const bool usbDigitalHigh = digitalRead(kUsbDetectPin) == HIGH;
  const bool usbPresent = usbAdcMv > 1000;
  const bool charging = digitalRead(kChargePin) == LOW;
  const bool standby = digitalRead(kStandbyPin) == LOW;
  const int vbatEstimatedMv = vbatAdcMv * 2;

  const char* state = "battery/discharge";
  if (charging) {
    state = "charging";
  } else if (standby) {
    state = "standby/full";
  } else if (usbPresent) {
    state = "usb present, not charging";
  }

  Serial.print("battery vbat_raw=");
  Serial.print(vbatRaw);
  Serial.print(" vbat_adc_mv=");
  Serial.print(vbatAdcMv);
  Serial.print(" vbat_est_mv=");
  Serial.print(vbatEstimatedMv);
  Serial.print(" usb_raw=");
  Serial.print(usbRaw);
  Serial.print(" usb_adc_mv=");
  Serial.print(usbAdcMv);
  Serial.print(" usb_digital=");
  Serial.print(usbDigitalHigh ? 1 : 0);
  Serial.print(" usb_adc_present=");
  Serial.print(usbPresent ? 1 : 0);
  Serial.print(" chrg_low=");
  Serial.print(charging ? 1 : 0);
  Serial.print(" stby_low=");
  Serial.print(standby ? 1 : 0);
  Serial.print(" state=");
  Serial.println(state);
}

void updateBatteryReport() {
  if (!batteryReportEnabled || millis() - lastBatteryReportMs < 1000) {
    return;
  }
  lastBatteryReportMs = millis();
  reportBattery();
}

bool i2cAddressAck(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

bool readRegister(uint8_t address, uint8_t reg, uint8_t& value) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  if (Wire.requestFrom(static_cast<int>(address), 1) != 1) {
    return false;
  }
  value = Wire.read();
  return true;
}

bool readRegisters(uint8_t address, uint8_t reg, uint8_t* buffer, uint8_t length) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }
  if (Wire.requestFrom(static_cast<int>(address), static_cast<int>(length)) != length) {
    return false;
  }
  for (uint8_t i = 0; i < length; ++i) {
    buffer[i] = Wire.read();
  }
  return true;
}

bool writeRegister(uint8_t address, uint8_t reg, uint8_t value) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

int16_t readInt16Le(const uint8_t* buffer) {
  return static_cast<int16_t>((static_cast<uint16_t>(buffer[1]) << 8) | buffer[0]);
}

void printHex2(uint8_t value) {
  if (value < 0x10) {
    Serial.print('0');
  }
  Serial.print(value, HEX);
}

void scanI2c() {
  Serial.println();
  Serial.println("i2c scan begin");
  uint8_t count = 0;
  for (uint8_t address = 0x08; address <= 0x77; ++address) {
    if (!i2cAddressAck(address)) {
      continue;
    }

    Serial.print("  found 0x");
    printHex2(address);
    if (address == 0x3C || address == 0x3D) {
      Serial.print("  OLED candidate");
    }
    if (address == kQmiAddrA || address == kQmiAddrB) {
      Serial.print("  QMI8658A candidate");
    }
    Serial.println();
    ++count;
  }
  Serial.print("i2c scan count=");
  Serial.println(count);
}

void dumpQmiRegisters(uint8_t address) {
  Serial.print("qmi register dump addr=0x");
  printHex2(address);
  Serial.println(" reg 0x00..0x0F");
  for (uint8_t reg = 0x00; reg <= 0x0F; ++reg) {
    uint8_t value = 0xFF;
    Serial.print("  0x");
    printHex2(reg);
    Serial.print(" = ");
    if (readRegister(address, reg, value)) {
      Serial.print("0x");
      printHex2(value);
      if (reg == kQmiWhoAmI) {
        Serial.print(value == kQmiExpectedWho ? " expected WHO_AM_I" : " unexpected WHO_AM_I");
      }
      Serial.println();
    } else {
      Serial.println("read failed");
    }
  }
}

bool configureQmi(uint8_t address) {
  bool ok = true;
  ok = writeRegister(address, kQmiCtrl1, 0x60) && ok;
  ok = writeRegister(address, kQmiCtrl2, 0x26) && ok;
  ok = writeRegister(address, kQmiCtrl3, 0x56) && ok;
  ok = writeRegister(address, kQmiCtrl5, 0x11) && ok;
  ok = writeRegister(address, kQmiCtrl7, 0x03) && ok;
  delay(30);
  return ok;
}

bool findQmi() {
  activeQmiAddress = 0;

  const uint8_t addresses[] = {kQmiAddrA, kQmiAddrB};
  for (uint8_t address : addresses) {
    if (!i2cAddressAck(address)) {
      continue;
    }

    uint8_t who = 0;
    const bool whoOk = readRegister(address, kQmiWhoAmI, who);
    Serial.print("qmi candidate addr=0x");
    printHex2(address);
    Serial.print(" who_read=");
    Serial.print(whoOk ? "ok" : "failed");
    if (whoOk) {
      Serial.print(" who=0x");
      printHex2(who);
    }
    Serial.println();

    dumpQmiRegisters(address);
    activeQmiAddress = address;
    return true;
  }

  return false;
}

void readQmiMotion() {
  if (activeQmiAddress == 0) {
    return;
  }

  uint8_t accel[6] = {};
  uint8_t gyro[6] = {};
  const bool accelOk = readRegisters(activeQmiAddress, kQmiAccelXoutL, accel, sizeof(accel));
  const bool gyroOk = readRegisters(activeQmiAddress, kQmiGyroXoutL, gyro, sizeof(gyro));

  Serial.print("motion addr=0x");
  printHex2(activeQmiAddress);
  Serial.print(" accel_ok=");
  Serial.print(accelOk ? 1 : 0);
  if (accelOk) {
    Serial.print(" ax=");
    Serial.print(readInt16Le(&accel[0]));
    Serial.print(" ay=");
    Serial.print(readInt16Le(&accel[2]));
    Serial.print(" az=");
    Serial.print(readInt16Le(&accel[4]));
  }

  Serial.print(" gyro_ok=");
  Serial.print(gyroOk ? 1 : 0);
  if (gyroOk) {
    Serial.print(" gx=");
    Serial.print(readInt16Le(&gyro[0]));
    Serial.print(" gy=");
    Serial.print(readInt16Le(&gyro[2]));
    Serial.print(" gz=");
    Serial.print(readInt16Le(&gyro[4]));
  }
  Serial.println();
}

bool beginMic() {
  i2s_config_t config = {};
  config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
  config.sample_rate = kMicSampleRateHz;
  config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
  config.dma_buf_count = 4;
  config.dma_buf_len = 256;
  config.use_apll = false;
  config.tx_desc_auto_clear = false;
  config.fixed_mclk = 0;

  const esp_err_t installResult = i2s_driver_install(kMicI2sPort, &config, 0, nullptr);
  if (installResult != ESP_OK) {
    Serial.print("mic: i2s_driver_install failed err=");
    Serial.println(static_cast<int>(installResult));
    return false;
  }

  i2s_pin_config_t pins = {};
  pins.mck_io_num = I2S_PIN_NO_CHANGE;
  pins.bck_io_num = I2S_PIN_NO_CHANGE;
  pins.ws_io_num = kPdmClockPin;
  pins.data_out_num = I2S_PIN_NO_CHANGE;
  pins.data_in_num = kPdmDataPin;

  const esp_err_t pinResult = i2s_set_pin(kMicI2sPort, &pins);
  if (pinResult != ESP_OK) {
    Serial.print("mic: i2s_set_pin failed err=");
    Serial.println(static_cast<int>(pinResult));
    i2s_driver_uninstall(kMicI2sPort);
    return false;
  }

  i2s_zero_dma_buffer(kMicI2sPort);
  Serial.print("mic: PDM begin ok CLK=IO");
  Serial.print(kPdmClockPin);
  Serial.print(" DATA=IO");
  Serial.print(kPdmDataPin);
  Serial.print(" sample_rate=");
  Serial.println(kMicSampleRateHz);
  return true;
}

void reportMicLevel() {
  if (!micReady || !micReportEnabled || millis() - lastMicReportMs < 500) {
    return;
  }
  lastMicReportMs = millis();

  int16_t samples[256] = {};
  size_t bytesRead = 0;
  const esp_err_t result = i2s_read(kMicI2sPort, samples, sizeof(samples), &bytesRead, pdMS_TO_TICKS(20));
  if (result != ESP_OK) {
    Serial.print("mic: read failed err=");
    Serial.println(static_cast<int>(result));
    return;
  }
  if (bytesRead == 0) {
    Serial.println("mic: no samples");
    return;
  }

  const size_t count = bytesRead / sizeof(samples[0]);
  int16_t minValue = 32767;
  int16_t maxValue = -32768;
  int64_t sum = 0;
  int64_t sumSquares = 0;
  for (size_t i = 0; i < count; ++i) {
    const int16_t value = samples[i];
    minValue = min(minValue, value);
    maxValue = max(maxValue, value);
    sum += value;
    sumSquares += static_cast<int32_t>(value) * value;
  }

  const int32_t average = static_cast<int32_t>(sum / static_cast<int64_t>(count));
  int64_t variance = (sumSquares / static_cast<int64_t>(count)) - static_cast<int64_t>(average) * average;
  if (variance < 0) {
    variance = 0;
  }

  Serial.print("mic samples=");
  Serial.print(count);
  Serial.print(" avg=");
  Serial.print(average);
  Serial.print(" min=");
  Serial.print(minValue);
  Serial.print(" max=");
  Serial.print(maxValue);
  Serial.print(" peak=");
  Serial.print(max(abs(minValue), abs(maxValue)));
  Serial.print(" rms2=");
  Serial.println(static_cast<long>(variance));
}

void runFullProbe() {
  Serial.println();
  Serial.println("==== TONGDOU V9 BOARD NO-LOAD TEST ====");
  Serial.print("SDA=IO");
  Serial.print(kI2cSda);
  Serial.print(" SCL=IO");
  Serial.print(kI2cScl);
  Serial.print(" clock=");
  Serial.print(kI2cClockHz);
  Serial.println("Hz");
  Serial.println("board no-load test: QMI8658A, WS2812, PDM mic, AT8833CT wake/output, NS4168 beep, battery/charge");
  Serial.print("WS2812 test LED=IO");
  Serial.println(kLedPin);
  Serial.print("PDM mic CLK=IO");
  Serial.print(kPdmClockPin);
  Serial.print(" DATA=IO");
  Serial.println(kPdmDataPin);
  Serial.print("AT8833CT nSLEEP=IO");
  Serial.print(kMotorSleepPin);
  Serial.print(" AIN1=IO");
  Serial.print(kMotorAin1Pin);
  Serial.print(" AIN2=IO");
  Serial.print(kMotorAin2Pin);
  Serial.print(" BIN1=IO");
  Serial.print(kMotorBin1Pin);
  Serial.print(" BIN2=IO");
  Serial.println(kMotorBin2Pin);
  Serial.print("NS4168 CTRL=IO");
  Serial.print(kSpeakerCtrlPin);
  Serial.print(" LRCLK=IO");
  Serial.print(kSpeakerLrclkPin);
  Serial.print(" BCLK=IO");
  Serial.print(kSpeakerBclkPin);
  Serial.print(" DATA=IO");
  Serial.println(kSpeakerDataPin);
  Serial.print("Battery VBAT_ADC=IO");
  Serial.print(kBatteryAdcPin);
  Serial.print(" USB_CHG=IO");
  Serial.print(kUsbDetectPin);
  Serial.print(" CHRG#=IO");
  Serial.print(kChargePin);
  Serial.print(" STBY#=IO");
  Serial.println(kStandbyPin);
  reportBattery();

  scanI2c();
  if (!findQmi()) {
    Serial.println("QMI8658A NOT FOUND");
    Serial.println("If only 0x3C appears, OLED is alive but QMI8658A is not responding on this I2C bus.");
    return;
  }

  Serial.print("QMI8658A FOUND at 0x");
  printHex2(activeQmiAddress);
  Serial.println();
  Serial.println(configureQmi(activeQmiAddress) ? "QMI8658A config ok" : "QMI8658A config failed");
}

void handleSerialCommand(String command) {
  command.trim();
  command.toLowerCase();
  if (command.length() == 0) {
    return;
  }

  if (command == "scan") {
    runFullProbe();
    return;
  }

  if (command == "led auto") {
    ledAutoTest = true;
    Serial.println("led auto test enabled");
    return;
  }
  if (command == "led red") {
    ledAutoTest = false;
    setTestLed(48, 0, 0);
    Serial.println("led manual: red");
    return;
  }
  if (command == "led green") {
    ledAutoTest = false;
    setTestLed(0, 48, 0);
    Serial.println("led manual: green");
    return;
  }
  if (command == "led blue") {
    ledAutoTest = false;
    setTestLed(0, 0, 48);
    Serial.println("led manual: blue");
    return;
  }
  if (command == "led white") {
    ledAutoTest = false;
    setTestLed(40, 40, 40);
    Serial.println("led manual: white");
    return;
  }
  if (command == "led off") {
    ledAutoTest = false;
    setTestLed(0, 0, 0);
    Serial.println("led manual: off");
    return;
  }
  if (command == "mic on") {
    micReportEnabled = true;
    Serial.println("mic report enabled");
    return;
  }
  if (command == "mic off") {
    micReportEnabled = false;
    Serial.println("mic report disabled");
    return;
  }
  if (command == "motor wake") {
    motorWake();
    motorStop(false);
    return;
  }
  if (command == "motor sleep") {
    motorStop(true);
    return;
  }
  if (command == "motor test") {
    runMotorTest();
    return;
  }
  if (command == "motor a+") {
    motorSinglePulse('A', 180, 500);
    return;
  }
  if (command == "motor a-") {
    motorSinglePulse('A', -180, 500);
    return;
  }
  if (command == "motor b+") {
    motorSinglePulse('B', 180, 500);
    return;
  }
  if (command == "motor b-") {
    motorSinglePulse('B', -180, 500);
    return;
  }
  if (command == "amp on") {
    ampOn();
    return;
  }
  if (command == "amp off") {
    ampOff();
    return;
  }
  if (command == "amp beep") {
    runAmpBeep();
    return;
  }
  if (command == "battery on") {
    batteryReportEnabled = true;
    Serial.println("battery report enabled");
    reportBattery();
    return;
  }
  if (command == "battery off") {
    batteryReportEnabled = false;
    Serial.println("battery report disabled");
    return;
  }
  if (command == "battery once") {
    reportBattery();
    return;
  }

  Serial.print("unknown command: ");
  Serial.println(command);
  printLedHelp();
}

}  // namespace

void setup() {
  Serial.begin(kSerialBaud);
  delay(1500);
  Serial.println();
  Serial.println("boot: TongDou V9 board no-load test firmware");
  setTestLed(0, 0, 0);
  setupMotorPins();
  setupSpeakerCtrl();
  setupBatteryPins();
  printLedHelp();

  Wire.begin(kI2cSda, kI2cScl);
  Wire.setClock(kI2cClockHz);
  Wire.setTimeOut(50);
  delay(100);

  runFullProbe();
  micReady = beginMic();
}

void loop() {
  if (Serial.available()) {
    const String command = Serial.readStringUntil('\n');
    handleSerialCommand(command);
  }

  updateLedAutoTest();
  reportMicLevel();
  updateBatteryReport();

  if (millis() - lastReportMs >= 1000) {
    lastReportMs = millis();
    readQmiMotion();
  }
}
