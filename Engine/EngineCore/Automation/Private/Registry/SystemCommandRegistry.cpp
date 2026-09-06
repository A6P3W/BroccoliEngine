#include "SystemCommandRegistry.h"

#include <utility>

#include "RegistryCommon.h"

namespace {
bool IsSystemCommandPermissionAllowed(EAutomationPermission Permission) {
  return Permission == EAutomationPermission::SystemMutation;
}

constexpr AutomationRegistryDetail::FAutomationRegistryValidationMessages SystemCommandMessages{
    "system command",
    "The system command registry is frozen.",
    "",
    "CommandName must match ^[a-z][a-z0-9_]{0,127}$.",
    "Description must not be empty.",
    "System commands require SystemMutation permission.",
    "Handler must not be empty.",
    "The system command is already registered."
};
}  // namespace

bool FAutomationSystemCommandRegistry::RegisterCommand(
    FAutomationSystemCommandDescriptor Descriptor, std::string* OutError
) {
  if (Frozen) {
    return AutomationRegistryDetail::RejectRegistration(
        std::string(SystemCommandMessages.Frozen), OutError, SystemCommandMessages.RegistryName
    );
  }
  if (!AutomationRegistryDetail::ValidateCallableDescriptor(
          Descriptor, IsSystemCommandPermissionAllowed, SystemCommandMessages, OutError
      )) {
    return false;
  }
  if (Commands.contains(Descriptor.Name)) {
    return AutomationRegistryDetail::RejectRegistration(
        std::string(SystemCommandMessages.Duplicate), OutError, SystemCommandMessages.RegistryName
    );
  }

  const std::string Name = Descriptor.Name;
  Commands.emplace(Name, std::move(Descriptor));
  return true;
}

const FAutomationSystemCommandDescriptor* FAutomationSystemCommandRegistry::FindCommand(
    std::string_view CommandName
) const {
  const auto Iterator = Commands.find(std::string(CommandName));
  return Iterator == Commands.end() ? nullptr : &Iterator->second;
}

std::vector<FAutomationSystemCommandSnapshot>
FAutomationSystemCommandRegistry::GetCommands() const {
  std::vector<FAutomationSystemCommandSnapshot> Result;
  Result.reserve(Commands.size());
  for (const auto& [CommandName, Descriptor] : Commands) {
    (void)CommandName;
    Result.push_back(
        AutomationRegistryDetail::MakeSnapshot<FAutomationSystemCommandSnapshot>(Descriptor)
    );
  }
  AutomationRegistryDetail::SortSnapshotsByName(Result);
  return Result;
}

void FAutomationSystemCommandRegistry::Freeze() { Frozen = true; }

bool FAutomationSystemCommandRegistry::IsFrozen() const { return Frozen; }
