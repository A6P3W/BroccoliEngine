#include "AutomationApiController.h"

#include <charconv>
#include <chrono>
#include <cmath>
#include <exception>
#include <initializer_list>
#include <limits>
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
constexpr std::string_view ClassNotRegisteredMessage =
    "The requested actor class is not registered.";
constexpr std::string_view ActorPendingDestroyMessage =
    "The requested actor is pending destruction.";
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

nlohmann::json MakeWorldMutationError(EAutomationWorldMutationStatus Status) {
  switch (Status) {
    case EAutomationWorldMutationStatus::WorldNotAvailable:
      return MakeAutomationError(EAutomationErrorCode::WorldNotAvailable, WorldNotAvailableMessage);
    case EAutomationWorldMutationStatus::ClassNotRegistered:
      return MakeAutomationError(
          EAutomationErrorCode::ClassNotRegistered, ClassNotRegisteredMessage
      );
    case EAutomationWorldMutationStatus::ActorNotFound:
      return MakeAutomationError(EAutomationErrorCode::ActorNotFound, ActorNotFoundMessage);
    case EAutomationWorldMutationStatus::ActorPendingDestroy:
      return MakeAutomationError(
          EAutomationErrorCode::ActorPendingDestroy, ActorPendingDestroyMessage
      );
    case EAutomationWorldMutationStatus::InvalidState:
    case EAutomationWorldMutationStatus::Success:
      return MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage);
  }
  return MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage);
}

bool HasOnlyAllowedFields(
    const nlohmann::json& Object, std::initializer_list<std::string_view> AllowedFields
) {
  if (!Object.is_object()) {
    return false;
  }
  for (const auto& [FieldName, Value] : Object.items()) {
    (void)Value;
    bool bAllowed = false;
    for (const std::string_view AllowedField : AllowedFields) {
      if (FieldName == AllowedField) {
        bAllowed = true;
        break;
      }
    }
    if (!bAllowed) {
      return false;
    }
  }
  return true;
}

bool TryReadFiniteFloat(
    const nlohmann::json& Object, std::string_view FieldName, float& OutValue, std::string& OutError
) {
  const std::string FieldNameString(FieldName);
  if (!Object.contains(FieldNameString) || !Object[FieldNameString].is_number()) {
    OutError = FieldNameString + " must be a number.";
    return false;
  }

  const double Value = Object[FieldNameString].get<double>();
  if (!std::isfinite(Value) ||
      Value < static_cast<double>((std::numeric_limits<float>::lowest)()) ||
      Value > static_cast<double>((std::numeric_limits<float>::max)())) {
    OutError = FieldNameString + " must be a finite 32-bit floating-point value.";
    return false;
  }

  OutValue = static_cast<float>(Value);
  return true;
}
}  // namespace

bool FAutomationTransformPatch::HasAnyValue() const {
  return Location.has_value() || Rotation.has_value() || Scale.has_value();
}

FAutomationApiController::FAutomationApiController(
    FAutomationCommandQueue& InCommandQueue,
    const FAutomationConfig& InConfig,
    FAutomationStateProvider InStateProvider,
    FAutomationActorListProvider InActorListProvider,
    FAutomationActorProvider InActorProvider,
    FAutomationSpawnActorProvider InSpawnActorProvider,
    FAutomationDestroyActorProvider InDestroyActorProvider,
    FAutomationPatchActorTransformProvider InPatchActorTransformProvider
)
    : CommandQueue(InCommandQueue),
      Config(InConfig),
      StateProvider(std::move(InStateProvider)),
      ActorListProvider(std::move(InActorListProvider)),
      ActorProvider(std::move(InActorProvider)),
      SpawnActorProvider(std::move(InSpawnActorProvider)),
      DestroyActorProvider(std::move(InDestroyActorProvider)),
      PatchActorTransformProvider(std::move(InPatchActorTransformProvider)) {}

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

FAutomationHttpResponse FAutomationApiController::CreateWorldActor(const nlohmann::json& Body) {
  FAutomationSpawnActorRequest Request;
  std::string Error;
  if (!TryParseSpawnRequest(Body, Request, Error)) {
    return {400, MakeAutomationError(EAutomationErrorCode::InvalidArgument, Error)};
  }

  try {
    if (!SpawnActorProvider) {
      return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
    }

    FAutomationCommandTicket Ticket =
        CommandQueue.Enqueue([Provider = SpawnActorProvider, Request = std::move(Request)]() {
          FAutomationActorSnapshot Snapshot;
          const EAutomationWorldMutationStatus Status = Provider(Request, Snapshot);
          if (Status != EAutomationWorldMutationStatus::Success) {
            return MakeWorldMutationError(Status);
          }
          return MakeAutomationSuccess(SerializeActor(Snapshot));
        });
    FAutomationHttpResponse Response = WaitForResult(std::move(Ticket));
    if (Response.Body.value("success", false)) {
      Response.StatusCode = 201;
    }
    return Response;
  } catch (...) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
  }
}

FAutomationHttpResponse FAutomationApiController::DeleteWorldActor(std::string_view ActorIdText) {
  FActorId ActorId = InvalidActorId;
  if (!TryParseActorId(ActorIdText, ActorId)) {
    return {400, MakeAutomationError(EAutomationErrorCode::InvalidArgument, InvalidActorIdMessage)};
  }

  try {
    if (!DestroyActorProvider) {
      return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
    }

    FAutomationCommandTicket Ticket =
        CommandQueue.Enqueue([Provider = DestroyActorProvider, ActorId]() {
          const EAutomationWorldMutationStatus Status = Provider(ActorId);
          if (Status != EAutomationWorldMutationStatus::Success) {
            return MakeWorldMutationError(Status);
          }
          return MakeAutomationSuccess({{"actorId", ActorId}, {"pendingDestroy", true}});
        });
    return WaitForResult(std::move(Ticket));
  } catch (...) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
  }
}

FAutomationHttpResponse FAutomationApiController::PatchWorldActorTransform(
    std::string_view ActorIdText, const nlohmann::json& Body
) {
  FActorId ActorId = InvalidActorId;
  if (!TryParseActorId(ActorIdText, ActorId)) {
    return {400, MakeAutomationError(EAutomationErrorCode::InvalidArgument, InvalidActorIdMessage)};
  }

  FAutomationTransformPatch Patch;
  std::string Error;
  if (!TryParseTransformPatch(Body, Patch, Error)) {
    return {400, MakeAutomationError(EAutomationErrorCode::InvalidArgument, Error)};
  }

  try {
    if (!PatchActorTransformProvider) {
      return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
    }

    FAutomationCommandTicket Ticket = CommandQueue.Enqueue(
        [Provider = PatchActorTransformProvider, ActorId, Patch = std::move(Patch)]() {
          FAutomationActorSnapshot Snapshot;
          const EAutomationWorldMutationStatus Status = Provider(ActorId, Patch, Snapshot);
          if (Status != EAutomationWorldMutationStatus::Success) {
            return MakeWorldMutationError(Status);
          }
          return MakeAutomationSuccess(SerializeActor(Snapshot));
        }
    );
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

bool FAutomationApiController::TryParseSpawnRequest(
    const nlohmann::json& Body, FAutomationSpawnActorRequest& OutRequest, std::string& OutError
) {
  if (!HasOnlyAllowedFields(Body, {"className", "transform", "instanceName"})) {
    OutError = "The spawn request contains an unknown field or is not an object.";
    return false;
  }
  if (!Body.contains("className") || !Body["className"].is_string()) {
    OutError = "className must be a string.";
    return false;
  }

  FAutomationSpawnActorRequest Request;
  Request.ClassName = Body["className"].get<std::string>();
  if (Request.ClassName.empty() || Request.ClassName.size() > 128) {
    OutError = "className must contain between 1 and 128 UTF-8 bytes.";
    return false;
  }

  if (Body.contains("instanceName")) {
    if (!Body["instanceName"].is_string()) {
      OutError = "instanceName must be a string.";
      return false;
    }
    std::string InstanceName = Body["instanceName"].get<std::string>();
    if (InstanceName.empty() || InstanceName.size() > 128) {
      OutError = "instanceName must contain between 1 and 128 UTF-8 bytes.";
      return false;
    }
    Request.InstanceName = std::move(InstanceName);
  }

  if (Body.contains("transform")) {
    const nlohmann::json& Transform = Body["transform"];
    if (!HasOnlyAllowedFields(Transform, {"location", "rotation", "scale"})) {
      OutError = "transform contains an unknown field or is not an object.";
      return false;
    }

    if (Transform.contains("location")) {
      const nlohmann::json& Location = Transform["location"];
      if (!HasOnlyAllowedFields(Location, {"x", "y"}) || !Location.contains("x") ||
          !Location.contains("y")) {
        OutError = "location must be an object containing only x and y.";
        return false;
      }
      if (!TryReadFiniteFloat(Location, "x", Request.Location.X, OutError) ||
          !TryReadFiniteFloat(Location, "y", Request.Location.Y, OutError)) {
        return false;
      }
    }

    if (Transform.contains("rotation") &&
        !TryReadFiniteFloat(Transform, "rotation", Request.Rotation.Rotation, OutError)) {
      return false;
    }
    if (Transform.contains("scale")) {
      if (!TryReadFiniteFloat(Transform, "scale", Request.Scale.Scale, OutError)) {
        return false;
      }
      if (Request.Scale.Scale <= 0.0f) {
        OutError = "scale must be greater than zero.";
        return false;
      }
    }
  }

  OutRequest = std::move(Request);
  return true;
}

bool FAutomationApiController::TryParseTransformPatch(
    const nlohmann::json& Body, FAutomationTransformPatch& OutPatch, std::string& OutError
) {
  if (!HasOnlyAllowedFields(Body, {"location", "rotation", "scale"})) {
    OutError = "The transform patch contains an unknown field or is not an object.";
    return false;
  }

  FAutomationTransformPatch Patch;
  if (Body.contains("location")) {
    const nlohmann::json& Location = Body["location"];
    if (!HasOnlyAllowedFields(Location, {"x", "y"}) || !Location.contains("x") ||
        !Location.contains("y")) {
      OutError = "location must be an object containing only x and y.";
      return false;
    }
    FVector2D Value;
    if (!TryReadFiniteFloat(Location, "x", Value.X, OutError) ||
        !TryReadFiniteFloat(Location, "y", Value.Y, OutError)) {
      return false;
    }
    Patch.Location = Value;
  }

  if (Body.contains("rotation")) {
    FRotator Value;
    if (!TryReadFiniteFloat(Body, "rotation", Value.Rotation, OutError)) {
      return false;
    }
    Patch.Rotation = Value;
  }
  if (Body.contains("scale")) {
    FScale Value;
    if (!TryReadFiniteFloat(Body, "scale", Value.Scale, OutError)) {
      return false;
    }
    if (Value.Scale <= 0.0f) {
      OutError = "scale must be greater than zero.";
      return false;
    }
    Patch.Scale = Value;
  }

  if (!Patch.HasAnyValue()) {
    OutError = "At least one transform value must be provided.";
    return false;
  }

  OutPatch = std::move(Patch);
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
  if (ErrorCode == "CLASS_NOT_REGISTERED") {
    return 404;
  }
  if (ErrorCode == "ACTOR_PENDING_DESTROY" || ErrorCode == "CONFLICT") {
    return 409;
  }
  if (ErrorCode == "REQUEST_TIMEOUT") {
    return 504;
  }
  return 500;
}
