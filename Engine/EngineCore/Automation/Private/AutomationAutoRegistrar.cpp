#include "AutomationAutoRegistrar.h"

#include <algorithm>

#include "AutomationMethodRegistry.h"

FAutomationAutoRegistrar& FAutomationAutoRegistrar::GetInstance() {
  static FAutomationAutoRegistrar Instance;
  return Instance;
}

void FAutomationAutoRegistrar::AddRegistrationFunction(FAutomationRegistrationFunction Function) {
  if (!Function) {
    return;
  }
  if (std::find(RegistrationFunctions.begin(), RegistrationFunctions.end(), Function) ==
      RegistrationFunctions.end()) {
    RegistrationFunctions.push_back(Function);
  }
}

void FAutomationAutoRegistrar::RegisterAll(FAutomationMethodRegistry& Registry) const {
  for (const FAutomationRegistrationFunction Function : RegistrationFunctions) {
    Function(Registry);
  }
}

FAutomationMethodAutoRegister::FAutomationMethodAutoRegister(
    FAutomationRegistrationFunction Function
) {
  FAutomationAutoRegistrar::GetInstance().AddRegistrationFunction(Function);
}
