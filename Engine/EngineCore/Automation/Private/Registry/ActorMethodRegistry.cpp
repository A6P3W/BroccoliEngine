#include "ActorMethodRegistry.h"

#include <utility>

#include "RegistryCommon.h"

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

bool FAutomationActorMethodRegistry::RegisterMethod(
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

const FAutomationMethodDescriptor* FAutomationActorMethodRegistry::FindMethod(
    std::string_view ClassName, std::string_view MethodName
) const {
  return AutomationRegistryDetail::FindClassMethod(MethodsByClass, ClassName, MethodName);
}

std::vector<FAutomationMethodSnapshot> FAutomationActorMethodRegistry::GetMethodsForClass(
    std::string_view ClassName
) const {
  return AutomationRegistryDetail::GetClassMethods<FAutomationMethodSnapshot>(
      MethodsByClass, ClassName
  );
}

void FAutomationActorMethodRegistry::Freeze() { Frozen = true; }

bool FAutomationActorMethodRegistry::IsFrozen() const { return Frozen; }
