#include "ComponentRegistry.h"

#include <algorithm>
#include <unordered_map>

#include "Actor.h"
#include "Log.h"

struct ComponentRegistry::Impl {
  struct Entry {
    FactoryFn Factory;
    FComponentClassInfo Info;
  };
  std::unordered_map<std::string, Entry> Entries;
  std::unordered_map<const MActorComponent*, std::string> LiveComponents;
};

ComponentRegistry::ComponentRegistry() : ImplPtr(new Impl()) {}
ComponentRegistry::~ComponentRegistry() { delete ImplPtr; }

ComponentRegistry& ComponentRegistry::GetInstance() {
  static ComponentRegistry Instance;
  return Instance;
}

bool ComponentRegistry::RegisterFactory(
    std::string ClassName,
    FactoryFn Factory,
    std::string ModuleOwner,
    FComponentClassOptions Options
) {
  if (ClassName.empty() || ImplPtr->Entries.contains(ClassName)) return false;
  const std::string Key = ClassName;
  ImplPtr->Entries.emplace(
      Key, Impl::Entry{std::move(Factory), {std::move(ClassName), std::move(ModuleOwner), Options}}
  );
  return true;
}

MActorComponent* ComponentRegistry::Create(
    AActor* Owner, std::string_view ClassName, std::string_view RequestedName
) {
  if (Owner == nullptr) return nullptr;
  auto Iterator = ImplPtr->Entries.find(std::string(ClassName));
  if (Iterator == ImplPtr->Entries.end()) return nullptr;
  if (!Iterator->second.Info.Options.AllowMultiple) {
    for (const auto& Component : Owner->GetComponents())
      if (Component && !Component->IsPendingDestroy() &&
          Component->GetComponentClassName() == ClassName)
        return nullptr;
  }
  std::unique_ptr<MActorComponent> Component = Iterator->second.Factory();
  if (!Component) return nullptr;
  return Owner->AcceptNewObjectComponent(
      std::move(Component),
      FComponentCreateParams{
          .Name = std::string(RequestedName), .Source = EComponentCreationSource::Instance
      }
  );
}

bool ComponentRegistry::Contains(std::string_view ClassName) const {
  return ImplPtr->Entries.contains(std::string(ClassName));
}

std::vector<FComponentClassInfo> ComponentRegistry::GetClasses() const {
  std::vector<FComponentClassInfo> Classes;
  for (const auto& [Name, Entry] : ImplPtr->Entries) Classes.push_back(Entry.Info);
  std::ranges::sort(Classes, {}, &FComponentClassInfo::ClassName);
  return Classes;
}

bool ComponentRegistry::HasLiveComponents(std::string_view ModuleOwner) const {
  return std::ranges::any_of(ImplPtr->LiveComponents, [ModuleOwner](const auto& Entry) {
    return Entry.second == ModuleOwner;
  });
}

void ComponentRegistry::NotifyCreated(
    const MActorComponent* Component, std::string_view ClassName
) {
  auto Iterator = ImplPtr->Entries.find(std::string(ClassName));
  if (Iterator != ImplPtr->Entries.end())
    ImplPtr->LiveComponents.emplace(Component, Iterator->second.Info.ModuleOwner);
}

void ComponentRegistry::NotifyDestroyed(const MActorComponent* Component) {
  ImplPtr->LiveComponents.erase(Component);
}

void ComponentRegistry::UnregisterModule(std::string_view ModuleOwner) {
  std::erase_if(ImplPtr->Entries, [ModuleOwner](const auto& Entry) {
    return Entry.second.Info.ModuleOwner == ModuleOwner;
  });
}

void ComponentRegistry::UnregisterClass(std::string_view ClassName) {
  ImplPtr->Entries.erase(std::string(ClassName));
}
