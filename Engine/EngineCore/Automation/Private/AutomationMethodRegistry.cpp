#include "AutomationMethodRegistry.h"

#include <utility>

#include "AutomationClassMethodRegistryCore.h"
#include "AutomationRegistryDetail.h"

namespace {
bool IsActorMethodPermissionAllowed(EAutomationPermission Permission) {
  return Permission == EAutomationPermission::ReadOnly ||
         Permission == EAutomationPermission::WorldMutation;
}

constexpr AutomationRegistryDetail::FAutomationRegistryValidationMessages ActorMethodMessages{
    "actor method",
    "The method registry is frozen.",
    "ClassName must contain between 1 and 128 UTF-8 bytes.",
    "MethodName must match ^[a-z][a-z0-9_]{0,127}$.",
    "Description must not be empty.",
    "Actor methods require ReadOnly or WorldMutation permission.",
    "Handler must not be empty.",
    "The method is already registered for this actor class."
};
}  // namespace

bool FAutomationMethodRegistry::RegisterMethod(
    std::string ClassName, FAutomationMethodDescriptor Descriptor, std::string* OutError
) {
  return AutomationRegistryDetail::RegisterClassMethod(
      Frozen,
      MethodsByClass,
      std::move(ClassName),
      std::move(Descriptor),
      IsActorMethodPermissionAllowed,
      ActorMethodMessages,
      OutError
  );
}

const FAutomationMethodDescriptor* FAutomationMethodRegistry::FindMethod(
    std::string_view ClassName, std::string_view MethodName
) const {
  return AutomationRegistryDetail::FindClassMethod(MethodsByClass, ClassName, MethodName);
}

std::vector<FAutomationMethodSnapshot> FAutomationMethodRegistry::GetMethodsForClass(
    std::string_view ClassName
) const {
  return AutomationRegistryDetail::GetClassMethods<FAutomationMethodSnapshot>(
      MethodsByClass, ClassName
  );
}

void FAutomationMethodRegistry::Freeze() { Frozen = true; }

bool FAutomationMethodRegistry::IsFrozen() const { return Frozen; }
