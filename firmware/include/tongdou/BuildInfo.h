#pragma once

#include "tongdou/ServerConfig.h"

#ifndef TONGDOU_FIRMWARE_VERSION
#define TONGDOU_FIRMWARE_VERSION "0.1.0"
#endif

namespace tongdou {

constexpr const char* kProjectName = "Tong Dou";
constexpr const char* kFirmwareVersion = TONGDOU_FIRMWARE_VERSION;
#if TONGDOU_XIAOZHI_CLOUD
constexpr const char* kFirmwareStage = "xiaozhi-cloud";
#else
constexpr const char* kFirmwareStage = "product-cleanup";
#endif

}  // namespace tongdou
