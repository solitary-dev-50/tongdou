#include "voice/VoiceLinePack.h"

namespace tongdou {

VoiceClip VoiceLinePack::clip(VoiceLine line) const {
  switch (line) {
    case VoiceLine::WakeGreeting:
      return {101, "嗨，朋友，你的桌面上是不是缺少一个这样的小混蛋……和你一样的牛马搭子？"};
    case VoiceLine::SnoozeSoft:
      return {102, "行吧，逃得过初一，逃不过十五。"};
    case VoiceLine::SnoozeTease:
      return {103, "你先拖着，我先记着。"};
    case VoiceLine::ConfirmReward:
      return {104, "可以，可以，今天算你支棱起来了。"};
    case VoiceLine::QuietReply:
      return {105, "行行行，我闭嘴，你凶什么凶。"};
    case VoiceLine::BreakReminder:
      return {106, "起来动一下吧，再坐下去我都替你的腰着急。"};
    case VoiceLine::OvertimeNudge:
      return {107, "这都几点了，还不下班？再晚回家，你老婆不罚你跪才怪……"};
    case VoiceLine::OvertimeCowardReply:
      return {108, "不敢不敢，我就是怕你跪久了膝盖疼……明天还得你驮着我加班呢。"};
    case VoiceLine::ReminderDueNudge:
      return {109, "到点了，别装没听见，我都替你记着呢。"};
    case VoiceLine::LowBatteryWhine:
      return {110, "我快没电了，你再不救我，我就只能安详下线了。"};
    case VoiceLine::ChargingRelief:
      return {111, "行，算你还有点良心。"};
    case VoiceLine::None:
    default:
      return {};
  }
}

}  // namespace tongdou
