#!/usr/bin/env python3
"""Run Tong Dou scenario decisions without hardware."""

from __future__ import annotations

import argparse
import json
from dataclasses import dataclass
from enum import IntEnum
from pathlib import Path
from typing import Iterable


class PersonalityStyle(IntEnum):
    GENTLE = 0
    BALANCED = 1
    DRAMATIC = 2


class ScenarioEventType(IntEnum):
    BOOT_COMPLETED = 0
    USER_ARRIVED = 1
    USER_LEFT = 2
    USER_IDLE_TOO_LONG = 3
    REMINDER_DUE = 4
    REMINDER_CONFIRMED = 5
    REMINDER_SNOOZED = 6
    QUIET_MODE_REQUESTED = 7
    LOW_BATTERY = 8
    CHARGING_STARTED = 9
    OVERTIME_REMINDER_DUE = 10
    VOICE_RECOGNITION_FAILED = 11


class FaceAction(IntEnum):
    NONE = 0
    SLEEP = 1
    WAKE_UP = 2
    AWAKE = 3
    BLINK = 4
    SMILE = 5
    SERIOUS = 6
    ROLL_EYES = 7
    SLEEPY = 8
    WRONGED = 9
    SQUINT = 10
    INNOCENT = 11
    CONFUSED = 12
    ANGRY = 13
    SURPRISED = 14
    SHY = 15
    FIERCE = 16
    PROUD = 17
    NERVOUS = 18


class LightAction(IntEnum):
    NONE = 0
    OFF = 1
    SOFT_WHITE = 2
    WARM_WAKE = 3
    RED_SHORT_BLINK = 4
    WEAK_BREATH = 5
    DIM_WARM = 6


class MotionAction(IntEnum):
    NONE = 0
    STOP = 1
    NOD = 2
    TINY_SHAKE = 3
    NUDGE_FORWARD = 4
    LEAN_FORWARD = 5
    SHRINK_BACK = 6


class VoiceLine(IntEnum):
    NONE = 0
    WAKE_GREETING = 1
    SNOOZE_SOFT = 2
    SNOOZE_TEASE = 3
    CONFIRM_REWARD = 4
    QUIET_REPLY = 5
    BREAK_REMINDER = 6
    OVERTIME_NUDGE = 7
    OVERTIME_COWARD_REPLY = 8
    REMINDER_DUE_NUDGE = 9
    LOW_BATTERY_WHINE = 10
    CHARGING_RELIEF = 11
    RECOGNITION_COFFEE_CHOKE = 12


@dataclass(frozen=True)
class ScenarioContext:
    quiet_mode: bool = False
    low_battery: bool = False
    user_present: bool = False
    charging: bool = False
    snooze_count: int = 0
    personality: PersonalityStyle = PersonalityStyle.BALANCED


@dataclass(frozen=True)
class ScenarioPlan:
    valid: bool = False
    face: FaceAction = FaceAction.NONE
    light: LightAction = LightAction.NONE
    motion: MotionAction = MotionAction.NONE
    voice: VoiceLine = VoiceLine.NONE
    duration_ms: int = 0


@dataclass(frozen=True)
class ScenarioRule:
    event_type: ScenarioEventType
    allow_in_quiet_mode: bool
    allow_when_low_battery: bool
    minimum_personality: PersonalityStyle
    plan: ScenarioPlan


SCENARIO_RULES = (
    ScenarioRule(
        ScenarioEventType.BOOT_COMPLETED,
        False,
        True,
        PersonalityStyle.GENTLE,
        ScenarioPlan(
            True,
            FaceAction.WAKE_UP,
            LightAction.WARM_WAKE,
            MotionAction.TINY_SHAKE,
            VoiceLine.WAKE_GREETING,
            1200,
        ),
    ),
    ScenarioRule(
        ScenarioEventType.REMINDER_SNOOZED,
        False,
        True,
        PersonalityStyle.GENTLE,
        ScenarioPlan(
            True,
            FaceAction.ROLL_EYES,
            LightAction.SOFT_WHITE,
            MotionAction.NONE,
            VoiceLine.SNOOZE_SOFT,
            1000,
        ),
    ),
    ScenarioRule(
        ScenarioEventType.REMINDER_DUE,
        False,
        True,
        PersonalityStyle.GENTLE,
        ScenarioPlan(
            True,
            FaceAction.FIERCE,
            LightAction.SOFT_WHITE,
            MotionAction.NUDGE_FORWARD,
            VoiceLine.REMINDER_DUE_NUDGE,
            1200,
        ),
    ),
    ScenarioRule(
        ScenarioEventType.REMINDER_CONFIRMED,
        False,
        True,
        PersonalityStyle.GENTLE,
        ScenarioPlan(
            True,
            FaceAction.PROUD,
            LightAction.SOFT_WHITE,
            MotionAction.NOD,
            VoiceLine.CONFIRM_REWARD,
            1000,
        ),
    ),
    ScenarioRule(
        ScenarioEventType.QUIET_MODE_REQUESTED,
        True,
        True,
        PersonalityStyle.GENTLE,
        ScenarioPlan(
            True,
            FaceAction.WRONGED,
            LightAction.OFF,
            MotionAction.STOP,
            VoiceLine.QUIET_REPLY,
            800,
        ),
    ),
    ScenarioRule(
        ScenarioEventType.USER_IDLE_TOO_LONG,
        False,
        True,
        PersonalityStyle.GENTLE,
        ScenarioPlan(
            True,
            FaceAction.SERIOUS,
            LightAction.WEAK_BREATH,
            MotionAction.NONE,
            VoiceLine.BREAK_REMINDER,
            1200,
        ),
    ),
    ScenarioRule(
        ScenarioEventType.LOW_BATTERY,
        True,
        True,
        PersonalityStyle.GENTLE,
        ScenarioPlan(
            True,
            FaceAction.WRONGED,
            LightAction.RED_SHORT_BLINK,
            MotionAction.SHRINK_BACK,
            VoiceLine.LOW_BATTERY_WHINE,
            1000,
        ),
    ),
    ScenarioRule(
        ScenarioEventType.CHARGING_STARTED,
        True,
        True,
        PersonalityStyle.GENTLE,
        ScenarioPlan(
            True,
            FaceAction.SHY,
            LightAction.WARM_WAKE,
            MotionAction.TINY_SHAKE,
            VoiceLine.CHARGING_RELIEF,
            1000,
        ),
    ),
    ScenarioRule(
        ScenarioEventType.VOICE_RECOGNITION_FAILED,
        True,
        True,
        PersonalityStyle.GENTLE,
        ScenarioPlan(
            True,
            FaceAction.NERVOUS,
            LightAction.RED_SHORT_BLINK,
            MotionAction.SHRINK_BACK,
            VoiceLine.RECOGNITION_COFFEE_CHOKE,
            1200,
        ),
    ),
    ScenarioRule(
        ScenarioEventType.OVERTIME_REMINDER_DUE,
        False,
        True,
        PersonalityStyle.BALANCED,
        ScenarioPlan(
            True,
            FaceAction.SQUINT,
            LightAction.DIM_WARM,
            MotionAction.LEAN_FORWARD,
            VoiceLine.OVERTIME_NUDGE,
            1800,
        ),
    ),
)


PACK_INFO = {
    "formatVersion": 1,
    "id": "tongdou.default.v1",
    "name": "Default Tong Dou",
    "description": "First built-in pack for warm, cheeky desk companion behavior.",
    "defaultPersonality": "balanced",
}


FACE_FRAMES = {
    FaceAction.NONE: (),
    FaceAction.SLEEP: (("Sleep", 800),),
    FaceAction.WAKE_UP: (
        ("Sleep", 180),
        ("HalfOpen", 220),
        ("Awake", 300),
        ("Blink", 70),
        ("HalfOpen", 120),
        ("Awake", 380),
    ),
    FaceAction.AWAKE: (("Awake", 800),),
    FaceAction.BLINK: (
        ("Awake", 120),
        ("HalfOpen", 80),
        ("Blink", 70),
        ("HalfOpen", 80),
        ("Awake", 240),
    ),
    FaceAction.SMILE: (("Smile", 600),),
    FaceAction.SERIOUS: (("Serious", 700),),
    FaceAction.ROLL_EYES: (
        ("RollEyesLeft", 220),
        ("RollEyesRight", 220),
        ("Awake", 260),
    ),
    FaceAction.SLEEPY: (("HalfOpen", 420), ("Sleep", 420)),
    FaceAction.WRONGED: (("Wronged", 700),),
    FaceAction.SQUINT: (("Awake", 160), ("Squint", 1200)),
    FaceAction.INNOCENT: (("Innocent", 600),),
    FaceAction.CONFUSED: (
        ("Awake", 120),
        ("Confused", 360),
        ("Wronged", 260),
        ("Innocent", 420),
    ),
    FaceAction.ANGRY: (("Awake", 100), ("Angry", 760)),
    FaceAction.SURPRISED: (
        ("Awake", 80),
        ("Surprised", 520),
        ("Blink", 80),
        ("Awake", 240),
    ),
    FaceAction.SHY: (("Awake", 120), ("Shy", 760)),
    FaceAction.FIERCE: (("Squint", 100), ("Fierce", 820)),
    FaceAction.PROUD: (("Awake", 120), ("Proud", 760)),
    FaceAction.NERVOUS: (
        ("Squint", 120),
        ("Nervous", 420),
        ("Wronged", 360),
    ),
}


LIGHT_FRAMES = {
    LightAction.NONE: (),
    LightAction.OFF: (((0, 0, 0), 0),),
    LightAction.SOFT_WHITE: (((28, 26, 20), 320),),
    LightAction.WARM_WAKE: (
        ((0, 0, 0), 80),
        ((8, 4, 1), 120),
        ((18, 10, 3), 160),
        ((32, 18, 5), 420),
    ),
    LightAction.RED_SHORT_BLINK: (
        ((42, 0, 0), 120),
        ((0, 0, 0), 80),
        ((42, 0, 0), 140),
        ((0, 0, 0), 0),
    ),
    LightAction.WEAK_BREATH: (
        ((4, 4, 4), 260),
        ((10, 9, 7), 420),
        ((3, 3, 3), 520),
    ),
    LightAction.DIM_WARM: (((12, 6, 2), 600),),
}


MOTION_FRAMES = {
    MotionAction.NONE: (),
    MotionAction.STOP: ((("stop", "stop"), 0),),
    MotionAction.NOD: (
        (("forward", "forward"), 80),
        (("reverse", "reverse"), 80),
        (("stop", "stop"), 0),
    ),
    MotionAction.TINY_SHAKE: (
        (("forward", "reverse"), 70),
        (("reverse", "forward"), 70),
        (("stop", "stop"), 0),
    ),
    MotionAction.NUDGE_FORWARD: (
        (("forward", "forward"), 100),
        (("stop", "stop"), 0),
    ),
    MotionAction.LEAN_FORWARD: (
        (("forward", "forward"), 140),
        (("stop", "stop"), 0),
    ),
    MotionAction.SHRINK_BACK: (
        (("reverse", "reverse"), 120),
        (("stop", "stop"), 0),
    ),
}


VOICE_CLIPS = {
    VoiceLine.NONE: (0, ""),
    VoiceLine.WAKE_GREETING: (
        101,
        "嗨，朋友，你的桌面上是不是缺少一个这样的小混蛋……和你一样的牛马搭子？",
    ),
    VoiceLine.SNOOZE_SOFT: (102, "行吧，十分钟后再抓你。"),
    VoiceLine.SNOOZE_TEASE: (103, "你先拖着，我先记着。"),
    VoiceLine.CONFIRM_REWARD: (104, "收到。"),
    VoiceLine.QUIET_REPLY: (105, "行行行，我闭嘴。"),
    VoiceLine.BREAK_REMINDER: (106, "起来动一下吧，再坐下去我都替你的腰着急。"),
    VoiceLine.OVERTIME_NUDGE: (
        107,
        "这都几点了，还不下班？再晚回家，你老婆不罚你跪才怪……",
    ),
    VoiceLine.OVERTIME_COWARD_REPLY: (
        108,
        "不敢不敢，我就是怕你跪久了膝盖疼……明天还得你驮着我加班呢。",
    ),
    VoiceLine.REMINDER_DUE_NUDGE: (109, "到点了，别装没听见，我都替你记着呢。"),
    VoiceLine.LOW_BATTERY_WHINE: (
        110,
        "我快没电了，你再不救我，我就只能安详下线了。",
    ),
    VoiceLine.CHARGING_RELIEF: (111, "行，算你还有点良心。"),
    VoiceLine.RECOGNITION_COFFEE_CHOKE: (
        112,
        "咳……刚才被咖啡呛了一下。你再说一遍，我这次肯定认真听。",
    ),
}


def select_plan(event_type: ScenarioEventType, context: ScenarioContext) -> ScenarioPlan:
    for rule in SCENARIO_RULES:
        if rule.event_type != event_type:
            continue
        if context.quiet_mode and not rule.allow_in_quiet_mode:
            continue
        if context.low_battery and not rule.allow_when_low_battery:
            continue
        if context.personality < rule.minimum_personality:
            continue
        return rule.plan
    return ScenarioPlan()


def enum_name(value: IntEnum) -> str:
    return value.name.lower().replace("_", "-")


def print_context(context: ScenarioContext) -> None:
    print(
        "  context:"
        f" personality={enum_name(context.personality)}"
        f" quiet={context.quiet_mode}"
        f" low_battery={context.low_battery}"
        f" user_present={context.user_present}"
        f" charging={context.charging}"
        f" snooze_count={context.snooze_count}"
    )


def print_plan(plan: ScenarioPlan) -> None:
    if not plan.valid:
        print("  plan: no matched scenario")
        return
    print(
        "  plan:"
        f" face={enum_name(plan.face)}"
        f" light={enum_name(plan.light)}"
        f" motion={enum_name(plan.motion)}"
        f" voice={enum_name(plan.voice)}"
        f" duration_ms={plan.duration_ms}"
    )


def print_face_frames(action: FaceAction) -> None:
    frames = FACE_FRAMES[action]
    if not frames:
        print("  face_frames: none")
        return
    print("  face_frames:")
    for expression, duration_ms in frames:
        print(f"    - {expression} {duration_ms}ms")


def print_light_frames(action: LightAction) -> None:
    frames = LIGHT_FRAMES[action]
    if not frames:
        print("  light_frames: none")
        return
    print("  light_frames:")
    for color, duration_ms in frames:
        print(f"    - rgb{color} {duration_ms}ms")


def print_motion_frames(action: MotionAction) -> None:
    frames = MOTION_FRAMES[action]
    if not frames:
        print("  motion_frames: none")
        return
    print("  motion_frames:")
    for wheels, duration_ms in frames:
        print(f"    - left={wheels[0]} right={wheels[1]} {duration_ms}ms")


def print_voice_clip(line: VoiceLine) -> None:
    clip_id, text = VOICE_CLIPS[line]
    if clip_id == 0:
        print("  voice_clip: none")
        return
    print(f"  voice_clip: id={clip_id} text={text}")


def run_case(label: str, context: ScenarioContext, events: Iterable[ScenarioEventType]) -> None:
    print(f"== case: {label} ==")
    print_context(context)
    for event_type in events:
        print(f"\n[event] {enum_name(event_type)}")
        plan = select_plan(event_type, context)
        print_plan(plan)
        if plan.valid:
            print_face_frames(plan.face)
            print_light_frames(plan.light)
            print_motion_frames(plan.motion)
            print_voice_clip(plan.voice)
    print()


def rule_to_dict(rule: ScenarioRule) -> dict[str, object]:
    plan = rule.plan
    return {
        "enabled": plan.valid,
        "event": enum_name(rule.event_type),
        "allowInQuietMode": rule.allow_in_quiet_mode,
        "allowWhenLowBattery": rule.allow_when_low_battery,
        "minimumPersonality": enum_name(rule.minimum_personality),
        "plan": {
            "face": enum_name(plan.face),
            "light": enum_name(plan.light),
            "motion": enum_name(plan.motion),
            "voice": enum_name(plan.voice),
            "durationMs": plan.duration_ms,
        },
    }


def scenario_pack_to_dict() -> dict[str, object]:
    return {
        "pack": PACK_INFO,
        "rules": [rule_to_dict(rule) for rule in SCENARIO_RULES],
    }


def export_pack(path: str) -> None:
    content = json.dumps(scenario_pack_to_dict(), ensure_ascii=False, indent=2)
    if path == "-":
        print(content)
        return

    target = Path(path)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(content + "\n", encoding="utf-8")
    print(f"exported scenario pack: {target}")


def default_events() -> tuple[ScenarioEventType, ...]:
    return (
        ScenarioEventType.BOOT_COMPLETED,
        ScenarioEventType.REMINDER_SNOOZED,
        ScenarioEventType.REMINDER_CONFIRMED,
        ScenarioEventType.QUIET_MODE_REQUESTED,
        ScenarioEventType.USER_IDLE_TOO_LONG,
        ScenarioEventType.OVERTIME_REMINDER_DUE,
        ScenarioEventType.REMINDER_DUE,
        ScenarioEventType.LOW_BATTERY,
        ScenarioEventType.CHARGING_STARTED,
        ScenarioEventType.VOICE_RECOGNITION_FAILED,
    )


def run_all_cases() -> None:
    events = default_events()
    run_case(
        "normal",
        ScenarioContext(user_present=True, personality=PersonalityStyle.BALANCED),
        events,
    )
    run_case(
        "quiet",
        ScenarioContext(
            quiet_mode=True,
            user_present=True,
            personality=PersonalityStyle.BALANCED,
        ),
        events,
    )
    run_case(
        "low-battery",
        ScenarioContext(
            low_battery=True,
            user_present=True,
            personality=PersonalityStyle.BALANCED,
        ),
        events,
    )
    run_case(
        "gentle-personality",
        ScenarioContext(user_present=True, personality=PersonalityStyle.GENTLE),
        events,
    )


def main() -> None:
    parser = argparse.ArgumentParser(description="Run or export Tong Dou scenarios.")
    parser.add_argument(
        "--export-pack",
        metavar="PATH",
        help="Export the built-in scenario pack as JSON. Use '-' for stdout.",
    )
    args = parser.parse_args()

    if args.export_pack:
        export_pack(args.export_pack)
        return

    run_all_cases()


if __name__ == "__main__":
    main()
