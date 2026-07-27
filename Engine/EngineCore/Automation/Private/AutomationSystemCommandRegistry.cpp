#include "AutomationSystemCommandRegistry.h"

#include <algorithm>
#include <utility>

#include "AutomationJsonSchemaValidator.h"
#include "AutomationRegistryDetail.h"

bool FAutomationSystemCommandRegistry::RegisterCommand(
    FAutomationSystemCommandDescriptor Descriptor, std::string* OutError
) {
  if (Frozen) {
    return AutomationRegistryDetail::RejectRegistration(
        "The system command registry is frozen.", OutError, "system command"
    );
  }
  if (!IsValidAutomationOperationName(Descriptor.Name)) {
    return AutomationRegistryDetail::RejectRegistration(
        "CommandName must match ^[a-z][a-z0-9_]{0,127}$.", OutError, "system command"
    );
  }
  if (Descriptor.Description.empty()) {
    return AutomationRegistryDetail::RejectRegistration(
        "Description must not be empty.", OutError, "system command"
    );
  }
  if (Descriptor.Permission != EAutomationPermission::SystemMutation) {
    return AutomationRegistryDetail::RejectRegistration(
        "System commands require SystemMutation permission.", OutError, "system command"
    );
  }
  if (!Descriptor.Handler) {
    return AutomationRegistryDetail::RejectRegistration(
        "Handler must not be empty.", OutError, "system command"
    );
  }

  FAutomationSchemaValidationError ValidationError;
  if (!FAutomationJsonSchemaValidator::ValidateSchemaDefinition(
          Descriptor.InputSchema, ValidationError
      )) {
    return AutomationRegistryDetail::RejectRegistration(
        ValidationError.JsonPath + ": " + ValidationError.Message, OutError, "system command"
    );
  }
  if (Commands.contains(Descriptor.Name)) {
    return AutomationRegistryDetail::RejectRegistration(
        "The system command is already registered.", OutError, "system command"
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
        {Descriptor.Name, Descriptor.Description, Descriptor.InputSchema, Descriptor.Permission}
    );
  }
  std::sort(
      Result.begin(),
      Result.end(),
      [](const FAutomationSystemCommandSnapshot& Left,
         const FAutomationSystemCommandSnapshot& Right) { return Left.Name < Right.Name; }
  );
  return Result;
}

void FAutomationSystemCommandRegistry::Freeze() { Frozen = true; }

bool FAutomationSystemCommandRegistry::IsFrozen() const { return Frozen; }
