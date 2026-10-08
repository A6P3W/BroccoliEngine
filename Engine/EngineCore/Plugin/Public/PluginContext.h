#pragma once

#include <type_traits>
#include <utility>

#include "BroccoliEngineAPI.h"
#include "ControlMacros.h"
#include "ReflectionGenerator.h"

class BROCCOLI_ENGINE_API PluginContext {
 public:
  explicit PluginContext(std::string ModuleOwner = {}) : ModuleOwner(std::move(ModuleOwner)) {}

  template <class T, class Base = [:ReflectionGenerator::DirectBase<T>():]>
  bool RegisterActor(bool IsGameMode = false) const {
    using DirectBaseType = [:ReflectionGenerator::DirectBase<T>():];
    static_assert(std::is_same_v<Base, DirectBaseType>, "Base must be the direct base class of T");
    if (ModuleOwner.empty()) return false;
    if (!ActorRegistry::GetInstance().RegisterOwned<T>(ModuleOwner, IsGameMode)) return false;
    if (ReflectionGenerator::RegisterClass<T, Base>(ModuleOwner) != 0) return true;
    ActorRegistry::GetInstance().UnregisterClass(T::StaticClassName());
    return false;
  }

  template <class T>
  bool RegisterControlClass() const {
    static_assert(
        std::derived_from<T, AActor> || std::derived_from<T, MActorComponent>,
        "Control classes must derive from AActor or MActorComponent."
    );
    if (ModuleOwner.empty()) return false;
    BroccoliAutomationDetail::RegisterCallbackOwned(
        &BroccoliAutomationDetail::RegisterClass<T>, ModuleOwner
    );
    return true;
  }

  void LogInfo(const char* Message) const;
  void LogWarning(const char* Message) const;
  void LogError(const char* Message) const;

 private:
  std::string ModuleOwner;
};
