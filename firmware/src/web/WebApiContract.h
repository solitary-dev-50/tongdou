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
constexpr WebApiRoute kStatus{kMethodGet, "/api/status"};
constexpr WebApiRoute kScenarioConfigGet{kMethodGet, "/api/scenario/config"};
constexpr WebApiRoute kScenarioConfigPut{kMethodPut, "/api/scenario/config"};
constexpr WebApiRoute kScenarioReset{kMethodPost, "/api/scenario/reset"};
constexpr WebApiRoute kScenarioExport{kMethodGet, "/api/scenario/export"};
constexpr WebApiRoute kScenarioImport{kMethodPost, "/api/scenario/import"};
constexpr WebApiRoute kScenarioOptions{kMethodGet, "/api/scenario/options"};
constexpr WebApiRoute kReminderList{kMethodGet, "/api/reminders"};
constexpr WebApiRoute kReminderCreate{kMethodPost, "/api/reminders"};
constexpr WebApiRoute kReminderDelete{kMethodPost, "/api/reminders/delete"};
constexpr WebApiRoute kDiagnostic{kMethodPost, "/api/diagnostic"};

}  // namespace tongdou::web_api
