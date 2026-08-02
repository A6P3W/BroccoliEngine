#include "AutomationAutoRegistrar.h"

#include <algorithm>

#include "AutomationComponentMethodRegistry.h"
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

void FAutomationAutoRegistrar::AddComponentRegistrationFunction(
    FAutomationComponentRegistrationFunction Function
) {
  if (!Function) {
    return;
  }
  if (std::find(
          ComponentRegistrationFunctions.begin(), ComponentRegistrationFunctions.end(), Function
      ) == ComponentRegistrationFunctions.end()) {
    ComponentRegistrationFunctions.push_back(Function);
  }
}

void FAutomationAutoRegistrar::RegisterAllComponents(
    FAutomationComponentMethodRegistry& Registry
) const {
  for (const FAutomationComponentRegistrationFunction Function : ComponentRegistrationFunctions) {
    Function(Registry);
  }
}

FAutomationComponentMethodAutoRegister::FAutomationComponentMethodAutoRegister(
    FAutomationComponentRegistrationFunction Function
) {
  FAutomationAutoRegistrar::GetInstance().AddComponentRegistrationFunction(Function);
}

void FAutomationAutoRegistrar::AddUnifiedRegistrationFunction(
    FAutomationUnifiedRegistrationFunction Function
) {
  if (!Function) {
    return;
  }
  if (std::find(
          UnifiedRegistrationFunctions.begin(), UnifiedRegistrationFunctions.end(), Function
      ) == UnifiedRegistrationFunctions.end()) {
    UnifiedRegistrationFunctions.push_back(Function);
  }
}

void FAutomationAutoRegistrar::RegisterAllUnified(
    FAutomationMethodRegistry& MethodRegistry,
    FAutomationComponentMethodRegistry& ComponentMethodRegistry
) const {
  for (const FAutomationUnifiedRegistrationFunction Function : UnifiedRegistrationFunctions) {
    Function(MethodRegistry, ComponentMethodRegistry);
  }
}

FAutomationUnifiedMethodAutoRegister::FAutomationUnifiedMethodAutoRegister(
    FAutomationUnifiedRegistrationFunction Function
) {
  FAutomationAutoRegistrar::GetInstance().AddUnifiedRegistrationFunction(Function);
}