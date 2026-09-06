#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

struct PluginManifest {
  std::string Name;
  std::string Version;
  uint32_t ApiVersion = 0;
  std::filesystem::path LibraryPath;
  bool Enabled = true;
};

bool ParsePluginManifest(
    const std::filesystem::path& ManifestPath, PluginManifest& OutManifest, std::string& OutError
);
