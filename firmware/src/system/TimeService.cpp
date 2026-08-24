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

bool TimeService::parseLocalDateTime(const String& value,
                                     uint32_t& epochSeconds) const {
  if (!ready() || (value.length() != 16 && value.length() != 19)) {
    return false;
  }
  if (value[4] != '-' || value[7] != '-' ||
      (value[10] != ' ' && value[10] != 'T') || value[13] != ':' ||
      (value.length() == 19 && value[16] != ':')) {
    return false;
  }

  tm local = {};
  local.tm_year = value.substring(0, 4).toInt() - 1900;
  local.tm_mon = value.substring(5, 7).toInt() - 1;
  local.tm_mday = value.substring(8, 10).toInt();
  local.tm_hour = value.substring(11, 13).toInt();
  local.tm_min = value.substring(14, 16).toInt();
  local.tm_sec = value.length() == 19 ? value.substring(17, 19).toInt() : 0;
  local.tm_isdst = -1;

  if (local.tm_year < 120 || local.tm_mon < 0 || local.tm_mon > 11 ||
      local.tm_mday < 1 || local.tm_mday > 31 || local.tm_hour < 0 ||
      local.tm_hour > 23 || local.tm_min < 0 || local.tm_min > 59 ||
      local.tm_sec < 0 || local.tm_sec > 59) {
    return false;
  }

  const tm requested = local;
  const time_t parsed = mktime(&local);
  if (parsed < kReasonableEpoch ||
      static_cast<uint64_t>(parsed) > UINT32_MAX) {
    return false;
  }

  tm verified = {};
  localtime_r(&parsed, &verified);
  if (verified.tm_year != requested.tm_year ||
      verified.tm_mon != requested.tm_mon ||
      verified.tm_mday != requested.tm_mday ||
      verified.tm_hour != requested.tm_hour ||
      verified.tm_min != requested.tm_min ||
      verified.tm_sec != requested.tm_sec) {
    return false;
  }

  epochSeconds = static_cast<uint32_t>(parsed);
  return true;
}

}  // namespace tongdou
