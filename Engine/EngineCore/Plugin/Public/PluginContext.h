#pragma once

#include "BroccoliEngineAPI.h"
#include "ReflectionGenerator.h"

class BROCCOLI_ENGINE_API PluginContext {
 public:
  explicit PluginContext(std::string ModuleOwner = {}) : ModuleOwner(std::move(ModuleOwner)) {}

  template <class T, class Base = AActor>
  bool RegisterActor(bool IsGameMode = false) const {
    if (ModuleOwner.empty()) return false;
    if (!ActorRegistry::GetInstance().RegisterOwned<T>(ModuleOwner, IsGameMode)) return false;
    if (ReflectionGenerator::RegisterClass<T, Base>(ModuleOwner) != 0) return true;
    ActorRegistry::GetInstance().UnregisterClass(T::StaticClassName());
    return false;
  }

  void LogInfo(const char* Message) const;
  void LogWarning(const char* Message) const;
  void LogError(const char* Message) const;

 private:
  std::string ModuleOwner;
};
