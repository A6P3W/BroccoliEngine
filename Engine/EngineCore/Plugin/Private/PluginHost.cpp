#include "PluginHost.h"

#include <exception>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>

#include "IPlugin.h"
#include "Log.h"
#include "NativeLibrary.h"
#include "PluginAPI.h"
#include "PluginContext.h"
#include "PluginManifest.h"

struct FLoadedPlugin {
  std::filesystem::path ManifestPath;
  PluginManifest Manifest;
  NativeLibrary Library;
  IPlugin* Instance = nullptr;
  FDestroyBroccoliPluginFunction DestroyFunction = nullptr;
  EPluginState State = EPluginState::Discovered;
  std::string LastError;
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

PluginHost::~PluginHost() { Shutdown(); }

bool PluginHost::Initialize(const std::filesystem::path& InPluginsDirectory) {
  Shutdown();

  PluginsDirectory = InPluginsDirectory;
  Initialized = true;
  return DiscoverPlugins();
}

void PluginHost::Update(float DeltaTime) {
  if (!Initialized) return;

  for (const std::unique_ptr<FLoadedPlugin>& Plugin : Plugins) {
    if (Plugin->State != EPluginState::Active || Plugin->Instance == nullptr) continue;

    try {
      Plugin->Instance->OnUpdate(DeltaTime);
    } catch (const std::exception& Exception) {
      DeactivatePlugin(*Plugin);
      SetPluginError(*Plugin, std::string("OnUpdate threw an exception: ") + Exception.what());
    } catch (...) {
      DeactivatePlugin(*Plugin);
      SetPluginError(*Plugin, "OnUpdate threw an unknown exception.");
    }
  }
}

void PluginHost::Shutdown() {
  for (auto Iterator = Plugins.rbegin(); Iterator != Plugins.rend(); ++Iterator) {
    DeactivatePlugin(**Iterator);
  }
  Plugins.clear();
  Initialized = false;
  PluginsDirectory.clear();
}

bool PluginHost::LoadPlugin(const std::filesystem::path& ManifestPath) {
  auto Plugin = std::make_unique<FLoadedPlugin>();
  Plugin->ManifestPath = ManifestPath;

  if (!ParsePluginManifest(ManifestPath, Plugin->Manifest, Plugin->LastError)) {
    SetPluginError(*Plugin, Plugin->LastError);
    Plugins.push_back(std::move(Plugin));
    return false;
  }

  for (const std::unique_ptr<FLoadedPlugin>& ExistingPlugin : Plugins) {
    if (ExistingPlugin->Manifest.Name == Plugin->Manifest.Name) {
      SetPluginError(*Plugin, "A plugin with the same name was already discovered.");
      Plugins.push_back(std::move(Plugin));
      return false;
    }
  }

  if (!Plugin->Manifest.Enabled) {
    M_LOG(Log, "Plugin '{}' is disabled.", Plugin->Manifest.Name);
    Plugins.push_back(std::move(Plugin));
    return true;
  }

  const bool Activated = ActivatePlugin(*Plugin);
  Plugins.push_back(std::move(Plugin));
  return Activated;
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

bool PluginHost::DiscoverPlugins() {
  std::error_code ErrorCode;
  if (!std::filesystem::is_directory(PluginsDirectory, ErrorCode)) {
    M_LOG(Log, "Plugin directory was not found: {}", PluginsDirectory.string());
    return !ErrorCode;
  }

  bool DiscoverySucceeded = true;
  std::filesystem::directory_iterator Iterator(PluginsDirectory, ErrorCode);
  const std::filesystem::directory_iterator End;
  while (!ErrorCode && Iterator != End) {
    if (Iterator->is_directory(ErrorCode)) {
      const std::filesystem::path ManifestPath = Iterator->path() / "plugin.json";
      if (std::filesystem::is_regular_file(ManifestPath, ErrorCode)) {
        LoadPlugin(ManifestPath);
      }
    }
    if (ErrorCode) {
      DiscoverySucceeded = false;
      break;
    }
    Iterator.increment(ErrorCode);
  }

  if (ErrorCode) {
    M_LOG(
        Error,
        "Failed to discover plugins in '{}': {}",
        PluginsDirectory.string(),
        ErrorCode.message()
    );
    return false;
  }
  return DiscoverySucceeded;
}

bool PluginHost::ActivatePlugin(FLoadedPlugin& Plugin) {
  if (Plugin.Manifest.ApiVersion != BROCCOLI_PLUGIN_API_VERSION) {
    SetPluginError(
        Plugin,
        "Manifest API version " + std::to_string(Plugin.Manifest.ApiVersion) + " is not supported."
    );
    return false;
  }

  const std::filesystem::path LibraryPath =
      Plugin.ManifestPath.parent_path() / Plugin.Manifest.LibraryPath;
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
  } catch (const std::exception& Exception) {
    SetPluginError(Plugin, std::string("GetApiVersion threw an exception: ") + Exception.what());
    DeactivatePlugin(Plugin);
    return false;
  } catch (...) {
    SetPluginError(Plugin, "GetApiVersion threw an unknown exception.");
    DeactivatePlugin(Plugin);
    return false;
  }
  if (PluginApiVersion != BROCCOLI_PLUGIN_API_VERSION) {
    SetPluginError(Plugin, "Plugin instance API version is not supported.");
    DeactivatePlugin(Plugin);
    return false;
  }

  PluginContext Context;
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

  Plugin.State = EPluginState::Active;
  M_LOG(Log, "Plugin '{}' activated.", Plugin.Manifest.Name);
  return true;
}

void PluginHost::DeactivatePlugin(FLoadedPlugin& Plugin) {
  if (Plugin.State == EPluginState::Active && Plugin.Instance != nullptr) {
    try {
      Plugin.Instance->OnUnload();
    } catch (const std::exception& Exception) {
      M_LOG(
          Error,
          "Plugin '{}' OnUnload threw an exception: {}",
          Plugin.Manifest.Name,
          Exception.what()
      );
    } catch (...) {
      M_LOG(Error, "Plugin '{}' OnUnload threw an unknown exception.", Plugin.Manifest.Name);
    }
  }

  if (Plugin.Instance != nullptr && Plugin.DestroyFunction != nullptr) {
    try {
      Plugin.DestroyFunction(Plugin.Instance);
    } catch (const std::exception& Exception) {
      M_LOG(
          Error,
          "Plugin '{}' DestroyBroccoliPlugin threw an exception: {}",
          Plugin.Manifest.Name,
          Exception.what()
      );
    } catch (...) {
      M_LOG(
          Error,
          "Plugin '{}' DestroyBroccoliPlugin threw an unknown exception.",
          Plugin.Manifest.Name
      );
    }
    Plugin.Instance = nullptr;
  }

  Plugin.DestroyFunction = nullptr;
  Plugin.Library.Unload();
  if (Plugin.State != EPluginState::Failed) Plugin.State = EPluginState::Unloaded;
}
