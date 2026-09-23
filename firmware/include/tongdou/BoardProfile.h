#pragma once

// 未指定主板时保留 V9.0，兼容已有编译和升级入口。
#ifndef TONGDOU_BOARD_REVISION
#define TONGDOU_BOARD_REVISION 90
#endif

#if TONGDOU_BOARD_REVISION == 90
#define TONGDOU_BOARD_TYPE "tongdou-esp32s3"
#define TONGDOU_BOARD_VERSION "V9.0"
#elif TONGDOU_BOARD_REVISION == 94
#define TONGDOU_BOARD_TYPE "tongdou-esp32s3-v9.4"
#define TONGDOU_BOARD_VERSION "V9.4"
#else
#error "Unsupported Tong Dou board revision"
#endif

namespace tongdou {
constexpr const char* kBoardType = TONGDOU_BOARD_TYPE;
constexpr const char* kBoardVersion = TONGDOU_BOARD_VERSION;
}  // namespace tongdou
