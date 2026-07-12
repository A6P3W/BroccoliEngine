#pragma once
#include "BroccoliEngineAPI.h"
#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ActorManager.h"
#include "UMath.h"
#include "World.h"
class AActor;
class BROCCOLI_ENGINE_API ActorRegistry {
 public:
  using FactoryFn = std::function<AActor*(World*, const FVector2D&, FRotator)>;

  static ActorRegistry& GetInstance();

  template <class T>
  void Register(bool bIsGameMode = false) {
    std::string ClassName = T::StaticClassName();
    RegisterFactory(
        ClassName,
        [](World* WorldPtr, const FVector2D& Location, FRotator Rotation) -> AActor* {
          return WorldPtr->SpawnActor<T>(Location, Rotation, true);
        },
        bIsGameMode
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

 private:
  ActorRegistry();
  ~ActorRegistry();
  void RegisterFactory(std::string ClassName, FactoryFn Factory, bool bIsGameMode);
  struct Impl;
  Impl* ImplPtr = nullptr;
};
