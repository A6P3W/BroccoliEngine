#pragma once

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <initializer_list>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include "Actor.h"
#include "ActorComponent.h"
#include "Log.h"
#include "Registry/ComponentMethodRegistry.h"
#include "Registry/Schema/SchemaValidator.h"

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
constexpr std::string_view MethodNotRegisteredMessage =
    "The requested method is not registered for this actor class.";
constexpr std::string_view PermissionDeniedMessage =
    "The requested method permission is not allowed.";
constexpr std::string_view InvalidMethodNameMessage =
    "The methodName must match ^[a-z][a-z0-9_]{0,127}$.";
constexpr std::string_view CommandNotRegisteredMessage =
    "The requested system command is not registered.";
constexpr std::string_view CommandPermissionDeniedMessage =
    "The requested command permission is not allowed.";
constexpr std::string_view InvalidCommandNameMessage =
    "The commandName must match ^[a-z][a-z0-9_]{0,127}$.";

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

nlohmann::json MakeActorResolveError(EAutomationActorResolveStatus Status) {
  switch (Status) {
    case EAutomationActorResolveStatus::WorldNotAvailable:
      return MakeAutomationError(EAutomationErrorCode::WorldNotAvailable, WorldNotAvailableMessage);
    case EAutomationActorResolveStatus::ActorNotFound:
      return MakeAutomationError(EAutomationErrorCode::ActorNotFound, ActorNotFoundMessage);
    case EAutomationActorResolveStatus::ActorPendingDestroy:
      return MakeAutomationError(
          EAutomationErrorCode::ActorPendingDestroy, ActorPendingDestroyMessage
      );
    case EAutomationActorResolveStatus::InvalidState:
    case EAutomationActorResolveStatus::Success:
      return MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage);
  }
  return MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage);
}

bool IsActorMethodPermissionAllowed(EAutomationPermission Permission) {
  return Permission == EAutomationPermission::ReadOnly ||
         Permission == EAutomationPermission::WorldMutation;
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

bool TryParseUnsigned(std::string_view Text, uint64_t& OutValue) {
  OutValue = 0;
  if (Text.empty()) {
    return false;
  }
  const char* Begin = Text.data();
  const char* End = Begin + Text.size();
  const auto [Position, Error] = std::from_chars(Begin, End, OutValue, 10);
  return Error == std::errc() && Position == End;
}

std::optional<ELogLevel> ParseLogLevel(std::string_view Text) {
  std::string Lowercase(Text);
  std::transform(
      Lowercase.begin(), Lowercase.end(), Lowercase.begin(), [](unsigned char Character) {
        return static_cast<char>(std::tolower(Character));
      }
  );
  if (Lowercase == "debug") {
    return ELogLevel::Debug;
  }
  if (Lowercase == "log" || Lowercase == "info") {
    return ELogLevel::Log;
  }
  if (Lowercase == "warning") {
    return ELogLevel::Warning;
  }
  if (Lowercase == "error") {
    return ELogLevel::Error;
  }
  return std::nullopt;
}

bool TryParseActorId(std::string_view Text, FActorId& OutActorId);
bool TryParseComponentId(std::string_view Text, FComponentId& OutComponentId);
bool TryParseSpawnRequest(
    const nlohmann::json& Body, FAutomationSpawnActorRequest& OutRequest, std::string& OutError
);
bool TryParseTransformPatch(
    const nlohmann::json& Body, FAutomationTransformPatch& OutPatch, std::string& OutError
);
nlohmann::json SerializeActor(const FAutomationActorSnapshot& Actor);
nlohmann::json SerializeActorList(const FAutomationActorListSnapshot& Snapshot);


bool TryParseActorId(std::string_view Text, FActorId& OutActorId) {
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

bool TryParseComponentId(std::string_view Text, FComponentId& OutComponentId) {
  OutComponentId = InvalidComponentId;
  uint64_t ParsedComponentId = 0;
  if (!TryParseUnsigned(Text, ParsedComponentId) || ParsedComponentId == 0) {
    return false;
  }
  OutComponentId = ParsedComponentId;
  return true;
}

bool TryParseSpawnRequest(
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

bool TryParseTransformPatch(
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

nlohmann::json SerializeActor(const FAutomationActorSnapshot& Actor) {
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

nlohmann::json SerializeActorList(const FAutomationActorListSnapshot& Snapshot) {
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


}  // namespace