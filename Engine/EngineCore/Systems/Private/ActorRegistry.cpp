#include "ActorRegistry.h"

struct ActorRegistry::Impl {
  std::unordered_map<std::string, FactoryFn> Factories;
  std::vector<std::string> ClassNames;
  std::vector<std::string> GameModeClassNames;
};

ActorRegistry::ActorRegistry() : ImplPtr(new Impl()) {}

ActorRegistry::~ActorRegistry() {
  delete ImplPtr;
}

ActorRegistry& ActorRegistry::GetInstance() {
  static ActorRegistry Instance;
  return Instance;
}

void ActorRegistry::RegisterFactory(std::string ClassName, FactoryFn Factory, bool bIsGameMode) {
  ImplPtr->Factories[ClassName] = std::move(Factory);
  std::vector<std::string>& Names = bIsGameMode ? ImplPtr->GameModeClassNames : ImplPtr->ClassNames;
  if (std::find(Names.begin(), Names.end(), ClassName) == Names.end()) {
    Names.push_back(std::move(ClassName));
  }
}

AActor* ActorRegistry::Spawn(
    World* WorldPtr, const std::string& ClassName, const FVector2D& Location, FRotator Rotation
) {
  const auto Iterator = ImplPtr->Factories.find(ClassName);
  return Iterator == ImplPtr->Factories.end() ? nullptr : Iterator->second(WorldPtr, Location, Rotation);
}

const std::vector<std::string>& ActorRegistry::GetClassNames() const {
  return ImplPtr->ClassNames;
}

const std::vector<std::string>& ActorRegistry::GetGameModeClassNames() const {
  return ImplPtr->GameModeClassNames;
}

bool ActorRegistry::Contains(const std::string& ClassName) const {
  return ImplPtr->Factories.contains(ClassName);
}