#pragma once

namespace tongdou::web_api {

struct WebApiRoute {
  const char* method;
  const char* path;
};

constexpr const char* kMethodGet = "GET";
constexpr const char* kMethodPut = "PUT";
constexpr const char* kMethodPost = "POST";

constexpr WebApiRoute kHealth{kMethodGet, "/api/health"};
constexpr WebApiRoute kScenarioConfigGet{kMethodGet, "/api/scenario/config"};
constexpr WebApiRoute kScenarioConfigPut{kMethodPut, "/api/scenario/config"};
constexpr WebApiRoute kScenarioReset{kMethodPost, "/api/scenario/reset"};
constexpr WebApiRoute kScenarioExport{kMethodGet, "/api/scenario/export"};
constexpr WebApiRoute kScenarioImport{kMethodPost, "/api/scenario/import"};
constexpr WebApiRoute kScenarioOptions{kMethodGet, "/api/scenario/options"};

}  // namespace tongdou::web_api
