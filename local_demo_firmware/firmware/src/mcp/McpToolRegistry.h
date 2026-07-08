#pragma once

#include <ArduinoJson.h>

#include <functional>
#include <string>
#include <vector>

namespace tongdou {

enum class McpPropertyType : uint8_t {
  Boolean,
  Integer,
  String,
};

struct McpProperty {
  std::string name;
  McpPropertyType type = McpPropertyType::String;
  bool required = true;

  static McpProperty RequiredString(const char* name);
  static McpProperty RequiredInteger(const char* name);
  static McpProperty OptionalInteger(const char* name);
};

class McpToolArguments {
 public:
  explicit McpToolArguments(JsonObjectConst input);

  int getInt(const char* name, int defaultValue = 0) const;
  std::string getString(const char* name, const char* defaultValue = "") const;

 private:
  JsonObjectConst input_;
};

struct McpToolResult {
  std::string text;
  bool isError = false;
};

using McpToolCallback = std::function<McpToolResult(const McpToolArguments&)>;

class McpTool {
 public:
  McpTool(std::string name, std::string description, std::vector<McpProperty> properties,
          McpToolCallback callback);

  const std::string& name() const;
  std::string toJson() const;
  McpToolResult call(JsonObjectConst arguments) const;

 private:
  bool validate(JsonObjectConst arguments, std::string& error) const;

  std::string name_;
  std::string description_;
  std::vector<McpProperty> properties_;
  McpToolCallback callback_;
};

class McpToolRegistry {
 public:
  bool addTool(std::string name, std::string description, std::vector<McpProperty> properties,
               McpToolCallback callback);
  std::string buildToolsListJson() const;
  McpToolResult callTool(const std::string& name, JsonObjectConst arguments) const;

 private:
  std::vector<McpTool> tools_;
};

std::string mcpEscapeJsonString(const std::string& value);
std::string mcpBuildTextResultJson(const McpToolResult& result);

}  // namespace tongdou
