#include "ActorRegistry.h"

#include "Log.h"

struct ActorRegistry::Impl {
  std::unordered_map<std::string, FactoryFn> Factories;
  std::unordered_map<std::string, std::string> Owners;
  std::unordered_map<const AActor*, std::string> LiveActors;
  std::vector<std::string> ClassNames;
  std::vector<std::string> GameModeClassNames;
};

ActorRegistry::ActorRegistry() : ImplPtr(new Impl()) {}

ActorRegistry::~ActorRegistry() { delete ImplPtr; }

ActorRegistry& ActorRegistry::GetInstance() {
  static ActorRegistry Instance;
  return Instance;
}

bool ActorRegistry::RegisterFactory(
    std::string ClassName, FactoryFn Factory, std::string ModuleOwner, bool IsGameMode
) {
  if (ImplPtr->Factories.contains(ClassName)) {
    M_LOG(Error, "Actor class '{}' is already registered.", ClassName);
    return false;
  }
  ImplPtr->Owners.emplace(ClassName, std::move(ModuleOwner));
  ImplPtr->Factories.emplace(ClassName, std::move(Factory));
  std::vector<std::string>& Names = IsGameMode ? ImplPtr->GameModeClassNames : ImplPtr->ClassNames;
  if (std::find(Names.begin(), Names.end(), ClassName) == Names.end()) {
    Names.push_back(std::move(ClassName));
  }
  return true;
}

AActor* ActorRegistry::Spawn(
    World* WorldPtr, const std::string& ClassName, const FVector2D& Location, FRotator Rotation
) {
  const auto Iterator = ImplPtr->Factories.find(ClassName);
  if (Iterator == ImplPtr->Factories.end()) return nullptr;
  AActor* Actor = Iterator->second(WorldPtr, Location, Rotation);
  if (Actor != nullptr) ImplPtr->LiveActors.emplace(Actor, ImplPtr->Owners.at(ClassName));
  return Actor;
}

const std::vector<std::string>& ActorRegistry::GetClassNames() const { return ImplPtr->ClassNames; }

const std::vector<std::string>& ActorRegistry::GetGameModeClassNames() const {
  return ImplPtr->GameModeClassNames;
}

bool ActorRegistry::Contains(const std::string& ClassName) const {
  return ImplPtr->Factories.contains(ClassName);
}

bool ActorRegistry::HasLiveActors(std::string_view ModuleOwner) const {
  return std::ranges::any_of(ImplPtr->LiveActors, [ModuleOwner](const auto& Entry) {
    return Entry.second == ModuleOwner;
  });
}

void ActorRegistry::NotifySpawned(const AActor* Actor, std::string_view ClassName) {
  const auto Iterator = ImplPtr->Owners.find(std::string(ClassName));
  if (Iterator != ImplPtr->Owners.end()) ImplPtr->LiveActors.emplace(Actor, Iterator->second);
}

void ActorRegistry::NotifyDestroyed(const AActor* Actor) { ImplPtr->LiveActors.erase(Actor); }

void ActorRegistry::UnregisterModule(std::string_view ModuleOwner) {
  for (auto Iterator = ImplPtr->Owners.begin(); Iterator != ImplPtr->Owners.end();) {
    if (Iterator->second != ModuleOwner) {
      ++Iterator;
      continue;
    }
    const std::string Name = Iterator->first;
    ImplPtr->Factories.erase(Name);
    std::erase(ImplPtr->ClassNames, Name);
    std::erase(ImplPtr->GameModeClassNames, Name);
    Iterator = ImplPtr->Owners.erase(Iterator);
  }
}

void ActorRegistry::UnregisterClass(std::string_view ClassName) {
  const std::string Name(ClassName);
  ImplPtr->Factories.erase(Name);
  ImplPtr->Owners.erase(Name);
  std::erase(ImplPtr->ClassNames, Name);
  std::erase(ImplPtr->GameModeClassNames, Name);
}
