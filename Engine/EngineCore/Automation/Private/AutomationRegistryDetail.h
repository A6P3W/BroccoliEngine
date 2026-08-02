#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "AutomationJsonSchemaValidator.h"
#include "Log.h"

namespace AutomationRegistryDetail {
struct FAutomationRegistryValidationMessages {
  std::string_view RegistryName;
  std::string_view Frozen;
  std::string_view InvalidClassName;
  std::string_view InvalidOperationName;
  std::string_view EmptyDescription;
  std::string_view InvalidPermission;
  std::string_view EmptyHandler;
  std::string_view Duplicate;
};

inline bool RejectRegistration(
    std::string Message, std::string* OutError, std::string_view RegistryName
) {
  if (OutError) {
    *OutError = Message;
  }

  M_LOG("Automation {} registration rejected: {}", RegistryName, Message);
  return false;
}

inline bool IsValidClassName(std::string_view ClassName) {
  return !ClassName.empty() && ClassName.size() <= 128;
}

template <class TDescriptor, class TPermissionValidator>
bool ValidateCallableDescriptor(
    const TDescriptor& Descriptor,
    TPermissionValidator PermissionValidator,
    const FAutomationRegistryValidationMessages& Messages,
    std::string* OutError
) {
  if (!IsValidAutomationOperationName(Descriptor.Name)) {
    return RejectRegistration(
        std::string(Messages.InvalidOperationName), OutError, Messages.RegistryName
    );
  }
  if (Descriptor.Description.empty()) {
    return RejectRegistration(
        std::string(Messages.EmptyDescription), OutError, Messages.RegistryName
    );
  }
  if (!PermissionValidator(Descriptor.Permission)) {
    return RejectRegistration(
        std::string(Messages.InvalidPermission), OutError, Messages.RegistryName
    );
  }
  if (!Descriptor.Handler) {
    return RejectRegistration(std::string(Messages.EmptyHandler), OutError, Messages.RegistryName);
  }

  FAutomationSchemaValidationError ValidationError;
  if (!FAutomationJsonSchemaValidator::ValidateSchemaDefinition(
          Descriptor.InputSchema, ValidationError
      )) {
    return RejectRegistration(
        ValidationError.JsonPath + ": " + ValidationError.Message, OutError, Messages.RegistryName
    );
  }
  return true;
}

template <class TSnapshot, class TDescriptor>
TSnapshot MakeSnapshot(const TDescriptor& Descriptor) {
  return {Descriptor.Name, Descriptor.Description, Descriptor.InputSchema, Descriptor.Permission};
}

template <class TSnapshot>
void SortSnapshotsByName(std::vector<TSnapshot>& Snapshots) {
  std::sort(Snapshots.begin(), Snapshots.end(), [](const TSnapshot& Left, const TSnapshot& Right) {
    return Left.Name < Right.Name;
  });
}
}  // namespace AutomationRegistryDetail
