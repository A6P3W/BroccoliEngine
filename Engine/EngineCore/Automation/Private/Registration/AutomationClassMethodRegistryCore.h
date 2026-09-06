#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "AutomationRegistryDetail.h"

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
