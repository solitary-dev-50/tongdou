#pragma once

#include <stdint.h>

namespace tongdou { namespace battery_config {
// 只定义当前电池的运行参数；由驱动写入电量计 RAM，不烧写 OTP。
constexpr uint16_t kDesignCapacityMah = 1200;
constexpr uint16_t kChargingVoltageMv = 4200;
constexpr uint8_t kLowPercent = 10;
constexpr uint8_t kSocDeltaPercent = 1;
constexpr uint32_t kPollIntervalMs = 1000;
constexpr uint32_t kMinimumPollIntervalMs = 500;
constexpr uint32_t kStaleAfterMs = 5000;
constexpr uint32_t kRetryIntervalMs = 30000;
// 电压检测现有分压为 100k / 100k。
constexpr uint32_t kAdcDividerRatio = 2;
// 10mΩ 使用芯片出厂电流校准，不提供电流比例覆盖项。
}}  // namespace tongdou::battery_config
