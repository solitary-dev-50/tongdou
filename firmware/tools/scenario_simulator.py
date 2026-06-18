#!/usr/bin/env python3
"""Run Tong Dou scenario decisions without hardware."""

from __future__ import annotations

from dataclasses import dataclass
from enum import IntEnum
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


class FaceAction(IntEnum):
    NONE = 0
    WAKE_UP = 1
    SMILE = 2
    SERIOUS = 3
    ROLL_EYES = 4
    SLEEPY = 5
    WRONGED = 6
    SQUINT = 7
    INNOCENT = 8


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
            FaceAction.SERIOUS,
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
            FaceAction.SMILE,
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
            FaceAction.SMILE,
            LightAction.WARM_WAKE,
            MotionAction.TINY_SHAKE,
            VoiceLine.CHARGING_RELIEF,
            1000,
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


FACE_FRAMES = {
    FaceAction.NONE: (),
    FaceAction.WAKE_UP: (("Sleep", 180), ("HalfOpen", 180), ("Awake", 500)),
    FaceAction.SMILE: (("Smile", 600),),
    FaceAction.SERIOUS: (("Serious", 700),),
    FaceAction.ROLL_EYES: (
        ("RollEyesLeft", 220),
        ("RollEyesRight", 220),
        ("Awake", 260),
    ),
    FaceAction.SLEEPY: (("HalfOpen", 420), ("Sleep", 420)),
    FaceAction.WRONGED: (("Wronged", 700),),
    FaceAction.SQUINT: (("Squint", 300), ("Innocent", 500)),
    FaceAction.INNOCENT: (("Innocent", 600),),
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
    print()


def main() -> None:
    events = (
        ScenarioEventType.BOOT_COMPLETED,
        ScenarioEventType.REMINDER_SNOOZED,
        ScenarioEventType.REMINDER_CONFIRMED,
        ScenarioEventType.QUIET_MODE_REQUESTED,
        ScenarioEventType.USER_IDLE_TOO_LONG,
        ScenarioEventType.OVERTIME_REMINDER_DUE,
        ScenarioEventType.REMINDER_DUE,
        ScenarioEventType.LOW_BATTERY,
        ScenarioEventType.CHARGING_STARTED,
    )
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


if __name__ == "__main__":
    main()
