#pragma once

#include <vector>

#include "Detail/AutomationRegistrationContext.h"

class FAutomationComponentMethodRegistry;
class FAutomationActorMethodRegistry;

class FAutomationRegistrationStore {
 public:
  static FAutomationRegistrationStore& Get();

  void AddCallback(BroccoliAutomationDetail::FAutomationRegistrationCallback Callback);
  void RegisterAll(
      FAutomationActorMethodRegistry& MethodRegistry,
      FAutomationComponentMethodRegistry& ComponentMethodRegistry
  ) const;

 private:
  std::vector<BroccoliAutomationDetail::FAutomationRegistrationCallback> Callbacks;
};
