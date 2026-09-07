#pragma once

#include <Arduino.h>

#include "app/AppCommandService.h"
#include "audio/AudioVolumeService.h"
#include "cloud/DeviceActivationService.h"
#include "cloud/FirmwareUpdateService.h"
#include "hardware/HardwareManager.h"
#include "mcp/McpServer.h"
#include "realtime/LocalAudioPipeline.h"
#include "realtime/RealtimeVoiceService.h"
#include "realtime/VoiceVisualFeedback.h"
#include "reminder/ReminderManager.h"
#include "reminder/ReminderAlertService.h"
#include "reminder/ProactiveReminderService.h"
#include "reminder/ReminderStore.h"
#include "resources/LocaleResourceStore.h"
#include "resources/LocaleResourceUpdateService.h"
#include "scenario/ScenarioEngine.h"
#include "scenario/ScenarioExecutor.h"
#include "scenario/ScenarioLibrary.h"
#include "system/DeviceLocaleStore.h"
#include "system/ServerEndpointStore.h"
#include "system/ProactiveReminderConfigStore.h"
#include "system/TimeService.h"
#include "system/WifiConfigStore.h"
#include "web/ScenarioConfigApi.h"
#include "web/ScenarioConfigStore.h"
#include "web/WebConfigServer.h"
#include "wake/LocalWakeWordDetector.h"

namespace tongdou {

class App {
 public:
  void begin();
  void update();

 private:
  void handleReminderEvents();
  void handleProactiveReminderEvents();
  void handleReminderAlertCompletion();
  void handleLogoTouchEvents();
  void handleLocalWakeWordEvents();
  void updateWakeWordArming();
  void handleConversationWindow();
  void startLogoVoiceTurn();
  void startConversationVoiceTurn();
  void startReminderResponseVoiceTurn();
  void startVoiceTurnFromInteraction(const __FlashStringHelper* source);
  void requestConversationExitFromInteraction(
      const __FlashStringHelper* source);
  void openConversationWindow();
  void closeConversationWindow(bool playExitCue);
  void handleAudioCueEvents();
  void handleRealtimeVoiceEvents();
  void handleFirmwareUpdate();
  void updateLocaleResourceEndpoint();
  void handleLocaleResourceRepair();
  void handleSerialDiagnostics();
  void handleStartupReport();
  void playSystemCue(SystemAudioCue cue, const char* label);
  void playLocalizedCue(LocalizedAudioCue cue, const char* label);
  void playActivationCodeCue(const String& code);
  void logAudioCueResult(const char* label, const AudioCuePlaybackResult& result) const;
  void printStartupReport(const char* title);

  HardwareManager hardware_;
  WifiConfigStore wifiConfigStore_;
  ServerEndpointStore serverEndpointStore_;
  DeviceLocaleStore deviceLocaleStore_;
  LocaleResourceStore localeResourceStore_;
  LocaleResourceUpdateService localeResourceUpdateService_{
      localeResourceStore_};
  TimeService timeService_;
  ReminderStore reminderStore_;
  ReminderManager reminderManager_{timeService_, reminderStore_};
  ScenarioLibrary scenarioLibrary_;
  ScenarioEngine scenarioEngine_{scenarioLibrary_};
  ScenarioExecutor scenarioExecutor_{hardware_.faceDisplay(), hardware_.led(),
                                    hardware_.motors(), hardware_.audioOutput(),
                                    hardware_.imu()};
  ScenarioContext scenarioContext_;
  LocalAudioPipeline localAudioPipeline_{hardware_.audioInput()};
  LocalWakeWordDetector localWakeWordDetector_;
  AudioVolumeService audioVolumeService_{hardware_.audioOutput()};
  RealtimeVoiceService realtimeVoiceService_{localAudioPipeline_,
                                            hardware_.audioOutput()};
  VoiceVisualFeedback voiceVisualFeedback_{realtimeVoiceService_,
                                           hardware_.faceDisplay(),
                                           hardware_.led()};
  ReminderAlertService reminderAlertService_{hardware_.faceDisplay(),
                                             hardware_.audioCuePlayer(),
                                             realtimeVoiceService_};
  ProactiveReminderConfigStore proactiveReminderConfigStore_;
  ProactiveReminderService proactiveReminderService_{
      proactiveReminderConfigStore_.settings()};
  DeviceActivationService deviceActivationService_{realtimeVoiceService_,
                                                  hardware_.faceDisplay(),
                                                  serverEndpointStore_};
  FirmwareUpdateService firmwareUpdateService_{serverEndpointStore_};
  NetworkInfoService networkInfoService_{hardware_.faceDisplay(),
                                        reminderAlertService_, deviceLocaleStore_};
  AppCommandService appCommandService_{scenarioEngine_, scenarioExecutor_, scenarioContext_,
                                      realtimeVoiceService_, audioVolumeService_,
                                      networkInfoService_};
  McpServer mcpServer_{timeService_, reminderStore_, hardware_.selfTest(), appCommandService_};
  ScenarioConfigStore scenarioConfigStore_;
  ScenarioConfigApi scenarioConfigApi_{scenarioConfigStore_};
  WebConfigServer webConfigServer_{scenarioConfigApi_, wifiConfigStore_, timeService_,
                                   reminderStore_, mcpServer_, hardware_.selfTest(),
                                   deviceLocaleStore_,
                                   localeResourceUpdateService_, serverEndpointStore_,
                                   proactiveReminderConfigStore_};
  unsigned long bootMs_ = 0;
  uint8_t startupReportCount_ = 0;
  bool startupReadyCueStarted_ = false;
  bool startupVoiceReady_ = false;
  bool voiceConversationActive_ = false;
  bool conversationFollowUpPending_ = false;
  bool reminderResponseStartPending_ = false;
  bool reminderInterruptedConversation_ = false;
  bool reminderCompletionOnline_ = false;
  bool reminderCompletionCommitFailureLogged_ = false;
  uint32_t reminderCompletionCommitPendingId_ = 0;
  uint32_t reminderResponseId_ = 0;
  unsigned long conversationFollowUpAtMs_ = 0;
  unsigned long reminderResponseStartAtMs_ = 0;
  unsigned long reminderCompletionCommitRetryAtMs_ = 0;
  unsigned long nextLocaleResourceEndpointSyncMs_ = 0;
  unsigned long nextLocaleResourceRepairMs_ = 0;
  unsigned long nextFirmwareUpdateAttemptMs_ = 0;
  bool firmwareUpdateWaitingLogged_ = false;
};

}  // namespace tongdou
