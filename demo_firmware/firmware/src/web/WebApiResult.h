#pragma once

#include <stdint.h>

namespace tongdou {

enum class WebApiStatus : uint8_t {
  Ok,
  InvalidRequest,
  StorageError,
  NotImplemented,
};

struct WebApiResult {
  WebApiStatus status = WebApiStatus::Ok;

  bool ok() const {
    return status == WebApiStatus::Ok;
  }
};

}  // namespace tongdou
