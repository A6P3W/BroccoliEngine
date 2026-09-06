#include "InvocationController.h"
#include "../Detail/HttpControllerUtilities.h"

FAutomationInvocationController::FAutomationInvocationController(
    FAutomationHttpRequestExecutor& InExecutor,
    FAutomationCommandQueue& InCommandQueue,
    FAutomationActorMethodRegistry& InMethodRegistry,
    FAutomationActorResolver InActorResolver,
    FAutomationComponentMethodRegistry& InComponentMethodRegistry,
    FAutomationComponentResolver InComponentResolver
)
    : FAutomationHttpControllerBase(InExecutor),
      CommandQueue(InCommandQueue),
      MethodRegistry(&InMethodRegistry),
      ActorResolver(std::move(InActorResolver)),
      ComponentMethodRegistry(&InComponentMethodRegistry),
      ComponentResolver(std::move(InComponentResolver)) {}

FAutomationHttpResponse FAutomationInvocationController::GetWorldActorMethods(
    std::string_view ActorIdText
) {
  FActorId ActorId = InvalidActorId;
  if (!TryParseActorId(ActorIdText, ActorId)) {
    return {400, MakeAutomationError(EAutomationErrorCode::InvalidArgument, InvalidActorIdMessage)};
  }

  try {
    if (!MethodRegistry || !ActorResolver) {
      return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
    }

    FAutomationCommandTicket Ticket =
        CommandQueue.Enqueue([Registry = MethodRegistry, Resolver = ActorResolver, ActorId]() {
          AActor* Actor = nullptr;
          const EAutomationActorResolveStatus ResolveStatus = Resolver(ActorId, Actor);
          if (ResolveStatus != EAutomationActorResolveStatus::Success) {
            return MakeActorResolveError(ResolveStatus);
          }
          if (!Actor) {
            return MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage);
          }

          const std::string ClassName = Actor->GetActorClassName();
          nlohmann::json Methods = nlohmann::json::array();
          for (const FAutomationMethodSnapshot& Snapshot :
               Registry->GetMethodsForClass(ClassName)) {
            if (!IsActorMethodPermissionAllowed(Snapshot.Permission)) {
              continue;
            }
            Methods.push_back(
                {{"name", Snapshot.Name},
                 {"description", Snapshot.Description},
                 {"inputSchema", Snapshot.InputSchema},
                 {"permission", ToAutomationPermissionString(Snapshot.Permission)}}
            );
          }
          return MakeAutomationSuccess(
              {{"actorId", ActorId}, {"className", ClassName}, {"methods", std::move(Methods)}}
          );
        });
    return WaitForResult(std::move(Ticket));
  } catch (...) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
  }
}

FAutomationHttpResponse FAutomationInvocationController::InvokeWorldActorMethod(
    std::string_view ActorIdText, std::string_view MethodName, const nlohmann::json& Body
) {
  FActorId ActorId = InvalidActorId;
  if (!TryParseActorId(ActorIdText, ActorId)) {
    return {400, MakeAutomationError(EAutomationErrorCode::InvalidArgument, InvalidActorIdMessage)};
  }
  if (!IsValidAutomationOperationName(MethodName)) {
    return {
        400, MakeAutomationError(EAutomationErrorCode::InvalidArgument, InvalidMethodNameMessage)
    };
  }
  if (!Body.is_object()) {
    return {
        400,
        MakeAutomationError(
            EAutomationErrorCode::InvalidArgument, "The actor method arguments must be an object."
        )
    };
  }

  try {
    if (!MethodRegistry || !ActorResolver) {
      return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
    }

    FAutomationCommandTicket Ticket = CommandQueue.Enqueue([Registry = MethodRegistry,
                                                            Resolver = ActorResolver,
                                                            ActorId,
                                                            MethodNameText =
                                                                std::string(MethodName),
                                                            Arguments = Body]() {
      AActor* Actor = nullptr;
      const EAutomationActorResolveStatus ResolveStatus = Resolver(ActorId, Actor);
      if (ResolveStatus != EAutomationActorResolveStatus::Success) {
        return MakeActorResolveError(ResolveStatus);
      }
      if (!Actor) {
        return MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage);
      }

      const std::string ClassName = Actor->GetActorClassName();
      const FAutomationMethodDescriptor* Descriptor =
          Registry->FindMethod(ClassName, MethodNameText);
      if (!Descriptor) {
        M_LOG(
            Log,
            "Automation actor method rejected: actorId={} class={} "
            "method={} code=METHOD_NOT_REGISTERED",
            ActorId,
            ClassName,
            MethodNameText
        );
        return MakeAutomationError(
            EAutomationErrorCode::MethodNotRegistered, MethodNotRegisteredMessage
        );
      }
      if (!IsActorMethodPermissionAllowed(Descriptor->Permission)) {
        M_LOG(
            Log,
            "Automation actor method rejected: actorId={} class={} "
            "method={} code=PERMISSION_DENIED",
            ActorId,
            ClassName,
            MethodNameText
        );
        return MakeAutomationError(EAutomationErrorCode::PermissionDenied, PermissionDeniedMessage);
      }

      FAutomationSchemaValidationError ValidationError;
      if (!FAutomationJsonSchemaValidator::ValidateValue(
              Descriptor->InputSchema, Arguments, ValidationError
          )) {
        M_LOG(
            Log,
            "Automation actor method rejected: actorId={} class={} "
            "method={} code=INVALID_ARGUMENT",
            ActorId,
            ClassName,
            MethodNameText
        );
        return MakeAutomationError(
            EAutomationErrorCode::InvalidArgument,
            ValidationError.JsonPath + ": " + ValidationError.Message
        );
      }

      M_LOG(
          Log,
          "Automation actor method starting: actorId={} class={} method={}",
          ActorId,
          ClassName,
          MethodNameText
      );
      nlohmann::json Result = Descriptor->Handler(*Actor, Arguments);
      M_LOG(
          Log,
          "Automation actor method completed: actorId={} class={} method={}",
          ActorId,
          ClassName,
          MethodNameText
      );
      return MakeAutomationSuccess(
          {{"actorId", ActorId},
           {"className", ClassName},
           {"methodName", MethodNameText},
           {"result", std::move(Result)}}
      );
    });
    return WaitForResult(std::move(Ticket));
  } catch (...) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
  }
}

FAutomationHttpResponse FAutomationInvocationController::GetWorldActorComponentMethods(
    std::string_view ActorIdText, std::string_view ComponentIdText
) {
  FActorId ActorId = InvalidActorId;
  FComponentId ComponentId = InvalidComponentId;
  if (!TryParseActorId(ActorIdText, ActorId) ||
      !TryParseComponentId(ComponentIdText, ComponentId)) {
    return {
        400,
        MakeAutomationError(
            EAutomationErrorCode::InvalidArgument,
            "The actorId and componentId must be unsigned decimal integers greater than zero."
        )
    };
  }
  if (!ComponentMethodRegistry || !ComponentResolver) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
  }
  try {
    FAutomationCommandTicket Ticket = CommandQueue.Enqueue(
        [Registry = ComponentMethodRegistry, Resolver = ComponentResolver, ActorId, ComponentId]() {
          MActorComponent* Component = nullptr;
          const EAutomationComponentResolveStatus Status =
              Resolver(ActorId, ComponentId, Component);
          if (Status != EAutomationComponentResolveStatus::Success) {
            return MakeAutomationError(
                Status == EAutomationComponentResolveStatus::WorldNotAvailable
                    ? EAutomationErrorCode::WorldNotAvailable
                : Status == EAutomationComponentResolveStatus::ActorPendingDestroy
                    ? EAutomationErrorCode::ActorPendingDestroy
                    : EAutomationErrorCode::ActorNotFound,
                Status == EAutomationComponentResolveStatus::ComponentNotFound
                    ? "The requested component was not found."
                    : ActorNotFoundMessage
            );
          }
          nlohmann::json Methods = nlohmann::json::array();
          for (const FAutomationComponentMethodSnapshot& Snapshot :
               Registry->GetMethodsForClass(Component->GetComponentClassName())) {
            Methods.push_back(
                {{"name", Snapshot.Name},
                 {"description", Snapshot.Description},
                 {"inputSchema", Snapshot.InputSchema},
                 {"permission", ToAutomationPermissionString(Snapshot.Permission)}}
            );
          }
          return MakeAutomationSuccess(
              {{"actorId", ActorId},
               {"componentId", ComponentId},
               {"className", Component->GetComponentClassName()},
               {"methods", std::move(Methods)}}
          );
        }
    );
    return WaitForResult(std::move(Ticket));
  } catch (...) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
  }
}

FAutomationHttpResponse FAutomationInvocationController::InvokeWorldActorComponentMethod(
    std::string_view ActorIdText,
    std::string_view ComponentIdText,
    std::string_view MethodName,
    const nlohmann::json& Body
) {
  FActorId ActorId = InvalidActorId;
  FComponentId ComponentId = InvalidComponentId;
  if (!TryParseActorId(ActorIdText, ActorId) ||
      !TryParseComponentId(ComponentIdText, ComponentId) ||
      !IsValidAutomationOperationName(MethodName)) {
    return {
        400,
        MakeAutomationError(
            EAutomationErrorCode::InvalidArgument,
            "The actorId, componentId, or methodName is invalid."
        )
    };
  }
  if (!Body.is_object() || !HasOnlyAllowedFields(Body, {"arguments"}) ||
      !Body.contains("arguments") || !Body["arguments"].is_object()) {
    return {
        400,
        MakeAutomationError(
            EAutomationErrorCode::InvalidArgument,
            "The component method request must contain an arguments object."
        )
    };
  }
  if (!ComponentMethodRegistry || !ComponentResolver) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
  }
  try {
    FAutomationCommandTicket Ticket = CommandQueue.Enqueue([Registry = ComponentMethodRegistry,
                                                            Resolver = ComponentResolver,
                                                            ActorId,
                                                            ComponentId,
                                                            MethodNameText =
                                                                std::string(MethodName),
                                                            Arguments = Body["arguments"]]() {
      MActorComponent* Component = nullptr;
      if (Resolver(ActorId, ComponentId, Component) != EAutomationComponentResolveStatus::Success) {
        return MakeAutomationError(
            EAutomationErrorCode::ActorNotFound, "The requested actor or component was not found."
        );
      }
      const std::string ClassName = Component->GetComponentClassName();
      const FAutomationComponentMethodDescriptor* Descriptor =
          Registry->FindMethod(ClassName, MethodNameText);
      if (!Descriptor) {
        return MakeAutomationError(
            EAutomationErrorCode::MethodNotRegistered,
            "The requested method is not registered for this component class."
        );
      }
      FAutomationSchemaValidationError ValidationError;
      if (!FAutomationJsonSchemaValidator::ValidateValue(
              Descriptor->InputSchema, Arguments, ValidationError
          )) {
        return MakeAutomationError(
            EAutomationErrorCode::InvalidArgument,
            ValidationError.JsonPath + ": " + ValidationError.Message
        );
      }
      return MakeAutomationSuccess(
          {{"actorId", ActorId},
           {"componentId", ComponentId},
           {"className", ClassName},
           {"methodName", MethodNameText},
           {"result", Descriptor->Handler(*Component, Arguments)}}
      );
    });
    return WaitForResult(std::move(Ticket));
  } catch (...) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
  }
}

