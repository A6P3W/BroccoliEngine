#pragma once
#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ActorManager.h"
#include "UMath.h"
#include "World.h"
class AActor;
class ActorRegistry {
 public:
  using FactoryFn = std::function<AActor*(World*, const FVector2D&, FRotator)>;

  static ActorRegistry& GetInstance() {
    static ActorRegistry instance;
    return instance;
  }

  template <class T>
  void Register(bool bIsGameMode = false) {
    std::string className = T::StaticClassName();
    Factories[className] = [](World* world, const FVector2D& loc, FRotator rot) -> AActor* {
      return world->SpawnActor<T>(loc, rot, true);
    };

    if (bIsGameMode) {
      if (std::find(GameModeClassNames.begin(), GameModeClassNames.end(), className) ==
          GameModeClassNames.end()) {
        GameModeClassNames.push_back(className);
      }
    } else if (std::find(ClassNames.begin(), ClassNames.end(), className) == ClassNames.end()) {
      ClassNames.push_back(className);
    }
  }

  // クラス名からスポーン
  AActor* Spawn(
      World* world,
      const std::string& className,
      const FVector2D& loc = FVector2D::ZeroVector,
      FRotator rot = FRotator(0)
  ) {
    auto it = Factories.find(className);
    if (it == Factories.end()) return nullptr;
    return it->second(world, loc, rot);
  }

  const std::vector<std::string>& GetClassNames() const { return ClassNames; }
  const std::vector<std::string>& GetGameModeClassNames() const { return GameModeClassNames; }

  bool Contains(const std::string& className) const { return Factories.count(className) > 0; }

 private:
  ActorRegistry() = default;
  std::unordered_map<std::string, FactoryFn> Factories;
  std::vector<std::string> ClassNames;
  std::vector<std::string> GameModeClassNames;
};
