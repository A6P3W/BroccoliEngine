#pragma once

#include <type_traits>
#include <utility>

#include "BroccoliEngineAPI.h"
#include "ComponentRegistry.h"
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

  template <class T, class Base = [:ReflectionGenerator::DirectBase<T>():]>
  bool RegisterComponent(FComponentClassOptions Options = {}) const {
    using DirectBaseType = [:ReflectionGenerator::DirectBase<T>():];
    static_assert(std::is_same_v<Base, DirectBaseType>, "Base must be the direct base class of T");
    static_assert(std::is_base_of_v<MActorComponent, Base>);
    if (ModuleOwner.empty()) return false;
    if (!ComponentRegistry::GetInstance().RegisterOwned<T>(ModuleOwner, Options)) return false;
    if constexpr (!std::is_same_v<Base, MActorComponent>) {
      if (!ReflectionGenerator::RegisterStaticComponentClass<Base>()) {
        ComponentRegistry::GetInstance().UnregisterClass(T::StaticComponentClassName());
        return false;
      }
    }
    if (ReflectionGenerator::RegisterTypeClass<T, Base>(ModuleOwner) != 0) return true;
    ComponentRegistry::GetInstance().UnregisterClass(T::StaticComponentClassName());
    return false;
  }

  void LogInfo(const char* Message) const;
  void LogWarning(const char* Message) const;
  void LogError(const char* Message) const;

 private:
  std::string ModuleOwner;
};
