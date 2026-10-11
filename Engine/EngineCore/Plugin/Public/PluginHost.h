#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string_view>
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

  bool Initialize(const std::filesystem::path& ConfigPath);
  void Update(float DeltaTime);
  void Shutdown();

  bool IsPluginActive(std::string_view Name) const;

  bool IsInitialized() const;
  size_t GetPluginCount() const;
  size_t GetActivePluginCount() const;

 private:
  PluginHost();

  bool ActivatePlugin(FLoadedPlugin& Plugin);
  bool DeactivatePlugin(FLoadedPlugin& Plugin);

  std::filesystem::path ConfigPath;
  std::vector<std::unique_ptr<FLoadedPlugin>> Plugins;
  bool Initialized = false;
};
