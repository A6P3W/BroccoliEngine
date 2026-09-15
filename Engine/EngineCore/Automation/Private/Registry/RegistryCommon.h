#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Schema/SchemaValidator.h"
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

  M_LOG(Log, "Automation {} registration rejected: {}", RegistryName, Message);
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

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace AutomationRegistryDetail {
template <class TMethodsByClass, class TDescriptor, class TPermissionValidator>
bool RegisterClassMethod(
    bool Frozen,
    TMethodsByClass& MethodsByClass,
    std::string ClassName,
    TDescriptor Descriptor,
    TPermissionValidator PermissionValidator,
    const FAutomationRegistryValidationMessages& Messages,
    std::string* OutError
) {
  if (Frozen) {
    return RejectRegistration(std::string(Messages.Frozen), OutError, Messages.RegistryName);
  }
  if (!IsValidClassName(ClassName)) {
    return RejectRegistration(
        std::string(Messages.InvalidClassName), OutError, Messages.RegistryName
    );
  }
  if (!ValidateCallableDescriptor(Descriptor, PermissionValidator, Messages, OutError)) {
    return false;
  }

  auto& Methods = MethodsByClass[ClassName];
  if (Methods.contains(Descriptor.Name)) {
    return RejectRegistration(std::string(Messages.Duplicate), OutError, Messages.RegistryName);
  }

  const std::string MethodName = Descriptor.Name;
  Methods.emplace(MethodName, std::move(Descriptor));
  return true;
}

template <class TMethodsByClass>
const typename TMethodsByClass::mapped_type::mapped_type* FindClassMethod(
    const TMethodsByClass& MethodsByClass, std::string_view ClassName, std::string_view MethodName
) {
  const auto ClassIterator = MethodsByClass.find(std::string(ClassName));
  if (ClassIterator == MethodsByClass.end()) {
    return nullptr;
  }

  const auto MethodIterator = ClassIterator->second.find(std::string(MethodName));
  return MethodIterator == ClassIterator->second.end() ? nullptr : &MethodIterator->second;
}

template <class TSnapshot, class TMethodsByClass>
std::vector<TSnapshot> GetClassMethods(
    const TMethodsByClass& MethodsByClass, std::string_view ClassName
) {
  std::vector<TSnapshot> Result;
  const auto ClassIterator = MethodsByClass.find(std::string(ClassName));
  if (ClassIterator == MethodsByClass.end()) {
    return Result;
  }

  Result.reserve(ClassIterator->second.size());
  for (const auto& [MethodName, Descriptor] : ClassIterator->second) {
    (void)MethodName;
    Result.push_back(MakeSnapshot<TSnapshot>(Descriptor));
  }
  SortSnapshotsByName(Result);
  return Result;
}
}  // namespace AutomationRegistryDetail
