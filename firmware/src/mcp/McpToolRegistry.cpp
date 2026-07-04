#include "mcp/McpToolRegistry.h"

#include <algorithm>
#include <utility>

namespace tongdou {
namespace {

const char* propertyTypeName(McpPropertyType type) {
  switch (type) {
    case McpPropertyType::Boolean:
      return "boolean";
    case McpPropertyType::Integer:
      return "integer";
    case McpPropertyType::String:
    default:
      return "string";
  }
}

std::string serializeDoc(JsonDocument& doc) {
  std::string output;
  serializeJson(doc, output);
  return output;
}

}  // namespace

McpProperty McpProperty::RequiredString(const char* nameValue) {
  McpProperty property;
  property.name = nameValue ? nameValue : "";
  property.type = McpPropertyType::String;
  property.required = true;
  return property;
}

McpProperty McpProperty::RequiredInteger(const char* nameValue) {
  McpProperty property;
  property.name = nameValue ? nameValue : "";
  property.type = McpPropertyType::Integer;
  property.required = true;
  return property;
}

McpProperty McpProperty::OptionalInteger(const char* nameValue) {
  McpProperty property;
  property.name = nameValue ? nameValue : "";
  property.type = McpPropertyType::Integer;
  property.required = false;
  return property;
}

McpToolArguments::McpToolArguments(JsonObjectConst input) : input_(input) {}

int McpToolArguments::getInt(const char* name, int defaultValue) const {
  if (input_.isNull()) {
    return defaultValue;
  }
  return input_[name] | defaultValue;
}

std::string McpToolArguments::getString(const char* name, const char* defaultValue) const {
  if (input_.isNull()) {
    return defaultValue ? defaultValue : "";
  }
  const char* value = input_[name] | defaultValue;
  return value ? value : "";
}

McpTool::McpTool(std::string name, std::string description,
                 std::vector<McpProperty> properties, McpToolCallback callback)
    : name_(std::move(name)),
      description_(std::move(description)),
      properties_(std::move(properties)),
      callback_(std::move(callback)) {}

const std::string& McpTool::name() const {
  return name_;
}

std::string McpTool::toJson() const {
  JsonDocument doc;
  doc["name"] = name_;
  doc["description"] = description_;

  JsonObject schema = doc["inputSchema"].to<JsonObject>();
  schema["type"] = "object";
  JsonObject properties = schema["properties"].to<JsonObject>();
  JsonArray required = schema["required"].to<JsonArray>();

  for (const McpProperty& property : properties_) {
    JsonObject item = properties[property.name].to<JsonObject>();
    item["type"] = propertyTypeName(property.type);
    if (property.required) {
      required.add(property.name);
    }
  }

  return serializeDoc(doc);
}

McpToolResult McpTool::call(JsonObjectConst arguments) const {
  std::string error;
  if (!validate(arguments, error)) {
    return {error, true};
  }
  if (!callback_) {
    return {"tool callback is not configured", true};
  }
  return callback_(McpToolArguments(arguments));
}

bool McpTool::validate(JsonObjectConst arguments, std::string& error) const {
  for (const McpProperty& property : properties_) {
    JsonVariantConst value = arguments[property.name];
    if (value.isNull()) {
      if (property.required) {
        error = "Missing argument: " + property.name;
        return false;
      }
      continue;
    }

    if (property.type == McpPropertyType::Integer && !value.is<int>()) {
      error = "Invalid integer argument: " + property.name;
      return false;
    }
    if (property.type == McpPropertyType::String && !value.is<const char*>()) {
      error = "Invalid string argument: " + property.name;
      return false;
    }
  }
  return true;
}

bool McpToolRegistry::addTool(std::string name, std::string description,
                              std::vector<McpProperty> properties,
                              McpToolCallback callback) {
  const auto found = std::find_if(tools_.begin(), tools_.end(), [&name](const McpTool& tool) {
    return tool.name() == name;
  });
  if (found != tools_.end()) {
    return false;
  }

  tools_.push_back(McpTool(std::move(name), std::move(description), std::move(properties),
                           std::move(callback)));
  return true;
}

std::string McpToolRegistry::buildToolsListJson() const {
  std::string output = "{\"tools\":[";
  for (size_t i = 0; i < tools_.size(); ++i) {
    if (i > 0) {
      output += ",";
    }
    output += tools_[i].toJson();
  }
  output += "]}";
  return output;
}

McpToolResult McpToolRegistry::callTool(const std::string& name,
                                        JsonObjectConst arguments) const {
  const auto found = std::find_if(tools_.begin(), tools_.end(), [&name](const McpTool& tool) {
    return tool.name() == name;
  });
  if (found == tools_.end()) {
    return {"Unknown tool: " + name, true};
  }
  return found->call(arguments);
}

std::string mcpEscapeJsonString(const std::string& value) {
  std::string output;
  output.reserve(value.size() + 8);
  for (char ch : value) {
    switch (ch) {
      case '\\':
        output += "\\\\";
        break;
      case '"':
        output += "\\\"";
        break;
      case '\n':
        output += "\\n";
        break;
      case '\r':
        output += "\\r";
        break;
      case '\t':
        output += "\\t";
        break;
      default:
        output += ch;
        break;
    }
  }
  return output;
}

std::string mcpBuildTextResultJson(const McpToolResult& result) {
  std::string output = "{\"content\":[{\"type\":\"text\",\"text\":\"";
  output += mcpEscapeJsonString(result.text);
  output += "\"}],\"isError\":";
  output += result.isError ? "true" : "false";
  output += "}";
  return output;
}

}  // namespace tongdou
