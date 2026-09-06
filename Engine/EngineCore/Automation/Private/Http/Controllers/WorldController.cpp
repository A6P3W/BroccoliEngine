#include "WorldController.h"
#include "../Detail/HttpControllerUtilities.h"

FAutomationWorldController::FAutomationWorldController(
    FAutomationHttpRequestExecutor& InExecutor,
    FAutomationCommandQueue& InCommandQueue,
    FAutomationStateProvider InStateProvider,
    FAutomationActorListProvider InActorListProvider,
    FAutomationActorProvider InActorProvider,
    FAutomationActorComponentListProvider InActorComponentListProvider,
    FAutomationSpawnActorProvider InSpawnActorProvider,
    FAutomationDestroyActorProvider InDestroyActorProvider,
    FAutomationPatchActorTransformProvider InPatchActorTransformProvider
)
    : FAutomationHttpControllerBase(InExecutor),
      CommandQueue(InCommandQueue),
      StateProvider(std::move(InStateProvider)),
      ActorListProvider(std::move(InActorListProvider)),
      ActorProvider(std::move(InActorProvider)),
      ActorComponentListProvider(std::move(InActorComponentListProvider)),
      SpawnActorProvider(std::move(InSpawnActorProvider)),
      DestroyActorProvider(std::move(InDestroyActorProvider)),
      PatchActorTransformProvider(std::move(InPatchActorTransformProvider)) {}

FAutomationHttpResponse FAutomationWorldController::GetState() {
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

FAutomationHttpResponse FAutomationWorldController::GetWorldActors(
    const FAutomationActorQueryText& QueryText
) {
  if (QueryText.bHasUnknownParameter) {
    return {
        400,
        MakeAutomationError(
            EAutomationErrorCode::InvalidArgument, "The actor query contains an unknown parameter."
        )
    };
  }
  if (QueryText.bHasDuplicateParameter) {
    return {
        400,
        MakeAutomationError(
            EAutomationErrorCode::InvalidArgument,
            "Each actor query parameter may be specified only once."
        )
    };
  }
  FAutomationActorQuery Query{QueryText.ClassName, QueryText.InstanceName};
  try {
    if (!ActorListProvider) {
      return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
    }

    FAutomationCommandTicket Ticket = CommandQueue.Enqueue([Provider = ActorListProvider, Query]() {
      FAutomationActorListSnapshot Snapshot;
      const EAutomationWorldReadStatus Status = Provider(Query, Snapshot);
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

FAutomationHttpResponse FAutomationWorldController::GetWorldActor(std::string_view ActorIdText) {
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

FAutomationHttpResponse FAutomationWorldController::GetWorldActorComponents(
    std::string_view ActorIdText
) {
  FActorId ActorId = InvalidActorId;
  if (!TryParseActorId(ActorIdText, ActorId)) {
    return {400, MakeAutomationError(EAutomationErrorCode::InvalidArgument, InvalidActorIdMessage)};
  }
  try {
    if (!ActorComponentListProvider) {
      return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
    }
    FAutomationCommandTicket Ticket =
        CommandQueue.Enqueue([Provider = ActorComponentListProvider, ActorId]() {
          FAutomationActorComponentListSnapshot Snapshot;
          const EAutomationWorldReadStatus Status = Provider(ActorId, Snapshot);
          if (Status != EAutomationWorldReadStatus::Success) {
            return MakeWorldReadError(Status);
          }
          nlohmann::json Components = nlohmann::json::array();
          for (const FAutomationActorComponentSnapshot& Component : Snapshot.Components) {
            Components.push_back(
                {{"componentId", Component.ComponentId},
                 {"name", Component.Name},
                 {"className", Component.ClassName},
                 {"registered", Component.bRegistered},
                 {"pendingDestroy", Component.bPendingDestroy},
                 {"replicates", Component.bReplicates},
                 {"networkId", Component.NetworkId}}
            );
          }
          return MakeAutomationSuccess(
              {{"actorId", Snapshot.ActorId},
               {"className", Snapshot.ClassName},
               {"components", std::move(Components)}}
          );
        });
    return WaitForResult(std::move(Ticket));
  } catch (...) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
  }
}

FAutomationHttpResponse FAutomationWorldController::CreateWorldActor(const nlohmann::json& Body) {
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

FAutomationHttpResponse FAutomationWorldController::DeleteWorldActor(std::string_view ActorIdText) {
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

FAutomationHttpResponse FAutomationWorldController::PatchWorldActorTransform(
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

