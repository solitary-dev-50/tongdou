#include "system/TimeService.h"

#include <sys/time.h>

namespace tongdou {
namespace {

constexpr const char* kTimezone = "CST-8";
constexpr const char* kNtpServer1 = "ntp.aliyun.com";
constexpr const char* kNtpServer2 = "cn.pool.ntp.org";
constexpr const char* kNtpServer3 = "pool.ntp.org";
constexpr time_t kReasonableEpoch = 1700000000;

}  // namespace

void TimeService::begin() {}

void TimeService::update() {}

void TimeService::configureNetworkTime() {
  if (configuredNetworkTime_) {
    return;
  }

  configTzTime(kTimezone, kNtpServer1, kNtpServer2, kNtpServer3);
  configuredNetworkTime_ = true;
}

bool TimeService::setEpoch(uint32_t epochSeconds) {
  if (epochSeconds < static_cast<uint32_t>(kReasonableEpoch)) {
    return false;
  }

  timeval value = {};
  value.tv_sec = static_cast<time_t>(epochSeconds);
  settimeofday(&value, nullptr);
  return true;
}

bool TimeService::ready() const {
  return now() >= kReasonableEpoch;
}

time_t TimeService::now() const {
  return time(nullptr);
}

String TimeService::isoNow() const {
  const time_t current = now();
  tm local = {};
  localtime_r(&current, &local);

  char buffer[24] = {};
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local);
  return String(buffer);
}

}  // namespace tongdou
