#include "AutomationApiController.h"

#include <chrono>
#include <exception>
#include <string>
#include <utility>

#include "Log.h"

namespace {
constexpr std::string_view QueueUnavailableMessage =
    "The automation command queue is not accepting requests.";
constexpr std::string_view RequestTimeoutMessage =
    "The main thread did not complete the request before the timeout.";
constexpr std::string_view InvalidResponseMessage =
    "The automation command returned an invalid response.";
constexpr std::string_view UnknownExceptionMessage =
    "The automation request failed with an unknown exception.";
}  // namespace

FAutomationApiController::FAutomationApiController(
    FAutomationCommandQueue& InCommandQueue,
    const FAutomationConfig& InConfig,
    FAutomationStateProvider InStateProvider
)
    : CommandQueue(InCommandQueue), Config(InConfig), StateProvider(std::move(InStateProvider)) {}

FAutomationHttpResponse FAutomationApiController::GetState() {
  try {
    if (!StateProvider) {
      return {
          500,
          MakeAutomationError(
              EAutomationErrorCode::InternalError, "The automation state provider is unavailable."
          )
      };
    }

    FAutomationCommandTicket Ticket = CommandQueue.Enqueue([Provider = StateProvider]() {
      return MakeAutomationSuccess(Provider());
    });
    return WaitForResult(std::move(Ticket));
  } catch (const std::exception& Exception) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, Exception.what())};
  } catch (...) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, UnknownExceptionMessage)};
  }
}

FAutomationHttpResponse FAutomationApiController::WaitForResult(FAutomationCommandTicket&& Ticket) {
  if (!Ticket.IsValid()) {
    return {
        503, MakeAutomationError(EAutomationErrorCode::EngineShuttingDown, QueueUnavailableMessage)
    };
  }

  const std::future_status Status =
      Ticket.Result.wait_for(std::chrono::seconds(Config.RequestTimeoutSeconds));
  if (Status != std::future_status::ready) {
    Ticket.Cancel();
    M_LOG("Automation request timed out.");
    return {504, MakeAutomationError(EAutomationErrorCode::RequestTimeout, RequestTimeoutMessage)};
  }

  nlohmann::json Body = Ticket.Result.get();
  if (!Body.is_object() || !Body.contains("success") || !Body["success"].is_boolean()) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InvalidResponseMessage)};
  }

  return {GetHttpStatusCode(Body), std::move(Body)};
}

int FAutomationApiController::GetHttpStatusCode(const nlohmann::json& Body) {
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
  if (ErrorCode == "REQUEST_TIMEOUT") {
    return 504;
  }
  return 500;
}
