#include "mcp/McpServer.h"

#include <WiFi.h>
#include <mbedtls/base64.h>

namespace tongdou {
namespace {

constexpr const char* kProtocolVersion = "2024-11-05";
constexpr const char* kServerName = "TongDou";
constexpr const char* kServerVersion = "tongdou-local";

std::string serializeDoc(JsonDocument& doc) {
  std::string output;
  serializeJson(doc, output);
  return output;
}

std::string toolErrorJson(const char* code, const char* message) {
  JsonDocument doc;
  doc["ok"] = false;
  doc["code"] = code;
  doc["message"] = message;
  return serializeDoc(doc);
}

}  // namespace

McpServer::McpServer(TimeService& timeService, ReminderStore& reminderStore,
                     HardwareSelfTestService& hardwareSelfTest,
                     AppCommandService& appCommandService)
    : timeService_(timeService),
      reminderStore_(reminderStore),
      hardwareSelfTest_(hardwareSelfTest),
      appCommandService_(appCommandService) {}

void McpServer::begin() {
  if (initialized_) {
    return;
  }

  addTools();
  initialized_ = true;
}

String McpServer::handlePayloadText(const String& payloadText) {
  return handlePayloadText(payloadText, registry_);
}

String McpServer::handleVoicePayloadText(const String& payloadText) {
  return handlePayloadText(payloadText, voiceRegistry_);
}

void McpServer::setReminderResponseContext(uint32_t reminderId) {
  reminderResponseContextId_ = reminderId;
}

void McpServer::clearReminderResponseContext() {
  reminderResponseContextId_ = 0;
}

String McpServer::handlePayloadText(const String& payloadText,
                                    const McpToolRegistry& registry) {
  begin();

  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, payloadText);
  if (error || !doc.is<JsonObjectConst>()) {
    return String(replyError("null", "Invalid JSON-RPC payload").c_str());
  }

  return String(handlePayload(doc.as<JsonObjectConst>(), registry).c_str());
}

void McpServer::addTools() {
  const auto status = [this](const McpToolArguments&) -> McpToolResult {
    return {statusJson(), false};
  };
  const auto create = [this](const McpToolArguments& arguments) -> McpToolResult {
    return createReminder(arguments);
  };
  const auto list = [this](const McpToolArguments&) -> McpToolResult {
    return {remindersJson(), false};
  };
  const auto remove = [this](const McpToolArguments& arguments) -> McpToolResult {
    return deleteReminder(arguments);
  };
  const auto snooze = [this](const McpToolArguments& arguments) -> McpToolResult {
    return snoozeReminder(arguments);
  };
  const auto play = [this](const McpToolArguments& arguments) -> McpToolResult {
    return playScenario(arguments);
  };
  const auto personality = [this](const McpToolArguments& arguments) -> McpToolResult {
    return setPersonality(arguments);
  };
  const auto configureVoice = [this](const McpToolArguments& arguments) -> McpToolResult {
    return configureVoiceBackend(arguments);
  };
  const auto connectVoice = [this](const McpToolArguments& arguments) -> McpToolResult {
    return connectVoiceBackend(arguments);
  };
  const auto voiceDetectTool = [this](const McpToolArguments& arguments) -> McpToolResult {
    return voiceDetect(arguments);
  };
  const auto voiceAbortTool = [this](const McpToolArguments& arguments) -> McpToolResult {
    return voiceAbort(arguments);
  };
  const auto voiceExitTool = [this](const McpToolArguments& arguments) -> McpToolResult {
    return voiceExit(arguments);
  };
  const auto startVoice = [this](const McpToolArguments& arguments) -> McpToolResult {
    return startVoiceTurn(arguments);
  };
  const auto startAutoVoice =
      [this](const McpToolArguments& arguments) -> McpToolResult {
    return startAutoVoiceTurn(arguments);
  };
  const auto voiceStatusTool = [this](const McpToolArguments& arguments) -> McpToolResult {
    return voiceStatus(arguments);
  };
  const auto voiceCodecSelfTestTool =
      [this](const McpToolArguments& arguments) -> McpToolResult {
    return voiceCodecSelfTest(arguments);
  };
  const auto playVoice = [this](const McpToolArguments& arguments) -> McpToolResult {
    return playVoiceResponse(arguments);
  };
  const auto failVoice = [this](const McpToolArguments& arguments) -> McpToolResult {
    return failVoiceTurn(arguments);
  };
  const auto getVolume =
      [this](const McpToolArguments& arguments) -> McpToolResult {
    return getAudioVolume(arguments);
  };
  const auto setVolume = [this](const McpToolArguments& arguments) -> McpToolResult {
    return setAudioVolume(arguments);
  };
  const auto networkInfo = [this](const McpToolArguments& arguments) -> McpToolResult {
    return showNetworkInfo(arguments);
  };
  const auto batteryStatus = [this](const McpToolArguments& arguments) -> McpToolResult {
    return getBatteryStatus(arguments);
  };
  const char* networkInfoDescription =
      "用户想知道铜豆自己的网络访问地址、如何进入铜豆本机控制台、或如何在网络中找到铜豆时调用。"
      "例如‘你的地址是多少’‘我怎么进入你的控制台’‘铜豆我怎么找到你’‘告诉我你的网络地址’，"
      "这些只是示例，按自然语言意思判断，不要求固定口令。不要用于询问现实住址、位置、其他设备或服务器管理后台。"
      "无需参数，每次读取设备当前真实地址并显示在屏幕上，不能根据聊天记录猜测地址。"
      "只有 displayed=true 才能告诉用户地址已经显示；成功后用当前对话语言简短口头确认，"
      "中文可说‘我的地址已经显示在屏幕上了，照着找我就行。’，保持铜豆角色风格。"
      "默认不逐位朗读地址；失败时按实际原因说明并给下一步，不能假装已经显示。";
  registry_.addTool("tongdou.network.show_info", networkInfoDescription, {}, networkInfo);
  voiceRegistry_.addTool("tongdou.network.show_info", networkInfoDescription, {}, networkInfo);

  const char* batteryStatusDescription =
      "用户询问铜豆当前电量、电池电压、充电状态或剩余容量时调用。"
      "必须读取设备当前真实电池快照，不能根据对话记忆猜测。"
      "当电量计不可用时，socPercent、电流、容量、温度和健康度会是 null；"
      "此时只能告诉用户当前电池电压和电量计暂不可用。无需参数。";
  registry_.addTool("tongdou.battery.get_status", batteryStatusDescription, {}, batteryStatus);
  voiceRegistry_.addTool("tongdou.battery.get_status", batteryStatusDescription, {}, batteryStatus);

  // 固定短动作，不向模型开放轮速、持续时间或循环次数。
  const auto addMotionTool = [this](const char* name, const char* description,
                                     MotionAction action) {
    const auto execute = [this, action](const McpToolArguments&) -> McpToolResult {
      const AppCommandResult result = appCommandService_.controlMotion(action);
      return {commandResultJson(result), !result.ok};
    };
    registry_.addTool(name, description, {}, execute);
    voiceRegistry_.addTool(name, description, {}, execute);
  };
  addMotionTool("tongdou.motion.forward",
                "用户明确要求铜豆向前走、前进或往前挪时调用一次。无需参数，只执行一次固定短前移并自动停止。"
                "不支持精确步数或距离，不得通过重复调用拼成长距离。ok=true 只表示动作已启动，不能说已走完几步；失败按真实原因说明。",
                MotionAction::NudgeForward);
  addMotionTool("tongdou.motion.backward",
                "用户明确要求铜豆后退、倒退或往后挪时调用一次。无需参数，只执行一次固定短后移并自动停止。"
                "不支持精确步数或距离，不得通过重复调用拼成长距离。ok=true 只表示动作已启动，不能说已走完几步；失败按真实原因说明。",
                MotionAction::ShrinkBack);
  addMotionTool("tongdou.motion.wiggle",
                "用户明确要求铜豆扭一扭、左右扭动身体时调用一次。无需参数，只执行一次短小左右扭动并自动停止。"
                "不要为普通聊天自动加动作，不要重复调用延长表演。ok=true 只表示动作已启动；失败按真实原因说明。",
                MotionAction::TinyShake);
  addMotionTool("tongdou.motion.stop",
                "用户要求铜豆停止移动、别动、停下电机或停止扭动时立即调用。无需参数，取消当前电机动作和回正，不结束对话、不取消提醒。"
                "即使已经停下也可调用。只有 ok=true 才能确认已停止。语音播报期间不支持同时收音，不能承诺语音即时急停。",
                MotionAction::Stop);

  const std::vector<McpProperty> reminderCreateProperties = {
      McpProperty::OptionalInteger("dueAt"),
      McpProperty::OptionalInteger("delaySeconds"),
      McpProperty::OptionalString("localDateTime"),
      McpProperty::RequiredString("text"),
      McpProperty::OptionalInteger("advanceSeconds"),
      McpProperty::OptionalString("advanceText")};
  const std::vector<McpProperty> reminderDeleteProperties = {
      McpProperty::RequiredInteger("id")};
  const std::vector<McpProperty> reminderSnoozeProperties = {
      McpProperty::OptionalInteger("id"),
      McpProperty::OptionalInteger("dueAt"),
      McpProperty::OptionalInteger("delaySeconds"),
      McpProperty::OptionalString("localDateTime")};
  const std::vector<McpProperty> scenarioPlayProperties = {
      McpProperty::RequiredString("event")};
  const std::vector<McpProperty> personalityProperties = {
      McpProperty::RequiredString("personality")};
  const std::vector<McpProperty> voiceBackendConfigureProperties = {
      McpProperty::RequiredString("host")};
  const std::vector<McpProperty> voiceDetectProperties = {
      McpProperty::RequiredString("text")};
  const std::vector<McpProperty> voiceStartProperties = {
      McpProperty::OptionalInteger("captureMs")};
  const std::vector<McpProperty> voiceResponseProperties = {
      McpProperty::RequiredString("text")};
  const std::vector<McpProperty> voiceFailProperties = {
      McpProperty::RequiredString("reason")};
  const std::vector<McpProperty> audioVolumeProperties = {
      McpProperty::OptionalInteger("volume"),
      McpProperty::OptionalInteger("delta")};

  registry_.addTool("get_status", "Get Tong Dou status.", {}, status);
  registry_.addTool("tongdou.status.get",
                    "Get Tong Dou status, including time, network, and hardware state.",
                    {}, status);

  registry_.addTool("create_reminder",
                    "Create a reminder. Provide exactly one of dueAt, delaySeconds, or localDateTime.",
                    reminderCreateProperties, create);
  registry_.addTool("tongdou.reminder.create",
                    "Create a reminder. Provide exactly one of dueAt, delaySeconds, or localDateTime.",
                    reminderCreateProperties, create);

  registry_.addTool("list_reminders", "List active reminders.", {}, list);
  registry_.addTool("tongdou.reminder.list", "List active reminders.", {}, list);

  registry_.addTool("delete_reminder", "Delete a reminder by id.",
                    reminderDeleteProperties, remove);
  registry_.addTool("tongdou.reminder.delete", "Delete a reminder by id.",
                    reminderDeleteProperties, remove);
  registry_.addTool(
      "snooze_reminder",
      "Snooze a reminder occurrence while keeping its event's other occurrences.",
      reminderSnoozeProperties, snooze);
  registry_.addTool(
      "tongdou.reminder.snooze",
      "Snooze a reminder occurrence while keeping its event's other occurrences.",
      reminderSnoozeProperties, snooze);

  voiceRegistry_.addTool(
      "tongdou.reminder.create",
      "用户要求创建新提醒或新事件时调用。时间参数表示事件正点，必须三选一：相对时间用 delaySeconds，明确本地时间用 localDateTime，只有可信绝对时间戳才用 dueAt。用户说会议、日程在某时开始并要求提前提醒时，必须传 advanceSeconds；text 写正点正文，advanceText 写提前准备正文，设备会同时保留提前和正点两个节点。刚刚播报后用户只说‘几分钟后再提醒我’时禁止调用本工具，必须调用 tongdou.reminder.snooze。只有返回 ok=true 才能确认成功。",
      reminderCreateProperties, create);
  voiceRegistry_.addTool(
      "tongdou.reminder.list",
      "用户询问已经设置了哪些提醒时调用。不要凭对话记忆编造提醒。", {},
      list);
  voiceRegistry_.addTool(
      "tongdou.reminder.delete",
      "用户明确要求取消或删除已有提醒事件时调用。删除任一节点编号会删除同一事件下全部未完成节点，id 必须来自提醒列表或当前对话中刚创建成功的提醒。",
      reminderDeleteProperties, remove);
  voiceRegistry_.addTool(
      "tongdou.reminder.snooze",
      "提醒刚播报完后，用户说‘几分钟后再提醒我’‘稍后再叫我’等延后要求时必须调用。时间参数必须三选一；通常相对时间使用 delaySeconds。提醒回应窗口内不要传 id，设备会使用刚播报的真实提醒上下文；普通对话中只有已经通过列表拿到可靠编号时才传 id。延后只新增同一事件的 snooze 节点，不删除正点提醒。",
      reminderSnoozeProperties, snooze);
  voiceRegistry_.addTool(
      "tongdou.audio.get_volume",
      "用户询问铜豆当前喇叭音量、静音状态或音量百分比时调用。这个工具不需要参数，会读取设备当前真实音量；不要根据聊天记录猜测，也不要用设置工具空参数代替查询。",
      {}, getVolume);
  voiceRegistry_.addTool(
      "tongdou.audio.set_volume",
      "用户明确要求调节铜豆喇叭音量时调用。volume 和 delta 必须二选一：明确百分比、静音时用 volume，范围 0 到 100，静音传 0；大一点或小一点时用 delta，通常分别传 10 或 -10。只有返回 ok=true 才能确认调节成功。不要用空参数查询当前音量，不要用它调节麦克风或语音识别灵敏度。",
      audioVolumeProperties, setVolume);

  registry_.addTool("play_scenario", "Trigger a Tong Dou scenario by event name.",
                    scenarioPlayProperties, play);
  registry_.addTool("tongdou.scenario.play", "Trigger a Tong Dou scenario by event name.",
                    scenarioPlayProperties, play);

  registry_.addTool("set_personality", "Set Tong Dou personality mode.",
                    personalityProperties, personality);
  registry_.addTool("tongdou.personality.set", "Set Tong Dou personality mode.",
                    personalityProperties, personality);

  registry_.addTool("configure_voice_backend",
                    "Configure Tong Dou realtime voice backend connection.",
                    voiceBackendConfigureProperties, configureVoice);
  registry_.addTool("tongdou.voice.backend.configure",
                    "Configure Tong Dou realtime voice backend connection.",
                    voiceBackendConfigureProperties, configureVoice);

  registry_.addTool("connect_voice_backend", "Connect Tong Dou realtime voice backend.",
                    {}, connectVoice);
  registry_.addTool("tongdou.voice.backend.connect",
                    "Connect Tong Dou realtime voice backend.", {}, connectVoice);

  registry_.addTool("voice_detect", "Send text to voice backend using listen detect.",
                    voiceDetectProperties, voiceDetectTool);
  registry_.addTool("tongdou.voice.detect",
                    "Send text to voice backend using listen detect.",
                    voiceDetectProperties, voiceDetectTool);

  registry_.addTool("voice_abort", "Abort current backend speech.", {}, voiceAbortTool);
  registry_.addTool("tongdou.voice.abort", "Abort current backend speech.", {},
                    voiceAbortTool);

  registry_.addTool("voice_exit", "Request a normal server-owned conversation exit.", {},
                    voiceExitTool);
  registry_.addTool("tongdou.voice.exit",
                    "Request a normal server-owned conversation exit.", {},
                    voiceExitTool);

  registry_.addTool("start_voice_turn", "Start a realtime voice turn.",
                    voiceStartProperties,
                    startVoice);
  registry_.addTool("tongdou.voice.start", "Start a realtime voice turn.",
                    voiceStartProperties,
                    startVoice);
  registry_.addTool("tongdou.voice.start_auto",
                    "Start the normal automatic continuous-listening voice turn.", {},
                    startAutoVoice);

  registry_.addTool("voice_status", "Get realtime voice turn status.", {},
                    voiceStatusTool);
  registry_.addTool("tongdou.voice.status", "Get realtime voice turn status.", {},
                    voiceStatusTool);

  registry_.addTool("voice_codec_self_test", "Run local Opus codec self test.", {},
                    voiceCodecSelfTestTool);
  registry_.addTool("tongdou.voice.codec_self_test",
                    "Run local Opus codec self test.", {}, voiceCodecSelfTestTool);

  registry_.addTool("play_voice_response", "Play a server voice response.",
                    voiceResponseProperties, playVoice);
  registry_.addTool("tongdou.voice.play_response", "Play a server voice response.",
                    voiceResponseProperties, playVoice);

  registry_.addTool("fail_voice_turn", "Report a realtime voice failure.",
                    voiceFailProperties, failVoice);
  registry_.addTool("tongdou.voice.fail", "Report a realtime voice failure.",
                    voiceFailProperties, failVoice);

  registry_.addTool(
      "tongdou.audio.get_volume",
      "Get the device's current speaker output volume percentage. This read-only tool takes no arguments.",
      {}, getVolume);
  registry_.addTool(
      "tongdou.audio.set_volume",
      "Set persistent speaker output volume. Provide exactly one of volume (0-100) or delta (-100 to 100, excluding zero).",
      audioVolumeProperties, setVolume);
}

std::string McpServer::handlePayload(JsonObjectConst payload,
                                     const McpToolRegistry& registry) {
  const char* jsonrpc = payload["jsonrpc"] | "";
  const char* method = payload["method"] | "";
  const std::string id = idJson(payload);

  if (strcmp(jsonrpc, "2.0") != 0) {
    return replyError(id, "Invalid JSON-RPC version");
  }

  if (strcmp(method, "initialize") == 0) {
    JsonDocument doc;
    doc["protocolVersion"] = kProtocolVersion;
    doc["capabilities"]["tools"].to<JsonObject>();
    doc["serverInfo"]["name"] = kServerName;
    doc["serverInfo"]["version"] = kServerVersion;
    return replyResult(id, serializeDoc(doc));
  }

  if (strcmp(method, "tools/list") == 0) {
    return replyResult(id, registry.buildToolsListJson());
  }

  if (strcmp(method, "tools/call") == 0) {
    JsonObjectConst params = payload["params"].as<JsonObjectConst>();
    const char* name = params["name"] | "";
    if (name[0] == '\0') {
      return replyError(id, "Missing tool name");
    }

    JsonObjectConst arguments = params["arguments"].as<JsonObjectConst>();
    const McpToolResult result = registry.callTool(name, arguments);
    return replyResult(id, mcpBuildTextResultJson(result));
  }

  return replyError(id, std::string("Unsupported method: ") + method);
}

std::string McpServer::replyResult(const std::string& idJson,
                                   const std::string& resultJson) const {
  std::string payload = "{\"jsonrpc\":\"2.0\",\"id\":";
  payload += idJson.empty() ? "null" : idJson;
  payload += ",\"result\":";
  payload += resultJson;
  payload += "}";
  return payload;
}

std::string McpServer::replyError(const std::string& idJson,
                                  const std::string& message) const {
  std::string payload = "{\"jsonrpc\":\"2.0\",\"id\":";
  payload += idJson.empty() ? "null" : idJson;
  payload += ",\"error\":{\"message\":\"";
  payload += mcpEscapeJsonString(message);
  payload += "\"}}";
  return payload;
}

std::string McpServer::idJson(JsonObjectConst payload) const {
  JsonVariantConst id = payload["id"];
  if (id.isNull()) {
    return "null";
  }
  return serializeVariant(id);
}

std::string McpServer::serializeVariant(JsonVariantConst value) const {
  std::string output;
  serializeJson(value, output);
  return output;
}

bool McpServer::resolveReminderDueAt(const McpToolArguments& arguments,
                                     uint32_t& dueAt,
                                     std::string& errorCode,
                                     std::string& errorMessage) const {
  if (!timeService_.ready()) {
    errorCode = "time_not_ready";
    errorMessage = "device time is not ready";
    return false;
  }
  const bool hasDueAt = arguments.has("dueAt");
  const bool hasDelaySeconds = arguments.has("delaySeconds");
  const bool hasLocalDateTime = arguments.has("localDateTime");
  const uint8_t timeArgumentCount = static_cast<uint8_t>(hasDueAt) +
                                    static_cast<uint8_t>(hasDelaySeconds) +
                                    static_cast<uint8_t>(hasLocalDateTime);
  if (timeArgumentCount != 1) {
    errorCode = "invalid_time";
    errorMessage =
        "provide exactly one of dueAt, delaySeconds, or localDateTime";
    return false;
  }

  dueAt = 0;
  if (hasDueAt) {
    const int value = arguments.getInt("dueAt", 0);
    if (value <= 0) {
      errorCode = "invalid_due_at";
      errorMessage = "dueAt is invalid";
      return false;
    }
    dueAt = static_cast<uint32_t>(value);
  } else if (hasDelaySeconds) {
    const int delaySeconds = arguments.getInt("delaySeconds", 0);
    if (delaySeconds <= 0) {
      errorCode = "invalid_delay";
      errorMessage = "delaySeconds must be positive";
      return false;
    }
    const uint64_t calculated =
        static_cast<uint64_t>(timeService_.now()) + delaySeconds;
    if (calculated > UINT32_MAX) {
      errorCode = "invalid_delay";
      errorMessage = "delaySeconds is too large";
      return false;
    }
    dueAt = static_cast<uint32_t>(calculated);
  } else {
    const String localDateTime(
        arguments.getString("localDateTime").c_str());
    if (!timeService_.parseLocalDateTime(localDateTime, dueAt)) {
      errorCode = "invalid_local_time";
      errorMessage = "localDateTime must use YYYY-MM-DD HH:MM:SS";
      return false;
    }
  }

  if (dueAt <= static_cast<uint32_t>(timeService_.now())) {
    errorCode = "invalid_due_at";
    errorMessage = "dueAt is in the past";
    return false;
  }
  return true;
}

McpToolResult McpServer::createReminder(const McpToolArguments& arguments) {
  const String text(arguments.getString("text").c_str());
  if (text.length() == 0) {
    return {toolErrorJson("invalid_request", "text is required"), true};
  }

  uint32_t dueAt = 0;
  std::string errorCode;
  std::string errorMessage;
  if (!resolveReminderDueAt(arguments, dueAt, errorCode, errorMessage)) {
    return {toolErrorJson(errorCode.c_str(), errorMessage.c_str()), true};
  }

  uint8_t displayBitmap[kReminderBitmapSize] = {};
  const std::string displayBitmapBase64 =
      arguments.getString("displayBitmapBase64");
  const uint8_t* displayBitmapData = nullptr;
  size_t displayBitmapLength = 0;
  if (!displayBitmapBase64.empty()) {
    const int decodeResult = mbedtls_base64_decode(
        displayBitmap, sizeof(displayBitmap), &displayBitmapLength,
        reinterpret_cast<const unsigned char*>(displayBitmapBase64.data()),
        displayBitmapBase64.size());
    if (decodeResult != 0 || displayBitmapLength != kReminderBitmapSize) {
      return {toolErrorJson("invalid_display_bitmap",
                            "display bitmap must be a 128x48 monochrome bitmap"),
              true};
    }
    displayBitmapData = displayBitmap;
  }

  const int advanceSeconds = arguments.getInt("advanceSeconds", 0);
  if (arguments.has("advanceSeconds")) {
    if (advanceSeconds <= 0 ||
        static_cast<uint32_t>(advanceSeconds) >= dueAt ||
        dueAt - static_cast<uint32_t>(advanceSeconds) <=
            static_cast<uint32_t>(timeService_.now())) {
      return {toolErrorJson(
                  "invalid_advance",
                  "advanceSeconds must place the advance reminder in the future before the event"),
              true};
    }
    if (displayBitmapData == nullptr) {
      return {toolErrorJson("event_bitmap_required",
                            "event reminder requires punctual display bitmap"),
              true};
    }

    String advanceText(arguments.getString("advanceText").c_str());
    if (advanceText.length() == 0) {
      advanceText = text;
    }
    uint8_t advanceDisplayBitmap[kReminderBitmapSize] = {};
    size_t advanceDisplayBitmapLength = 0;
    const std::string advanceDisplayBitmapBase64 =
        arguments.getString("advanceDisplayBitmapBase64");
    const int advanceDecodeResult = mbedtls_base64_decode(
        advanceDisplayBitmap, sizeof(advanceDisplayBitmap),
        &advanceDisplayBitmapLength,
        reinterpret_cast<const unsigned char*>(
            advanceDisplayBitmapBase64.data()),
        advanceDisplayBitmapBase64.size());
    if (advanceDisplayBitmapBase64.empty() || advanceDecodeResult != 0 ||
        advanceDisplayBitmapLength != kReminderBitmapSize) {
      return {toolErrorJson(
                  "invalid_advance_display_bitmap",
                  "advance display bitmap must be a 128x48 monochrome bitmap"),
              true};
    }

    ReminderRecord punctualCreated;
    ReminderRecord advanceCreated;
    const uint32_t advanceAt =
        dueAt - static_cast<uint32_t>(advanceSeconds);
    if (!reminderStore_.addEvent(
            dueAt, text, displayBitmapData, displayBitmapLength, advanceAt,
            advanceText, advanceDisplayBitmap, advanceDisplayBitmapLength,
            &punctualCreated, &advanceCreated)) {
      return {toolErrorJson("storage_full",
                            "two reminder slots are required for this event"),
              true};
    }

    JsonDocument doc;
    doc["ok"] = true;
    doc["eventId"] = punctualCreated.eventId;
    doc["type"] = "event";
    doc["punctual"]["id"] = punctualCreated.id;
    doc["punctual"]["dueAt"] = punctualCreated.dueAt;
    doc["punctual"]["text"] = punctualCreated.text;
    doc["advance"]["id"] = advanceCreated.id;
    doc["advance"]["dueAt"] = advanceCreated.dueAt;
    doc["advance"]["text"] = advanceCreated.text;
    return {serializeDoc(doc), false};
  }

  ReminderRecord created;
  if (!reminderStore_.add(dueAt, text, displayBitmapData,
                          displayBitmapLength, &created)) {
    return {toolErrorJson("storage_full", "reminder storage is full"), true};
  }

  JsonDocument doc;
  doc["ok"] = true;
  doc["id"] = created.id;
  doc["eventId"] = created.eventId;
  doc["kind"] = reminderOccurrenceKindName(created.kind);
  doc["dueAt"] = created.dueAt;
  doc["text"] = created.text;
  doc["displayBitmap"] = created.hasDisplayBitmap();
  return {serializeDoc(doc), false};
}

McpToolResult McpServer::snoozeReminder(
    const McpToolArguments& arguments) {
  uint32_t dueAt = 0;
  std::string errorCode;
  std::string errorMessage;
  if (!resolveReminderDueAt(arguments, dueAt, errorCode, errorMessage)) {
    return {toolErrorJson(errorCode.c_str(), errorMessage.c_str()), true};
  }

  const int requestedId = arguments.getInt("id", 0);
  const uint32_t sourceId = requestedId > 0
                                ? static_cast<uint32_t>(requestedId)
                                : reminderResponseContextId_;
  if (sourceId == 0) {
    return {toolErrorJson(
                "reminder_context_missing",
                "no recent reminder context; provide a reminder id"),
            true};
  }

  ReminderRecord created;
  if (!reminderStore_.snooze(sourceId, dueAt, &created)) {
    return {toolErrorJson("reminder_snooze_failed",
                          "source reminder was not found or storage is full"),
            true};
  }
  reminderResponseContextId_ = created.id;

  JsonDocument doc;
  doc["ok"] = true;
  doc["id"] = created.id;
  doc["eventId"] = created.eventId;
  doc["kind"] = reminderOccurrenceKindName(created.kind);
  doc["dueAt"] = created.dueAt;
  doc["text"] = created.text;
  doc["existingEventNodesPreserved"] = true;
  return {serializeDoc(doc), false};
}

McpToolResult McpServer::deleteReminder(const McpToolArguments& arguments) {
  const uint32_t id = static_cast<uint32_t>(arguments.getInt("id", 0));
  const ReminderRecord* target = reminderStore_.find(id);
  const uint32_t eventId =
      target == nullptr ? 0 : (target->eventId == 0 ? target->id : target->eventId);
  if (id == 0 || !reminderStore_.remove(id)) {
    return {toolErrorJson("reminder_not_found", "reminder not found"), true};
  }

  JsonDocument doc;
  doc["ok"] = true;
  doc["id"] = id;
  doc["eventId"] = eventId;
  doc["scope"] = "event";
  return {serializeDoc(doc), false};
}

McpToolResult McpServer::playScenario(const McpToolArguments& arguments) {
  const String event(arguments.getString("event").c_str());
  const AppCommandResult result = appCommandService_.playScenario(event);
  return {commandResultJson(result), !result.ok};
}

McpToolResult McpServer::setPersonality(const McpToolArguments& arguments) {
  const String personality(arguments.getString("personality").c_str());
  const AppCommandResult result = appCommandService_.setPersonality(personality);
  return {commandResultJson(result), !result.ok};
}

McpToolResult McpServer::configureVoiceBackend(const McpToolArguments& arguments) {
  const String host(arguments.getString("host").c_str());
  const int port = arguments.getInt("port", 8100);
  const String path(arguments.getString("path", "/xiaozhi/v1/").c_str());
  const String token(arguments.getString("token").c_str());
  const String deviceId(arguments.getString("deviceId").c_str());
  const String clientId(arguments.getString("clientId", "tongdou-demo").c_str());
  const bool useTls = arguments.getInt("useTls", 0) != 0;
  const AppCommandResult result = appCommandService_.configureVoiceBackend(
      host, port, path, token, deviceId, clientId, useTls);
  return {commandResultJson(result), !result.ok};
}

McpToolResult McpServer::connectVoiceBackend(const McpToolArguments&) {
  const AppCommandResult result = appCommandService_.connectVoiceBackend();
  return {commandResultJson(result), !result.ok};
}

McpToolResult McpServer::voiceDetect(const McpToolArguments& arguments) {
  const String text(arguments.getString("text").c_str());
  const AppCommandResult result = appCommandService_.sendVoiceDetect(text);
  return {commandResultJson(result), !result.ok};
}

McpToolResult McpServer::voiceAbort(const McpToolArguments&) {
  const AppCommandResult result = appCommandService_.abortVoice();
  return {commandResultJson(result), !result.ok};
}

McpToolResult McpServer::voiceExit(const McpToolArguments&) {
  const AppCommandResult result = appCommandService_.exitVoiceConversation();
  return {commandResultJson(result), !result.ok};
}

McpToolResult McpServer::startVoiceTurn(const McpToolArguments& arguments) {
  const int captureMs = arguments.getInt("captureMs", 2500);
  const AppCommandResult result =
      appCommandService_.startVoiceTurn(static_cast<uint16_t>(captureMs));
  return {commandResultJson(result), !result.ok};
}

McpToolResult McpServer::startAutoVoiceTurn(const McpToolArguments&) {
  const AppCommandResult result = appCommandService_.startAutoVoiceTurn();
  return {commandResultJson(result), !result.ok};
}

McpToolResult McpServer::voiceStatus(const McpToolArguments&) {
  const String status = appCommandService_.voiceStatusJson();
  return {status.c_str(), false};
}

McpToolResult McpServer::voiceCodecSelfTest(const McpToolArguments&) {
  const String result = appCommandService_.voiceCodecSelfTestJson();
  return {result.c_str(), false};
}

McpToolResult McpServer::playVoiceResponse(const McpToolArguments& arguments) {
  const String text(arguments.getString("text").c_str());
  const AppCommandResult result = appCommandService_.playVoiceResponse(text);
  return {commandResultJson(result), !result.ok};
}

McpToolResult McpServer::failVoiceTurn(const McpToolArguments& arguments) {
  const String reason(arguments.getString("reason").c_str());
  const AppCommandResult result = appCommandService_.failVoiceTurn(reason);
  return {commandResultJson(result), !result.ok};
}

McpToolResult McpServer::getAudioVolume(const McpToolArguments&) {
  const uint8_t volume = appCommandService_.audioVolume();
  JsonDocument doc;
  doc["ok"] = true;
  doc["volume"] = volume;
  doc["muted"] = volume == 0;
  return {serializeDoc(doc), false};
}

McpToolResult McpServer::setAudioVolume(const McpToolArguments& arguments) {
  const bool hasVolume = arguments.has("volume");
  const bool hasDelta = arguments.has("delta");
  if (hasVolume == hasDelta) {
    return {toolErrorJson("invalid_audio_volume",
                          "provide exactly one of volume or delta"),
            true};
  }

  const uint8_t previousVolume = appCommandService_.audioVolume();
  const AppCommandResult result =
      hasVolume
          ? appCommandService_.setAudioVolume(arguments.getInt("volume"))
          : appCommandService_.adjustAudioVolume(arguments.getInt("delta"));

  JsonDocument doc;
  doc["ok"] = result.ok;
  doc["code"] = result.code;
  doc["message"] = result.message;
  doc["previousVolume"] = previousVolume;
  doc["volume"] = appCommandService_.audioVolume();
  return {serializeDoc(doc), !result.ok};
}

McpToolResult McpServer::showNetworkInfo(const McpToolArguments&) {
  const NetworkInfoResult result = appCommandService_.showNetworkInfo();
  JsonDocument doc;
  doc["ok"] = result.displayed;
  doc["displayed"] = result.displayed;
  doc["code"] = result.code;
  doc["ip"] = result.ip;
  doc["url"] = result.url;
  doc["accessMode"] = result.accessMode;
  if (result.displayed) {
    doc["message"] = "我的地址已经显示在屏幕上了，照着找我就行。";
    doc["hint"] = result.accessMode == "setup_hotspot"
                      ? "先连接铜豆配网热点，再在浏览器打开屏幕上的地址。"
                      : "手机或电脑与铜豆连接同一无线网络，再在浏览器打开屏幕上的地址。";
    doc["displaySeconds"] = 30;
  } else if (result.code == "network_unavailable") {
    doc["message"] = "我还没拿到可用的网络地址，先帮我连上无线网络，再问我一次。";
  } else if (result.code == "reminder_busy") {
    doc["message"] = "屏幕正在显示提醒，等这条提醒说完再问我一次。";
  } else {
    doc["message"] = "地址查到了，但我的屏幕暂时用不了；可以把返回的真实地址告诉用户，在同一网络的浏览器里打开。";
  }
  return {serializeDoc(doc), !result.displayed};
}

McpToolResult McpServer::getBatteryStatus(const McpToolArguments&) {
  const String status = appCommandService_.batteryStatusJson();
  return {status.c_str(), false};
}

std::string McpServer::commandResultJson(const AppCommandResult& result) const {
  JsonDocument doc;
  doc["ok"] = result.ok;
  doc["code"] = result.code;
  doc["message"] = result.message;
  doc["personality"] = appCommandService_.personalityName();
  return serializeDoc(doc);
}

std::string McpServer::remindersJson() const {
  JsonDocument doc;
  JsonArray array = doc.to<JsonArray>();
  const ReminderRecord* records = reminderStore_.records();
  for (uint8_t i = 0; i < reminderStore_.capacity(); ++i) {
    const ReminderRecord& record = records[i];
    if (!record.active || record.completed) {
      continue;
    }

    JsonObject item = array.add<JsonObject>();
    item["id"] = record.id;
    item["eventId"] = record.eventId;
    item["kind"] = reminderOccurrenceKindName(record.kind);
    item["dueAt"] = record.dueAt;
    item["completed"] = record.completed;
    item["displayBitmap"] = record.hasDisplayBitmap();
    item["text"] = record.text;
  }
  return serializeDoc(doc);
}

std::string McpServer::statusJson() const {
  const HardwareDiagnosticStatus hardware = hardwareSelfTest_.status();
  uint8_t activeReminders = 0;
  const ReminderRecord* records = reminderStore_.records();
  for (uint8_t i = 0; i < reminderStore_.capacity(); ++i) {
    if (records[i].active && !records[i].completed) {
      ++activeReminders;
    }
  }

  JsonDocument doc;
  doc["wifiConnected"] = WiFi.status() == WL_CONNECTED;
  doc["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "";
  doc["rssi"] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
  doc["timeReady"] = timeService_.ready();
  doc["now"] = timeService_.ready() ? timeService_.isoNow() : "";
  doc["systemMode"] = hardware.degraded ? "degraded" : "normal";
  doc["personality"] = appCommandService_.personalityName();
  doc["audioVolume"] = appCommandService_.audioVolume();
  doc["activeReminders"] = activeReminders;
  doc["reminderCapacity"] = reminderStore_.capacity();
  doc["hardware"]["face"] = hardware.faceReady;
  doc["hardware"]["led"] = hardware.ledReady;
  doc["hardware"]["audioInput"] = hardware.audioInputReady;
  doc["hardware"]["audioOutput"] = hardware.audioOutputReady;
  doc["hardware"]["motorFault"] = hardware.motorFaultActive;
  doc["hardware"]["motorFaultSimulated"] = hardware.motorFaultSimulated;
  doc["hardware"]["motorAvailable"] = hardware.motorAvailable;
  doc["hardware"]["degraded"] = hardware.degraded;
  doc["capabilities"]["reminders"] = hardware.remindersAvailable;
  doc["capabilities"]["web"] = hardware.webAvailable;
  doc["capabilities"]["visual"] = hardware.visualFeedbackAvailable;
  doc["capabilities"]["sound"] = hardware.soundAvailable;
  doc["capabilities"]["voiceInput"] = hardware.voiceInputAvailable;
  return serializeDoc(doc);
}

}  // namespace tongdou
