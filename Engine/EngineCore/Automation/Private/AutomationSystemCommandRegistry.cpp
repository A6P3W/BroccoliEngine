#include "AutomationSystemCommandRegistry.h"

#include <algorithm>
#include <utility>

#include "AutomationJsonSchemaValidator.h"
#include "Log.h"

namespace {
bool RejectRegistration(std::string Message, std::string* OutError) {
  if (OutError) {
    *OutError = Message;
  }
  M_LOG("Automation system command registration rejected: {}", Message);
  return false;
}
}  // namespace

bool FAutomationSystemCommandRegistry::RegisterCommand(
    FAutomationSystemCommandDescriptor Descriptor, std::string* OutError
) {
  if (Frozen) {
    return RejectRegistration("The system command registry is frozen.", OutError);
  }
  if (!IsValidAutomationOperationName(Descriptor.Name)) {
    return RejectRegistration("CommandName must match ^[a-z][a-z0-9_]{0,127}$.", OutError);
  }
  if (Descriptor.Description.empty()) {
    return RejectRegistration("Description must not be empty.", OutError);
  }
  if (Descriptor.Permission != EAutomationPermission::SystemMutation) {
    return RejectRegistration("System commands require SystemMutation permission.", OutError);
  }
  if (!Descriptor.Handler) {
    return RejectRegistration("Handler must not be empty.", OutError);
  }

  FAutomationSchemaValidationError ValidationError;
  if (!FAutomationJsonSchemaValidator::ValidateSchemaDefinition(
          Descriptor.InputSchema, ValidationError
      )) {
    return RejectRegistration(ValidationError.JsonPath + ": " + ValidationError.Message, OutError);
  }
  if (Commands.contains(Descriptor.Name)) {
    return RejectRegistration("The system command is already registered.", OutError);
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
