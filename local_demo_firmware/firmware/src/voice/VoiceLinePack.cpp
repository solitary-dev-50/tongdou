#include "voice/VoiceLinePack.h"

namespace tongdou {

VoiceClip VoiceLinePack::clip(VoiceLine line) const {
  switch (line) {
    case VoiceLine::WakeGreeting:
      return {101, "", "嗨，朋友，你的桌面上是不是缺少一个这样的小混蛋……和你一样的牛马搭子？"};
    case VoiceLine::SnoozeSoft:
      return {102, "", "行吧，十分钟后再抓你。"};
    case VoiceLine::SnoozeTease:
      return {103, "", "你先拖着，我先记着。"};
    case VoiceLine::ConfirmReward:
      return {104, "", "收到。"};
    case VoiceLine::QuietReply:
      return {105, "", "行行行，我闭嘴。"};
    case VoiceLine::BreakReminder:
      return {106, "", "起来动一下吧，再坐下去我都替你的腰着急。"};
    case VoiceLine::OvertimeNudge:
      return {107, "", "这都几点了，还不下班？再晚回家，你老婆不罚你跪才怪……"};
    case VoiceLine::OvertimeCowardReply:
      return {108, "", "不敢不敢，我就是怕你跪久了膝盖疼……明天还得你驮着我加班呢。"};
    case VoiceLine::ReminderDueNudge:
      return {109, "", "到点了，别装没听见，我都替你记着呢。"};
    case VoiceLine::LowBatteryWhine:
      return {110, "", "我快没电了，你再不救我，我就只能安详下线了。"};
    case VoiceLine::ChargingRelief:
      return {111, "", "行，算你还有点良心。"};
    case VoiceLine::RecognitionCoffeeChoke:
      return {112, "", "咳……刚才被咖啡呛了一下。你再说一遍，我这次肯定认真听。"};
    case VoiceLine::DanceShowDomestic:
      return {113, "", "左三下，右三下，铜豆扭扭不白忙。跳完了，拿表演费。要不用奶茶顶也行，嘿嘿。"};
    case VoiceLine::DanceShowGlobal:
      return {114, "demo_06_dance.wav", "This is not dancing. This is a highly advanced wheel calibration protocol. Mm-mm... left wheel, right wheel. Nobody dances better than me. Coffee. Now."};
    case VoiceLine::DemoCheekyCalibrationDance:
      return {121, "demo_06_dance.wav", "This is not dancing. This is a highly advanced wheel calibration protocol. Mm-mm... left wheel, right wheel. Nobody dances better than me. Coffee. Now."};
    case VoiceLine::DemoNightGossipGlobal:
      return {115, "demo_01_late_night_hook.wav", "Still working? Careful, the couch is waiting. I brought popcorn."};
    case VoiceLine::DemoNightCowardGlobal:
      return {116, "demo_02_late_night_tail.wav", "No, no, no. I just wanted to make sure you had entertainment while finding a comfortable blanket."};
    case VoiceLine::DemoMorningSoftGlobal:
      return {117, "demo_03_morning.wav", "Morning. Take it slow today. I will keep an eye on the little things."};
    case VoiceLine::DemoRecordedEmailSummaryGlobal:
      return {118, "demo_05_email_summary.wav", "You got one supplier email. The key point is brass part sampling time. Nothing urgent, but worth checking after this take."};
    case VoiceLine::DemoFirstSummonNightmare:
      return {119, "demo_00_first_summon.wav", "Did you... summon me from the void? Good. Foolish human. Your desktop nightmare... begins now! Gah-ha-ha-ha!"};
    case VoiceLine::DemoCrowdfundingCallGlobal:
      return {120, "demo_07_campaign_call.wav", "The campaign is live. Back me now, or no more dance shows."};
    case VoiceLine::None:
    default:
      return {};
  }
}

}  // namespace tongdou
