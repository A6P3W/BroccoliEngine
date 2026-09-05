#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <vector>

#include "BroccoliEngineAPI.h"

enum class EPluginState {
  Discovered,
  Loaded,
  Active,
  Failed,
  Unloaded,
};

struct FLoadedPlugin;

class BROCCOLI_ENGINE_API PluginHost {
 public:
  static PluginHost& GetInstance();

  ~PluginHost();

  PluginHost(const PluginHost&) = delete;
  PluginHost& operator=(const PluginHost&) = delete;

  bool Initialize(const std::filesystem::path& PluginsDirectory);
  void Update(float DeltaTime);
  void Shutdown();

  bool LoadPlugin(const std::filesystem::path& ManifestPath);

  bool IsInitialized() const;
  size_t GetPluginCount() const;
  size_t GetActivePluginCount() const;

 private:
  PluginHost();

  bool DiscoverPlugins();
  bool ActivatePlugin(FLoadedPlugin& Plugin);
  void DeactivatePlugin(FLoadedPlugin& Plugin);

  std::filesystem::path PluginsDirectory;
  std::vector<std::unique_ptr<FLoadedPlugin>> Plugins;
  bool Initialized = false;
};
