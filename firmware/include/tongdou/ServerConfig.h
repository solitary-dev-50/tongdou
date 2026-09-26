#pragma once

#ifndef TONGDOU_XIAOZHI_CLOUD
#define TONGDOU_XIAOZHI_CLOUD 0
#endif

#ifndef TONGDOU_OTA_URL
#if TONGDOU_XIAOZHI_CLOUD
#define TONGDOU_OTA_URL "https://api.tenclass.net/xiaozhi/ota/"
#else
#define TONGDOU_OTA_URL ""
#endif
#endif
