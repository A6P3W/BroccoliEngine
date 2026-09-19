#include "ComponentMethodRegistry.h"

#include <utility>

#include "RegistryCommon.h"

namespace {
constexpr AutomationRegistryDetail::FAutomationRegistryValidationMessages ComponentMethodMessages{
    "component method",
    "The component method registry is frozen.",
    "ClassName must contain between 1 and 128 UTF-8 bytes.",
    "MethodName must match ^[a-z][a-z0-9_]{0,127}$.",
    "Description must not be empty.",
    "Handler must not be empty.",
    "The method is already registered for this component class."
};
}  // namespace

bool FAutomationComponentMethodRegistry::RegisterMethod(
    std::string ClassName, FAutomationComponentMethodDescriptor Descriptor, std::string* OutError
) {
  return AutomationRegistryDetail::RegisterClassMethod(
      Frozen,
      MethodsByClass,
      std::move(ClassName),
      std::move(Descriptor),
      ComponentMethodMessages,
      OutError
  );
}

const FAutomationComponentMethodDescriptor* FAutomationComponentMethodRegistry::FindMethod(
    std::string_view ClassName, std::string_view MethodName
) const {
  return AutomationRegistryDetail::FindClassMethod(MethodsByClass, ClassName, MethodName);
}

std::vector<FAutomationComponentMethodSnapshot>
FAutomationComponentMethodRegistry::GetMethodsForClass(std::string_view ClassName) const {
  return AutomationRegistryDetail::GetClassMethods<FAutomationComponentMethodSnapshot>(
      MethodsByClass, ClassName
  );
}

void FAutomationComponentMethodRegistry::Freeze() { Frozen = true; }

bool FAutomationComponentMethodRegistry::IsFrozen() const { return Frozen; }
