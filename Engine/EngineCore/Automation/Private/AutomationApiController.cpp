#include "AutomationApiController.h"

#include <charconv>
#include <chrono>
#include <cmath>
#include <exception>
#include <stdexcept>
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
constexpr std::string_view InternalErrorMessage = "The automation request failed.";
constexpr std::string_view WorldNotAvailableMessage = "No world is currently available.";
constexpr std::string_view ActorNotFoundMessage = "The requested actor was not found.";
constexpr std::string_view InvalidActorIdMessage =
    "The actorId must be an unsigned decimal integer greater than zero.";

nlohmann::json MakeWorldReadError(EAutomationWorldReadStatus Status) {
  switch (Status) {
    case EAutomationWorldReadStatus::WorldNotAvailable:
      return MakeAutomationError(EAutomationErrorCode::WorldNotAvailable, WorldNotAvailableMessage);
    case EAutomationWorldReadStatus::ActorNotFound:
      return MakeAutomationError(EAutomationErrorCode::ActorNotFound, ActorNotFoundMessage);
    case EAutomationWorldReadStatus::InvalidState:
    case EAutomationWorldReadStatus::Success:
      return MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage);
  }
  return MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage);
}
}  // namespace

FAutomationApiController::FAutomationApiController(
    FAutomationCommandQueue& InCommandQueue,
    const FAutomationConfig& InConfig,
    FAutomationStateProvider InStateProvider,
    FAutomationActorListProvider InActorListProvider,
    FAutomationActorProvider InActorProvider
)
    : CommandQueue(InCommandQueue),
      Config(InConfig),
      StateProvider(std::move(InStateProvider)),
      ActorListProvider(std::move(InActorListProvider)),
      ActorProvider(std::move(InActorProvider)) {}

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
  } catch (const std::exception&) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
  } catch (...) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, UnknownExceptionMessage)};
  }
}

FAutomationHttpResponse FAutomationApiController::GetWorldActors() {
  try {
    if (!ActorListProvider) {
      return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
    }

    FAutomationCommandTicket Ticket = CommandQueue.Enqueue([Provider = ActorListProvider]() {
      FAutomationActorListSnapshot Snapshot;
      const EAutomationWorldReadStatus Status = Provider(Snapshot);
      if (Status != EAutomationWorldReadStatus::Success) {
        return MakeWorldReadError(Status);
      }
      return MakeAutomationSuccess(SerializeActorList(Snapshot));
    });
    return WaitForResult(std::move(Ticket));
  } catch (...) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
  }
}

FAutomationHttpResponse FAutomationApiController::GetWorldActor(std::string_view ActorIdText) {
  FActorId ActorId = InvalidActorId;
  if (!TryParseActorId(ActorIdText, ActorId)) {
    return {400, MakeAutomationError(EAutomationErrorCode::InvalidArgument, InvalidActorIdMessage)};
  }

  try {
    if (!ActorProvider) {
      return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
    }

    FAutomationCommandTicket Ticket = CommandQueue.Enqueue([Provider = ActorProvider, ActorId]() {
      FAutomationActorSnapshot Snapshot;
      const EAutomationWorldReadStatus Status = Provider(ActorId, Snapshot);
      if (Status != EAutomationWorldReadStatus::Success) {
        return MakeWorldReadError(Status);
      }
      return MakeAutomationSuccess(SerializeActor(Snapshot));
    });
    return WaitForResult(std::move(Ticket));
  } catch (...) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
  }
}

bool FAutomationApiController::TryParseActorId(std::string_view Text, FActorId& OutActorId) {
  OutActorId = InvalidActorId;
  if (Text.empty()) {
    return false;
  }

  FActorId ParsedActorId = InvalidActorId;
  const char* Begin = Text.data();
  const char* End = Begin + Text.size();
  const auto [Position, Error] = std::from_chars(Begin, End, ParsedActorId, 10);
  if (Error != std::errc() || Position != End || ParsedActorId == InvalidActorId) {
    return false;
  }

  OutActorId = ParsedActorId;
  return true;
}

nlohmann::json FAutomationApiController::SerializeActor(const FAutomationActorSnapshot& Actor) {
  const float LocationX = Actor.Location.X;
  const float LocationY = Actor.Location.Y;
  const float Rotation = Actor.Rotation.Rotation;
  const float Scale = Actor.Scale.Scale;
  if (Actor.ActorId == InvalidActorId || Actor.InstanceName.empty() || Actor.ClassName.empty() ||
      !std::isfinite(LocationX) || !std::isfinite(LocationY) || !std::isfinite(Rotation) ||
      !std::isfinite(Scale)) {
    throw std::runtime_error("Invalid actor snapshot");
  }

  return {
      {"actorId", Actor.ActorId},
      {"instanceName", Actor.InstanceName},
      {"className", Actor.ClassName},
      {"transform",
       {{"location", {{"x", LocationX}, {"y", LocationY}}},
        {"rotation", Rotation},
        {"scale", Scale}}}
  };
}

nlohmann::json FAutomationApiController::SerializeActorList(
    const FAutomationActorListSnapshot& Snapshot
) {
  nlohmann::json Actors = nlohmann::json::array();
  for (const FAutomationActorSnapshot& Actor : Snapshot.Actors) {
    Actors.push_back(SerializeActor(Actor));
  }
  return {
      {"sceneName", Snapshot.SceneName},
      {"actorCount", Snapshot.Actors.size()},
      {"actors", std::move(Actors)}
  };
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
  if (ErrorCode == "WORLD_NOT_AVAILABLE") {
    return 503;
  }
  if (ErrorCode == "ACTOR_NOT_FOUND") {
    return 404;
  }
  if (ErrorCode == "REQUEST_TIMEOUT") {
    return 504;
  }
  return 500;
}
