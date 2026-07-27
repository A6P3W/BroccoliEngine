#include "AutomationMethodRegistry.h"

#include <algorithm>
#include <utility>

#include "AutomationJsonSchemaValidator.h"
#include "AutomationRegistryDetail.h"

namespace {
bool IsValidClassName(std::string_view ClassName) {
  return !ClassName.empty() && ClassName.size() <= 128;
}

bool IsActorMethodPermissionAllowed(EAutomationPermission Permission) {
  return Permission == EAutomationPermission::ReadOnly ||
         Permission == EAutomationPermission::WorldMutation;
}
}  // namespace

bool FAutomationMethodRegistry::RegisterMethod(
    std::string ClassName, FAutomationMethodDescriptor Descriptor, std::string* OutError
) {
  if (Frozen) {
    return AutomationRegistryDetail::RejectRegistration(
        "The method registry is frozen.", OutError, "actor method"
    );
  }
  if (!IsValidClassName(ClassName)) {
    return AutomationRegistryDetail::RejectRegistration(
        "ClassName must contain between 1 and 128 UTF-8 bytes.", OutError, "actor method"
    );
  }
  if (!IsValidAutomationOperationName(Descriptor.Name)) {
    return AutomationRegistryDetail::RejectRegistration(
        "MethodName must match ^[a-z][a-z0-9_]{0,127}$.", OutError, "actor method"
    );
  }
  if (Descriptor.Description.empty()) {
    return AutomationRegistryDetail::RejectRegistration(
        "Description must not be empty.", OutError, "actor method"
    );
  }
  if (!IsActorMethodPermissionAllowed(Descriptor.Permission)) {
    return AutomationRegistryDetail::RejectRegistration(
        "Actor methods require ReadOnly or WorldMutation permission.", OutError, "actor method"
    );
  }
  if (!Descriptor.Handler) {
    return AutomationRegistryDetail::RejectRegistration(
        "Handler must not be empty.", OutError, "actor method"
    );
  }

  FAutomationSchemaValidationError ValidationError;
  if (!FAutomationJsonSchemaValidator::ValidateSchemaDefinition(
          Descriptor.InputSchema, ValidationError
      )) {
    return AutomationRegistryDetail::RejectRegistration(
        ValidationError.JsonPath + ": " + ValidationError.Message, OutError, "actor method"
    );
  }

  FMethodMap& Methods = MethodsByClass[ClassName];
  if (Methods.contains(Descriptor.Name)) {
    return AutomationRegistryDetail::RejectRegistration(
        "The method is already registered for this actor class.", OutError, "actor method"
    );
  }

  Methods.emplace(Descriptor.Name, std::move(Descriptor));
  return true;
}

const FAutomationMethodDescriptor* FAutomationMethodRegistry::FindMethod(
    std::string_view ClassName, std::string_view MethodName
) const {
  const auto ClassIterator = MethodsByClass.find(std::string(ClassName));
  if (ClassIterator == MethodsByClass.end()) {
    return nullptr;
  }
  const auto MethodIterator = ClassIterator->second.find(std::string(MethodName));
  return MethodIterator == ClassIterator->second.end() ? nullptr : &MethodIterator->second;
}

std::vector<FAutomationMethodSnapshot> FAutomationMethodRegistry::GetMethodsForClass(
    std::string_view ClassName
) const {
  std::vector<FAutomationMethodSnapshot> Result;
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
      [](const FAutomationMethodSnapshot& Left, const FAutomationMethodSnapshot& Right) {
        return Left.Name < Right.Name;
      }
  );
  return Result;
}

void FAutomationMethodRegistry::Freeze() { Frozen = true; }

bool FAutomationMethodRegistry::IsFrozen() const { return Frozen; }
