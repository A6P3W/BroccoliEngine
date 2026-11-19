#include "AutomationError.h"

#include <string>

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
    case EAutomationErrorCode::ComponentNotFound:
      return "COMPONENT_NOT_FOUND";
    case EAutomationErrorCode::ComponentPendingDestroy:
      return "COMPONENT_PENDING_DESTROY";
    case EAutomationErrorCode::Conflict:
      return "CONFLICT";
    case EAutomationErrorCode::MethodNotRegistered:
      return "METHOD_NOT_REGISTERED";
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

nlohmann::json MakeAutomationError(EAutomationErrorCode ErrorCode, std::string_view Message) {
  return {
      {"success", false},
      {"error",
       {{"code", ToAutomationErrorCodeString(ErrorCode)}, {"message", std::string(Message)}}}
  };
}
