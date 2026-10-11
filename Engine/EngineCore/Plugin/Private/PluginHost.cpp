#include "PluginHost.h"

#include <exception>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>
#include <system_error>
#include <utility>

#include "ActorRegistry.h"
#include "ComponentRegistry.h"
#include "Detail/AutomationRegistrationContext.h"
#include "IPlugin.h"
#include "Log.h"
#include "NativeLibrary.h"
#include "NetworkManager.h"
#include "NetworkTransportRegistry.h"
#include "PluginAPI.h"
#include "PluginContext.h"
#include "Reflection.h"
#include "nlohmann/json.hpp"

struct FLoadedPlugin {
  std::filesystem::path ManifestPath;
  std::string Name;
  NativeLibrary Library;
  IPlugin* Instance = nullptr;
  FDestroyBroccoliPluginFunction DestroyFunction = nullptr;
  EPluginState State = EPluginState::Discovered;
  std::string LastError;
  std::string ModuleOwner;
  bool UnloadCallbackPending = false;
};

namespace {
void SetPluginError(FLoadedPlugin& Plugin, std::string Error) {
  Plugin.State = EPluginState::Failed;
  Plugin.LastError = std::move(Error);
  M_LOG(Error, "Plugin '{}': {}", Plugin.ManifestPath.string(), Plugin.LastError);
}
}  // namespace

PluginHost& PluginHost::GetInstance() {
  static PluginHost Instance;
  return Instance;
}

PluginHost::PluginHost() = default;

PluginHost::~PluginHost() {
  Shutdown();
  // Keep deferred DLL handles pinned until process exit if actors outlive the host.
  for (auto& Plugin : Plugins) Plugin.release();
}

bool PluginHost::Initialize(const std::filesystem::path& InConfigPath) {
  Shutdown();
  if (!Plugins.empty()) {
    M_LOG(Error, "Plugin host cannot reinitialize while plugin actors are alive.");
    return false;
  }

  ConfigPath = InConfigPath;
  NetworkTransportRegistry::GetInstance();
  try {
    std::ifstream File(ConfigPath);
    if (!File) throw std::runtime_error("plugins.json could not be opened");
    nlohmann::json Json;
    File >> Json;
    if (!Json.is_object() || !Json.contains("schema_version") ||
        !Json["schema_version"].is_number_integer() || Json["schema_version"] != 1 ||
        !Json.contains("required_plugins") || !Json["required_plugins"].is_array())
      throw std::runtime_error("Invalid plugins.json schema");
    std::set<std::string> Names;
    for (const auto& Value : Json["required_plugins"]) {
      if (!Value.is_string()) throw std::runtime_error("Plugin name must be a string");
      const auto Name = Value.get<std::string>();
      if (Name.empty() ||
          Name.find_first_not_of(
              "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-"
          ) != std::string::npos ||
          !Names.insert(Name).second)
        throw std::runtime_error("Invalid or duplicate plugin name");
    }
    for (const auto& Name : Names) {
      auto Plugin = std::make_unique<FLoadedPlugin>();
      Plugin->Name = Name;
      Plugin->ManifestPath = ConfigPath.parent_path() / (Name + ".dll");
      Plugin->ModuleOwner = "Plugin:" + Name;
      const bool Activated = ActivatePlugin(*Plugin);
      Plugins.push_back(std::move(Plugin));
      if (!Activated) throw std::runtime_error("Required plugin failed: " + Name);
    }
    Initialized = true;
    return true;
  } catch (const std::exception& Exception) {
    M_LOG(Error, "Plugin initialization failed: {}", Exception.what());
    NetworkManager::GetInstance().Stop();
    Shutdown();
    return false;
  }
}

void PluginHost::Update(float DeltaTime) {
  if (!Initialized) return;

  for (const std::unique_ptr<FLoadedPlugin>& Plugin : Plugins) {
    if (Plugin->State != EPluginState::Active || Plugin->Instance == nullptr) continue;

    try {
      Plugin->Instance->OnUpdate(DeltaTime);
    } catch (const std::exception& Exception) {
      SetPluginError(*Plugin, std::string("OnUpdate threw an exception: ") + Exception.what());
    } catch (...) {
      SetPluginError(*Plugin, "OnUpdate threw an unknown exception.");
    }
  }
}

void PluginHost::Shutdown() {
  for (std::size_t Index = Plugins.size(); Index > 0; --Index) {
    if (DeactivatePlugin(*Plugins[Index - 1])) Plugins.erase(Plugins.begin() + Index - 1);
  }
  if (Plugins.empty()) {
    Initialized = false;
    ConfigPath.clear();
  }
}

bool PluginHost::IsPluginActive(std::string_view Name) const {
  for (const auto& Plugin : Plugins)
    if (Plugin->Name == Name && Plugin->State == EPluginState::Active) return true;
  return false;
}

bool PluginHost::IsInitialized() const { return Initialized; }

size_t PluginHost::GetPluginCount() const { return Plugins.size(); }

size_t PluginHost::GetActivePluginCount() const {
  size_t ActivePluginCount = 0;
  for (const std::unique_ptr<FLoadedPlugin>& Plugin : Plugins) {
    if (Plugin->State == EPluginState::Active) ++ActivePluginCount;
  }
  return ActivePluginCount;
}

bool PluginHost::ActivatePlugin(FLoadedPlugin& Plugin) {
  const std::filesystem::path LibraryPath = Plugin.ManifestPath;
  std::error_code ErrorCode;
  if (!std::filesystem::is_regular_file(LibraryPath, ErrorCode)) {
    SetPluginError(Plugin, "Plugin library was not found: " + LibraryPath.string());
    return false;
  }

  if (!Plugin.Library.Load(LibraryPath)) {
    SetPluginError(Plugin, "Failed to load library: " + Plugin.Library.GetLastError());
    return false;
  }
  Plugin.State = EPluginState::Loaded;

  const auto CreateFunction = reinterpret_cast<FCreateBroccoliPluginFunction>(
      Plugin.Library.GetSymbol("CreateBroccoliPlugin")
  );
  if (CreateFunction == nullptr) {
    SetPluginError(Plugin, "CreateBroccoliPlugin was not found: " + Plugin.Library.GetLastError());
    Plugin.Library.Unload();
    return false;
  }

  Plugin.DestroyFunction = reinterpret_cast<FDestroyBroccoliPluginFunction>(
      Plugin.Library.GetSymbol("DestroyBroccoliPlugin")
  );
  if (Plugin.DestroyFunction == nullptr) {
    SetPluginError(Plugin, "DestroyBroccoliPlugin was not found: " + Plugin.Library.GetLastError());
    Plugin.Library.Unload();
    return false;
  }

  try {
    Plugin.Instance = CreateFunction();
  } catch (const std::exception& Exception) {
    SetPluginError(
        Plugin, std::string("CreateBroccoliPlugin threw an exception: ") + Exception.what()
    );
    Plugin.Library.Unload();
    return false;
  } catch (...) {
    SetPluginError(Plugin, "CreateBroccoliPlugin threw an unknown exception.");
    Plugin.Library.Unload();
    return false;
  }
  if (Plugin.Instance == nullptr) {
    SetPluginError(Plugin, "CreateBroccoliPlugin returned nullptr.");
    Plugin.Library.Unload();
    return false;
  }

  uint32_t PluginApiVersion = 0;
  try {
    PluginApiVersion = Plugin.Instance->GetApiVersion();
    const char* InstanceName = Plugin.Instance->GetName();
    if (!InstanceName || Plugin.Name != InstanceName)
      throw std::runtime_error("Plugin name mismatch");
  } catch (const std::exception& Exception) {
    SetPluginError(Plugin, std::string("Plugin identity validation failed: ") + Exception.what());
    DeactivatePlugin(Plugin);
    return false;
  } catch (...) {
    SetPluginError(Plugin, "Plugin identity validation threw an unknown exception.");
    DeactivatePlugin(Plugin);
    return false;
  }
  if (PluginApiVersion != BROCCOLI_PLUGIN_API_VERSION) {
    SetPluginError(Plugin, "Plugin instance API version is not supported.");
    DeactivatePlugin(Plugin);
    return false;
  }

  PluginContext Context(Plugin.ModuleOwner);
  Plugin.UnloadCallbackPending = true;
  try {
    if (!Plugin.Instance->OnLoad(Context)) {
      SetPluginError(Plugin, "OnLoad returned false.");
      DeactivatePlugin(Plugin);
      return false;
    }
  } catch (const std::exception& Exception) {
    SetPluginError(Plugin, std::string("OnLoad threw an exception: ") + Exception.what());
    DeactivatePlugin(Plugin);
    return false;
  } catch (...) {
    SetPluginError(Plugin, "OnLoad threw an unknown exception.");
    DeactivatePlugin(Plugin);
    return false;
  }

  Plugin.UnloadCallbackPending = true;
  Plugin.State = EPluginState::Active;
  M_LOG(Log, "Plugin '{}' activated.", Plugin.Name);
  return true;
}

bool PluginHost::DeactivatePlugin(FLoadedPlugin& Plugin) {
  if (ActorRegistry::GetInstance().HasLiveActors(Plugin.ModuleOwner) ||
      ComponentRegistry::GetInstance().HasLiveComponents(Plugin.ModuleOwner)) {
    M_LOG(
        Warning, "Plugin '{}' unload delayed: actors or components are still alive.", Plugin.Name
    );
    return false;
  }
  if (Plugin.UnloadCallbackPending && Plugin.Instance != nullptr) {
    try {
      Plugin.Instance->OnUnload();
    } catch (const std::exception& Exception) {
      M_LOG(Error, "Plugin '{}' OnUnload threw an exception: {}", Plugin.Name, Exception.what());
    } catch (...) {
      M_LOG(Error, "Plugin '{}' OnUnload threw an unknown exception.", Plugin.Name);
    }
    Plugin.UnloadCallbackPending = false;
    Plugin.State = EPluginState::Loaded;
    if (ActorRegistry::GetInstance().HasLiveActors(Plugin.ModuleOwner) ||
        ComponentRegistry::GetInstance().HasLiveComponents(Plugin.ModuleOwner)) {
      M_LOG(
          Warning,
          "Plugin '{}' unload delayed: OnUnload left actors or components alive.",
          Plugin.Name
      );
      return false;
    }
  }

  BroccoliAutomationDetail::UnregisterControlModule(Plugin.ModuleOwner);

  if (Plugin.Instance != nullptr && Plugin.DestroyFunction != nullptr) {
    try {
      Plugin.DestroyFunction(Plugin.Instance);
    } catch (const std::exception& Exception) {
      M_LOG(
          Error,
          "Plugin '{}' DestroyBroccoliPlugin threw an exception: {}",
          Plugin.Name,
          Exception.what()
      );
    } catch (...) {
      M_LOG(Error, "Plugin '{}' DestroyBroccoliPlugin threw an unknown exception.", Plugin.Name);
    }
    Plugin.Instance = nullptr;
  }

  if (!FReflectionRegistry::GetInstance().UnregisterModule(Plugin.ModuleOwner)) return false;
  ActorRegistry::GetInstance().UnregisterModule(Plugin.ModuleOwner);
  ComponentRegistry::GetInstance().UnregisterModule(Plugin.ModuleOwner);
  NetworkTransportRegistry::GetInstance().UnregisterModule(Plugin.ModuleOwner);
  Plugin.DestroyFunction = nullptr;
  Plugin.Library.Unload();
  if (Plugin.State != EPluginState::Failed) Plugin.State = EPluginState::Unloaded;
  return true;
}
