#include "app/App.h"

#include <WiFi.h>

#include "face/FaceExpression.h"
#include "light/LightTypes.h"
#include "tongdou/BuildInfo.h"

namespace tongdou {
namespace {

constexpr unsigned long kFirstStartupReportMs = 2500;
constexpr unsigned long kSecondStartupReportMs = 8000;

}  // namespace

void App::begin() {
  Serial.begin(115200);
  bootMs_ = millis();
  hardware_.begin();

  delay(200);
  Serial.println();
  Serial.print(kProjectName);
  Serial.print(" firmware stage: ");
  Serial.println(kFirmwareStage);

  webConfigServer_.begin();

  hardware_.faceDisplay().show(FaceExpression::Awake);
  hardware_.led().off();

  Serial.println("tongdou board test initialized");
}

void App::update() {
  webConfigServer_.update();
  hardware_.update();
  handleStartupReport();
  handleSerialDiagnostics();
  handleLogoTouchEvents();
}

void App::handleLogoTouchEvents() {
  const LogoTouchEvent event = hardware_.logoTouch().consumeEvent();
  if (event == LogoTouchEvent::None) {
    return;
  }

  if (event == LogoTouchEvent::LongPress) {
    logoSleepMode_ = !logoSleepMode_;
    hardware_.faceDisplay().show(logoSleepMode_ ? FaceExpression::Sleep
                                                : FaceExpression::Awake);
    if (logoSleepMode_) {
      hardware_.led().off();
    } else {
      hardware_.led().show({12, 10, 4});
    }
    Serial.println(logoSleepMode_ ? "logo action=long_press_sleep"
                                  : "logo action=long_press_wake");
  } else if (event == LogoTouchEvent::DoubleTap) {
    hardware_.faceDisplay().show(FaceExpression::Proud);
    hardware_.led().show({0, 16, 18});
    Serial.println("logo action=double_tap_indicator");
  } else {
    hardware_.faceDisplay().show(FaceExpression::Blink);
    hardware_.led().show({8, 8, 8});
    Serial.println("logo action=single_tap_blink");
  }
}

void App::handleSerialDiagnostics() {
  if (!Serial.available()) {
    return;
  }

  const String command = Serial.readStringUntil('\n');
  if (command.length() == 0) {
    return;
  }

  String trimmed = command;
  trimmed.trim();
  if (trimmed.length() == 0) {
    return;
  }

  if (hardware_.selfTest().handleCommand(command, Serial)) {
    return;
  }

  Serial.println("unknown command, type help");
}

void App::handleStartupReport() {
  const unsigned long elapsed = millis() - bootMs_;
  if (startupReportCount_ == 0 && elapsed >= kFirstStartupReportMs) {
    printStartupReport("startup report");
    startupReportCount_ = 1;
    return;
  }

  if (startupReportCount_ == 1 && elapsed >= kSecondStartupReportMs) {
    printStartupReport("startup report repeat");
    startupReportCount_ = 2;
  }
}

void App::printStartupReport(const char* title) {
  Serial.println();
  Serial.println(title);
  Serial.print(kProjectName);
  Serial.print(" firmware stage: ");
  Serial.println(kFirmwareStage);
  hardware_.printStartupReport(Serial);
  Serial.println("application begin");
  Serial.println("  board test web server started");
  Serial.println("  serial hardware commands enabled");
  Serial.print("  wifi ");
  if (WiFi.isConnected()) {
    Serial.print("connected ip=");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("not connected");
  }
  Serial.println("application ready");
}
}  // namespace tongdou
