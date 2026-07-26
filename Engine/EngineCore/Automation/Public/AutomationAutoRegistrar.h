#pragma once

#include <vector>

#include "BroccoliEngineAPI.h"

class FAutomationMethodRegistry;

using FAutomationRegistrationFunction = void (*)(FAutomationMethodRegistry&);

class BROCCOLI_ENGINE_API FAutomationAutoRegistrar {
 public:
  static FAutomationAutoRegistrar& GetInstance();

  void AddRegistrationFunction(FAutomationRegistrationFunction Function);
  void RegisterAll(FAutomationMethodRegistry& Registry) const;

 private:
  std::vector<FAutomationRegistrationFunction> RegistrationFunctions;
};

class BROCCOLI_ENGINE_API FAutomationMethodAutoRegister {
 public:
  explicit FAutomationMethodAutoRegister(FAutomationRegistrationFunction Function);
};

#define BROCCOLI_JOIN_IMPL(A, B) A##B
#define BROCCOLI_JOIN(A, B) BROCCOLI_JOIN_IMPL(A, B)

#define REGISTER_AUTOMATION_METHODS(ClassName)                                                \
  namespace {                                                                                 \
  const FAutomationMethodAutoRegister BROCCOLI_JOIN(GAutomationMethodRegister_, __COUNTER__)( \
      &ClassName::RegisterAutomationMethods                                                   \
  );                                                                                          \
  }
