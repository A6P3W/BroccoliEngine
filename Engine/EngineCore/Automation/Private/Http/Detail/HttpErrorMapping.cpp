#include "HttpErrorMapping.h"
namespace AutomationHttpDetail {
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

nlohmann::json MakeComponentResolveError(EAutomationComponentResolveStatus Status) {
  switch (Status) {
    case EAutomationComponentResolveStatus::WorldNotAvailable:
      return MakeAutomationError(EAutomationErrorCode::WorldNotAvailable, WorldNotAvailableMessage);
    case EAutomationComponentResolveStatus::ActorNotFound:
      return MakeAutomationError(EAutomationErrorCode::ActorNotFound, ActorNotFoundMessage);
    case EAutomationComponentResolveStatus::ActorPendingDestroy:
      return MakeAutomationError(
          EAutomationErrorCode::ActorPendingDestroy, ActorPendingDestroyMessage
      );
    case EAutomationComponentResolveStatus::ComponentNotFound:
      return MakeAutomationError(
          EAutomationErrorCode::ComponentNotFound, "The requested component was not found."
      );
    case EAutomationComponentResolveStatus::ComponentPendingDestroy:
      return MakeAutomationError(
          EAutomationErrorCode::ComponentPendingDestroy,
          "The requested component is pending destruction."
      );
    case EAutomationComponentResolveStatus::InvalidState:
    case EAutomationComponentResolveStatus::Success:
      return MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage);
  }
  return MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage);
}


}
