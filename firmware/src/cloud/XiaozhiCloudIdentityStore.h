#pragma once

#include <Arduino.h>

namespace tongdou {

// 只负责保存小智云所需、重启后不变的客户端编号。
class XiaozhiCloudIdentityStore {
 public:
  bool begin();
  const String& clientId() const;

 private:
  String clientId_;
};

}  // namespace tongdou
