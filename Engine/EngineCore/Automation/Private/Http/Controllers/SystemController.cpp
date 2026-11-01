#include "SystemController.h"

#include "../Detail/HttpErrorMapping.h"
#include "../Detail/HttpParsing.h"
#include "../Detail/HttpSerialization.h"
#include "Application.h"

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
        Commands.push_back(
            {{"name", Snapshot.Name},
             {"description", Snapshot.Description},
             {"inputSchema", Snapshot.InputSchema}}
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

          const auto* Control = Application::GetUpdateControl();
          if (Control && Control->IsShuttingDown()) {
            return MakeAutomationError(
                EAutomationErrorCode::EngineShuttingDown, "Engine shutting down."
            );
          }
          if (Control && !Control->CanMutate() && CommandNameText != "stop_engine_update" &&
              CommandNameText != "operation_status" && CommandNameText != "quit_game") {
            return MakeAutomationError(
                EAutomationErrorCode::Conflict, "Mutation is not allowed during Engine stepping."
            );
          }
          nlohmann::json Result = Descriptor->Handler(Arguments);
          const bool IsExecutionControlCommand =
              CommandNameText == "stop_engine_update" || CommandNameText == "start_engine_update" ||
              CommandNameText == "step_engine_update" || CommandNameText == "operation_status" ||
              CommandNameText == "open_level_by_id" || CommandNameText == "open_level_by_path";
          if (IsExecutionControlCommand && Result.is_object() && Result.contains("success") &&
              !Result.value("success", true)) {
            return Result;
          }
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
