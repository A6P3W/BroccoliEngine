#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "Detail/AutomationRegistrationContext.h"

class FAutomationComponentMethodRegistry;
class FAutomationActorMethodRegistry;

class FAutomationRegistrationStore {
 public:
  static FAutomationRegistrationStore& Get();

  void AddCallback(
      BroccoliAutomationDetail::FAutomationRegistrationCallback Callback,
      std::string ModuleOwner = "Static"
  );
  void RegisterAll(
      FAutomationActorMethodRegistry& MethodRegistry,
      FAutomationComponentMethodRegistry& ComponentMethodRegistry
  );
  void Detach();
  void UnregisterModule(std::string_view ModuleOwner);

 private:
  struct FEntry {
    BroccoliAutomationDetail::FAutomationRegistrationCallback Callback = nullptr;
    std::string ModuleOwner;
  };
  std::vector<FEntry> Callbacks;
  FAutomationActorMethodRegistry* ActorRegistry = nullptr;
  FAutomationComponentMethodRegistry* ComponentRegistry = nullptr;
};
