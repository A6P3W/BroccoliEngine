#include "PluginManifest.h"

#include <fstream>
#include <limits>
#include <system_error>

#include "nlohmann/json.hpp"

namespace {
bool GetRequiredString(
    const nlohmann::json& Json, const char* Name, std::string& OutValue, std::string& OutError
) {
  const auto Iterator = Json.find(Name);
  if (Iterator == Json.end() || !Iterator->is_string()) {
    OutError = std::string("The '") + Name + "' field must be a string.";
    return false;
  }

  OutValue = Iterator->get<std::string>();
  if (OutValue.empty()) {
    OutError = std::string("The '") + Name + "' field must not be empty.";
    return false;
  }
  return true;
}
}  // namespace

bool ParsePluginManifest(
    const std::filesystem::path& ManifestPath, PluginManifest& OutManifest, std::string& OutError
) {
  OutManifest = {};
  OutError.clear();

  std::ifstream File(ManifestPath);
  if (!File) {
    OutError = "Failed to open manifest file.";
    return false;
  }

  nlohmann::json Json;
  try {
    File >> Json;
  } catch (const nlohmann::json::exception& Exception) {
    OutError = std::string("Failed to parse JSON: ") + Exception.what();
    return false;
  }

  if (!Json.is_object()) {
    OutError = "The manifest root must be an object.";
    return false;
  }
  if (!GetRequiredString(Json, "name", OutManifest.Name, OutError) ||
      !GetRequiredString(Json, "version", OutManifest.Version, OutError)) {
    return false;
  }

  const auto ApiVersionIterator = Json.find("apiVersion");
  if (ApiVersionIterator == Json.end() || !ApiVersionIterator->is_number_unsigned()) {
    OutError = "The 'apiVersion' field must be an unsigned integer.";
    return false;
  }
  const auto ApiVersion = ApiVersionIterator->get<nlohmann::json::number_unsigned_t>();
  if (ApiVersion > std::numeric_limits<uint32_t>::max()) {
    OutError = "The 'apiVersion' field is out of range.";
    return false;
  }
  OutManifest.ApiVersion = static_cast<uint32_t>(ApiVersion);

  std::string LibraryName;
  if (!GetRequiredString(Json, "library", LibraryName, OutError)) return false;
  OutManifest.LibraryPath = LibraryName;

  const auto EnabledIterator = Json.find("enabled");
  if (EnabledIterator != Json.end()) {
    if (!EnabledIterator->is_boolean()) {
      OutError = "The 'enabled' field must be a boolean.";
      return false;
    }
    OutManifest.Enabled = EnabledIterator->get<bool>();
  }
  return true;
}
