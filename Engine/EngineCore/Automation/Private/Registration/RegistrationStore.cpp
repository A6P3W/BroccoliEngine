#include "Registration/RegistrationStore.h"

#include <algorithm>

#include "Registry/ActorMethodRegistry.h"
#include "Registry/ComponentMethodRegistry.h"

FAutomationRegistrationStore& FAutomationRegistrationStore::Get() {
  static FAutomationRegistrationStore Store;
  return Store;
}

void FAutomationRegistrationStore::AddCallback(
    BroccoliAutomationDetail::FAutomationRegistrationCallback Callback
) {
  if (Callback && std::find(Callbacks.begin(), Callbacks.end(), Callback) == Callbacks.end()) {
    Callbacks.push_back(Callback);
  }
}

void FAutomationRegistrationStore::RegisterAll(
    FAutomationActorMethodRegistry& MethodRegistry,
    FAutomationComponentMethodRegistry& ComponentMethodRegistry
) const {
  BroccoliAutomationDetail::FAutomationRegistrationContext Context(
      &MethodRegistry, &ComponentMethodRegistry
  );
  for (const BroccoliAutomationDetail::FAutomationRegistrationCallback Callback : Callbacks) {
    Callback(Context);
  }
}
