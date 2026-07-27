#pragma once

#include <vector>

#include "BroccoliEngineAPI.h"

class FAutomationMethodRegistry;
class FAutomationComponentMethodRegistry;

using FAutomationRegistrationFunction = void (*)(FAutomationMethodRegistry&);
using FAutomationComponentRegistrationFunction = void (*)(FAutomationComponentMethodRegistry&);

class BROCCOLI_ENGINE_API FAutomationAutoRegistrar {
 public:
  static FAutomationAutoRegistrar& GetInstance();

  void AddRegistrationFunction(FAutomationRegistrationFunction Function);
  void RegisterAll(FAutomationMethodRegistry& Registry) const;
  void AddComponentRegistrationFunction(FAutomationComponentRegistrationFunction Function);
  void RegisterAllComponents(FAutomationComponentMethodRegistry& Registry) const;

 private:
  std::vector<FAutomationRegistrationFunction> RegistrationFunctions;
  std::vector<FAutomationComponentRegistrationFunction> ComponentRegistrationFunctions;
};

class BROCCOLI_ENGINE_API FAutomationMethodAutoRegister {
 public:
  explicit FAutomationMethodAutoRegister(FAutomationRegistrationFunction Function);
};

class BROCCOLI_ENGINE_API FAutomationComponentMethodAutoRegister {
 public:
  explicit FAutomationComponentMethodAutoRegister(FAutomationComponentRegistrationFunction Function);
};

#define BROCCOLI_JOIN_IMPL(A, B) A##B
#define BROCCOLI_JOIN(A, B) BROCCOLI_JOIN_IMPL(A, B)

#define REGISTER_AUTOMATION_METHODS(ClassName)                                                \
  namespace {                                                                                 \
  const FAutomationMethodAutoRegister BROCCOLI_JOIN(GAutomationMethodRegister_, __COUNTER__)( \
      &ClassName::RegisterAutomationMethods                                                   \
  );                                                                                          \
  }

#define REGISTER_AUTOMATION_COMPONENT_METHODS(ClassName)                                      \
  namespace {                                                                                 \
  const FAutomationComponentMethodAutoRegister                                                \
      BROCCOLI_JOIN(GAutomationComponentMethodRegister_, __COUNTER__)(                       \
          &ClassName::RegisterAutomationMethods                                               \
      );                                                                                      \
  }
