#include "Registration/RegistrationStore.h"

#include <algorithm>

#include "Registry/ActorMethodRegistry.h"
#include "Registry/ComponentMethodRegistry.h"

FAutomationRegistrationStore& FAutomationRegistrationStore::Get() {
  static FAutomationRegistrationStore Store;
  return Store;
}

void FAutomationRegistrationStore::AddCallback(
    BroccoliAutomationDetail::FAutomationRegistrationCallback Callback, std::string ModuleOwner
) {
  if (!Callback || ModuleOwner.empty()) return;
  const bool Exists =
      std::any_of(Callbacks.begin(), Callbacks.end(), [Callback](const FEntry& Entry) {
        return Entry.Callback == Callback;
      });
  if (Exists) return;
  if (ActorRegistry && ComponentRegistry) {
    BroccoliAutomationDetail::FAutomationRegistrationContext Context(
        ActorRegistry, ComponentRegistry, ModuleOwner
    );
    try {
      Callback(Context);
    } catch (...) {
      UnregisterModule(ModuleOwner);
      throw;
    }
  }
  Callbacks.push_back({Callback, std::move(ModuleOwner)});
}

void FAutomationRegistrationStore::RegisterAll(
    FAutomationActorMethodRegistry& MethodRegistry,
    FAutomationComponentMethodRegistry& InComponentMethodRegistry
) {
  ActorRegistry = &MethodRegistry;
  ComponentRegistry = &InComponentMethodRegistry;
  for (const FEntry& Entry : Callbacks) {
    BroccoliAutomationDetail::FAutomationRegistrationContext Context(
        ActorRegistry, ComponentRegistry, Entry.ModuleOwner
    );
    Entry.Callback(Context);
  }
}

void FAutomationRegistrationStore::Detach() {
  ActorRegistry = nullptr;
  ComponentRegistry = nullptr;
}

void FAutomationRegistrationStore::UnregisterModule(std::string_view ModuleOwner) {
  if (ActorRegistry) ActorRegistry->UnregisterModule(ModuleOwner);
  if (ComponentRegistry) ComponentRegistry->UnregisterModule(ModuleOwner);
  std::erase_if(Callbacks, [ModuleOwner](const FEntry& Entry) {
    return Entry.ModuleOwner == ModuleOwner;
  });
}
