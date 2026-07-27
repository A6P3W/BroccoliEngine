#include "AutomationComponentMethodRegistry.h"

#include <algorithm>
#include <utility>

#include "AutomationJsonSchemaValidator.h"
#include "AutomationRegistryDetail.h"

namespace {
bool IsValidClassName(std::string_view ClassName) {
  return !ClassName.empty() && ClassName.size() <= 128;
}

bool IsComponentMethodPermissionAllowed(EAutomationPermission Permission) {
  return Permission == EAutomationPermission::ReadOnly ||
         Permission == EAutomationPermission::WorldMutation;
}
}  // namespace

bool FAutomationComponentMethodRegistry::RegisterMethod(
    std::string ClassName, FAutomationComponentMethodDescriptor Descriptor, std::string* OutError
) {
  if (Frozen) {
    return AutomationRegistryDetail::RejectRegistration(
        "The component method registry is frozen.", OutError, "component method"
    );
  }
  if (!IsValidClassName(ClassName)) {
    return AutomationRegistryDetail::RejectRegistration(
        "ClassName must contain between 1 and 128 UTF-8 bytes.", OutError, "component method"
    );
  }
  if (!IsValidAutomationOperationName(Descriptor.Name)) {
    return AutomationRegistryDetail::RejectRegistration(
        "MethodName must match ^[a-z][a-z0-9_]{0,127}$.", OutError, "component method"
    );
  }
  if (Descriptor.Description.empty()) {
    return AutomationRegistryDetail::RejectRegistration(
        "Description must not be empty.", OutError, "component method"
    );
  }
  if (!IsComponentMethodPermissionAllowed(Descriptor.Permission)) {
    return AutomationRegistryDetail::RejectRegistration(
        "Component methods require ReadOnly or WorldMutation permission.",
        OutError,
        "component method"
    );
  }
  if (!Descriptor.Handler) {
    return AutomationRegistryDetail::RejectRegistration(
        "Handler must not be empty.", OutError, "component method"
    );
  }

  FAutomationSchemaValidationError ValidationError;
  if (!FAutomationJsonSchemaValidator::ValidateSchemaDefinition(
          Descriptor.InputSchema, ValidationError
      )) {
    return AutomationRegistryDetail::RejectRegistration(
        ValidationError.JsonPath + ": " + ValidationError.Message, OutError, "component method"
    );
  }

  FMethodMap& Methods = MethodsByClass[ClassName];
  if (Methods.contains(Descriptor.Name)) {
    return AutomationRegistryDetail::RejectRegistration(
        "The method is already registered for this component class.", OutError, "component method"
    );
  }
  Methods.emplace(Descriptor.Name, std::move(Descriptor));
  return true;
}

const FAutomationComponentMethodDescriptor* FAutomationComponentMethodRegistry::FindMethod(
    std::string_view ClassName, std::string_view MethodName
) const {
  const auto ClassIterator = MethodsByClass.find(std::string(ClassName));
  if (ClassIterator == MethodsByClass.end()) {
    return nullptr;
  }
  const auto MethodIterator = ClassIterator->second.find(std::string(MethodName));
  return MethodIterator == ClassIterator->second.end() ? nullptr : &MethodIterator->second;
}

std::vector<FAutomationComponentMethodSnapshot>
FAutomationComponentMethodRegistry::GetMethodsForClass(std::string_view ClassName) const {
  std::vector<FAutomationComponentMethodSnapshot> Result;
  const auto ClassIterator = MethodsByClass.find(std::string(ClassName));
  if (ClassIterator == MethodsByClass.end()) {
    return Result;
  }
  Result.reserve(ClassIterator->second.size());
  for (const auto& [MethodName, Descriptor] : ClassIterator->second) {
    (void)MethodName;
    Result.push_back(
        {Descriptor.Name, Descriptor.Description, Descriptor.InputSchema, Descriptor.Permission}
    );
  }
  std::sort(
      Result.begin(),
      Result.end(),
      [](const FAutomationComponentMethodSnapshot& Left,
         const FAutomationComponentMethodSnapshot& Right) { return Left.Name < Right.Name; }
  );
  return Result;
}

void FAutomationComponentMethodRegistry::Freeze() { Frozen = true; }

bool FAutomationComponentMethodRegistry::IsFrozen() const { return Frozen; }
