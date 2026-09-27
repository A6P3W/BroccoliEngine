#pragma once
#include <algorithm>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "ActorManager.h"
#include "BroccoliEngineAPI.h"
#include "UMath.h"
#include "World.h"
class AActor;
class BROCCOLI_ENGINE_API ActorRegistry {
 public:
  using FactoryFn = std::function<AActor*(World*, const FVector2D&, FRotator)>;

  static ActorRegistry& GetInstance();

  template <class T>
  void Register(bool IsGameMode = false) {
    RegisterOwned<T>("Static", IsGameMode);
  }

  template <class T>
  bool RegisterOwned(std::string ModuleOwner, bool IsGameMode = false) {
    return RegisterFactory(
        T::StaticClassName(),
        [](World* WorldPtr, const FVector2D& Location, FRotator Rotation) -> AActor* {
          return WorldPtr->SpawnActor<T>(Location, Rotation, true);
        },
        std::move(ModuleOwner),
        IsGameMode
    );
  }

  AActor* Spawn(
      World* WorldPtr,
      const std::string& ClassName,
      const FVector2D& Location = FVector2D::ZeroVector(),
      FRotator Rotation = FRotator(0)
  );

  const std::vector<std::string>& GetClassNames() const;
  const std::vector<std::string>& GetGameModeClassNames() const;
  bool Contains(const std::string& ClassName) const;
  bool HasLiveActors(std::string_view ModuleOwner) const;
  void NotifySpawned(const AActor* Actor, std::string_view ClassName);
  void NotifyDestroyed(const AActor* Actor);
  void UnregisterModule(std::string_view ModuleOwner);
  void UnregisterClass(std::string_view ClassName);

 private:
  ActorRegistry();
  ~ActorRegistry();
  bool RegisterFactory(
      std::string ClassName, FactoryFn Factory, std::string ModuleOwner, bool IsGameMode
  );
  struct Impl;
  Impl* ImplPtr = nullptr;
};
