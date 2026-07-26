#include "AutomationTypes.h"

#include <string>
#include <utility>

std::string_view ToAutomationErrorCodeString(EAutomationErrorCode ErrorCode) {
  switch (ErrorCode) {
    case EAutomationErrorCode::None:
      return "NONE";
    case EAutomationErrorCode::InvalidRequest:
      return "INVALID_REQUEST";
    case EAutomationErrorCode::InvalidJson:
      return "INVALID_JSON";
    case EAutomationErrorCode::InvalidArgument:
      return "INVALID_ARGUMENT";
    case EAutomationErrorCode::RequestTooLarge:
      return "REQUEST_TOO_LARGE";
    case EAutomationErrorCode::WorldNotAvailable:
      return "WORLD_NOT_AVAILABLE";
    case EAutomationErrorCode::ActorNotFound:
      return "ACTOR_NOT_FOUND";
    case EAutomationErrorCode::ClassNotRegistered:
      return "CLASS_NOT_REGISTERED";
    case EAutomationErrorCode::ActorPendingDestroy:
      return "ACTOR_PENDING_DESTROY";
    case EAutomationErrorCode::Conflict:
      return "CONFLICT";
    case EAutomationErrorCode::MethodNotRegistered:
      return "METHOD_NOT_REGISTERED";
    case EAutomationErrorCode::PermissionDenied:
      return "PERMISSION_DENIED";
    case EAutomationErrorCode::CommandNotRegistered:
      return "COMMAND_NOT_REGISTERED";
    case EAutomationErrorCode::RequestTimeout:
      return "REQUEST_TIMEOUT";
    case EAutomationErrorCode::EngineShuttingDown:
      return "ENGINE_SHUTTING_DOWN";
    case EAutomationErrorCode::InternalError:
      return "INTERNAL_ERROR";
  }

  return "INTERNAL_ERROR";
}

std::string_view ToAutomationPermissionString(EAutomationPermission Permission) {
  switch (Permission) {
    case EAutomationPermission::ReadOnly:
      return "ReadOnly";
    case EAutomationPermission::WorldMutation:
      return "WorldMutation";
    case EAutomationPermission::SystemMutation:
      return "SystemMutation";
    case EAutomationPermission::Dangerous:
      return "Dangerous";
  }
  return "Dangerous";
}

bool IsValidAutomationOperationName(std::string_view Name) {
  if (Name.empty() || Name.size() > 128 || Name.front() < 'a' || Name.front() > 'z') {
    return false;
  }
  for (const char Character : Name.substr(1)) {
    if ((Character < 'a' || Character > 'z') && (Character < '0' || Character > '9') &&
        Character != '_') {
      return false;
    }
  }
  return true;
}

nlohmann::json MakeAutomationSuccess(nlohmann::json Data) {
  return {{"success", true}, {"data", std::move(Data)}};
}

nlohmann::json MakeAutomationError(EAutomationErrorCode ErrorCode, std::string_view Message) {
  return {
      {"success", false},
      {"error",
       {{"code", ToAutomationErrorCodeString(ErrorCode)}, {"message", std::string(Message)}}}
  };
}
