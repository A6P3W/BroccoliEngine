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
    case EAutomationErrorCode::RequestTimeout:
      return "REQUEST_TIMEOUT";
    case EAutomationErrorCode::EngineShuttingDown:
      return "ENGINE_SHUTTING_DOWN";
    case EAutomationErrorCode::InternalError:
      return "INTERNAL_ERROR";
  }

  return "INTERNAL_ERROR";
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
