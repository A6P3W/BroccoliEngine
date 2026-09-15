#include "SystemController.h"
#include "../Detail/HttpErrorMapping.h"
#include "../Detail/HttpParsing.h"
#include "../Detail/HttpSerialization.h"

using namespace AutomationHttpDetail;

#include "Registry/Schema/SchemaValidator.h"

FAutomationSystemController::FAutomationSystemController(
    FAutomationHttpRequestExecutor& InExecutor,
    FAutomationCommandQueue& InCommandQueue,
    FAutomationSystemCommandRegistry& InSystemCommandRegistry
)
    : FAutomationHttpControllerBase(InExecutor),
      CommandQueue(InCommandQueue),
      SystemCommandRegistry(&InSystemCommandRegistry) {}

FAutomationHttpResponse FAutomationSystemController::GetSystemCommands() {
  try {
    if (!SystemCommandRegistry) {
      return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
    }

    FAutomationCommandTicket Ticket = CommandQueue.Enqueue([Registry = SystemCommandRegistry]() {
      nlohmann::json Commands = nlohmann::json::array();
      for (const FAutomationSystemCommandSnapshot& Snapshot : Registry->GetCommands()) {
        if (Snapshot.Permission != EAutomationPermission::SystemMutation) {
          continue;
        }
        Commands.push_back(
            {{"name", Snapshot.Name},
             {"description", Snapshot.Description},
             {"inputSchema", Snapshot.InputSchema},
             {"permission", ToAutomationPermissionString(Snapshot.Permission)}}
        );
      }
      return MakeAutomationSuccess({{"commands", std::move(Commands)}});
    });
    return WaitForResult(std::move(Ticket));
  } catch (...) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
  }
}

FAutomationHttpResponse FAutomationSystemController::ExecuteSystemCommand(
    std::string_view CommandName, const nlohmann::json& Body
) {
  if (!IsValidAutomationOperationName(CommandName)) {
    return {
        400, MakeAutomationError(EAutomationErrorCode::InvalidArgument, InvalidCommandNameMessage)
    };
  }
  if (!Body.is_object()) {
    return {
        400,
        MakeAutomationError(
            EAutomationErrorCode::InvalidArgument, "The system command arguments must be an object."
        )
    };
  }

  try {
    if (!SystemCommandRegistry) {
      return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
    }

    FAutomationCommandTicket Ticket =
        CommandQueue.Enqueue([Registry = SystemCommandRegistry,
                              CommandNameText = std::string(CommandName),
                              Arguments = Body]() {
          const FAutomationSystemCommandDescriptor* Descriptor =
              Registry->FindCommand(CommandNameText);
          if (!Descriptor) {
            M_LOG(
                Log,
                "Automation system command rejected: command={} "
                "code=COMMAND_NOT_REGISTERED",
                CommandNameText
            );
            return MakeAutomationError(
                EAutomationErrorCode::CommandNotRegistered, CommandNotRegisteredMessage
            );
          }
          if (Descriptor->Permission != EAutomationPermission::SystemMutation) {
            M_LOG(
                Log,
                "Automation system command rejected: command={} "
                "code=PERMISSION_DENIED",
                CommandNameText
            );
            return MakeAutomationError(
                EAutomationErrorCode::PermissionDenied, CommandPermissionDeniedMessage
            );
          }

          FAutomationSchemaValidationError ValidationError;
          if (!FAutomationJsonSchemaValidator::ValidateValue(
                  Descriptor->InputSchema, Arguments, ValidationError
              )) {
            M_LOG(
                Log,
                "Automation system command rejected: command={} "
                "code=INVALID_ARGUMENT",
                CommandNameText
            );
            return MakeAutomationError(
                EAutomationErrorCode::InvalidArgument,
                ValidationError.JsonPath + ": " + ValidationError.Message
            );
          }

          nlohmann::json Result = Descriptor->Handler(Arguments);
          M_LOG(Log, "Automation system command completed: command={}", CommandNameText);
          return MakeAutomationSuccess(
              {{"commandName", CommandNameText}, {"result", std::move(Result)}}
          );
        });
    return WaitForResult(std::move(Ticket));
  } catch (...) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
  }
}
