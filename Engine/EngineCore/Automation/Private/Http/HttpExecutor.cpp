#include "Http/HttpExecutor.h"
#include "Detail/HttpErrorMapping.h"

using namespace AutomationHttpDetail;

#include <chrono>
#include <future>

#include "Log.h"

FAutomationHttpRequestExecutor::FAutomationHttpRequestExecutor(
    FAutomationCommandQueue& InCommandQueue, const FAutomationConfig& InConfig
)
    : CommandQueue(InCommandQueue), Config(InConfig) {}

FAutomationHttpResponse FAutomationHttpRequestExecutor::WaitForResult(
    FAutomationCommandTicket&& Ticket
) {
  if (!Ticket.IsValid()) {
    return {
        503, MakeAutomationError(EAutomationErrorCode::EngineShuttingDown, QueueUnavailableMessage)
    };
  }

  const std::future_status Status =
      Ticket.Result.wait_for(std::chrono::seconds(Config.RequestTimeoutSeconds));
  if (Status != std::future_status::ready) {
    Ticket.Cancel();
    M_LOG(Log, "Automation request timed out.");
    return {504, MakeAutomationError(EAutomationErrorCode::RequestTimeout, RequestTimeoutMessage)};
  }

  nlohmann::json Body = Ticket.Result.get();
  if (!Body.is_object() || !Body.contains("success") || !Body["success"].is_boolean()) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InvalidResponseMessage)};
  }

  return {GetHttpStatusCode(Body), std::move(Body)};
}

int FAutomationHttpRequestExecutor::GetHttpStatusCode(const nlohmann::json& Body) {
  if (Body.value("success", false)) {
    return 200;
  }

  if (!Body.contains("error") || !Body["error"].is_object()) {
    return 500;
  }

  const std::string ErrorCode = Body["error"].value("code", "INTERNAL_ERROR");
  if (ErrorCode == "INVALID_REQUEST" || ErrorCode == "INVALID_JSON" ||
      ErrorCode == "INVALID_ARGUMENT") {
    return 400;
  }
  if (ErrorCode == "REQUEST_TOO_LARGE") {
    return 413;
  }
  if (ErrorCode == "ENGINE_SHUTTING_DOWN") {
    return 503;
  }
  if (ErrorCode == "WORLD_NOT_AVAILABLE") {
    return 503;
  }
  if (ErrorCode == "ACTOR_NOT_FOUND" || ErrorCode == "COMPONENT_NOT_FOUND") {
    return 404;
  }
  if (ErrorCode == "CLASS_NOT_REGISTERED") {
    return 404;
  }
  if (ErrorCode == "METHOD_NOT_REGISTERED" || ErrorCode == "COMMAND_NOT_REGISTERED") {
    return 404;
  }
  if (ErrorCode == "PERMISSION_DENIED") {
    return 403;
  }
  if (ErrorCode == "ACTOR_PENDING_DESTROY" || ErrorCode == "COMPONENT_PENDING_DESTROY" ||
      ErrorCode == "CONFLICT") {
    return 409;
  }
  if (ErrorCode == "REQUEST_TIMEOUT") {
    return 504;
  }
  return 500;
}
