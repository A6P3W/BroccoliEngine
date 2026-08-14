#include "AutomationApiController.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <chrono>
#include <cmath>
#include <exception>
#include <filesystem>
#include <initializer_list>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

#include "Actor.h"
#include "ActorComponent.h"
#include "AutomationComponentMethodRegistry.h"
#include "AutomationJsonSchemaValidator.h"
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
      Lowercase.begin(),
      Lowercase.end(),
      Lowercase.begin(),
      [](unsigned char Character) { return static_cast<char>(std::tolower(Character)); }
  );
  if (Lowercase == "debug") {
    return ELogLevel::Debug;
  }
  if (Lowercase == "info") {
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
    FAutomationActorComponentListProvider InActorComponentListProvider,
    FAutomationSpawnActorProvider InSpawnActorProvider,
    FAutomationDestroyActorProvider InDestroyActorProvider,
    FAutomationPatchActorTransformProvider InPatchActorTransformProvider,
    FAutomationMethodRegistry* InMethodRegistry,
    FAutomationActorResolver InActorResolver,
    FAutomationComponentMethodRegistry* InComponentMethodRegistry,
    FAutomationComponentResolver InComponentResolver,
    FAutomationSystemCommandRegistry* InSystemCommandRegistry,
    FAutomationActorClassListProvider InActorClassListProvider,
    FAutomationLevelListProvider InLevelListProvider,
    FAutomationActorClassExistsProvider InActorClassExistsProvider
)
    : CommandQueue(InCommandQueue),
      Config(InConfig),
      StateProvider(std::move(InStateProvider)),
      ActorListProvider(std::move(InActorListProvider)),
      ActorProvider(std::move(InActorProvider)),
      ActorComponentListProvider(std::move(InActorComponentListProvider)),
      SpawnActorProvider(std::move(InSpawnActorProvider)),
      DestroyActorProvider(std::move(InDestroyActorProvider)),
      PatchActorTransformProvider(std::move(InPatchActorTransformProvider)),
      MethodRegistry(InMethodRegistry),
      ActorResolver(std::move(InActorResolver)),
      ComponentMethodRegistry(InComponentMethodRegistry),
      ComponentResolver(std::move(InComponentResolver)),
      SystemCommandRegistry(InSystemCommandRegistry),
      ActorClassListProvider(std::move(InActorClassListProvider)),
      LevelListProvider(std::move(InLevelListProvider)),
      ActorClassExistsProvider(std::move(InActorClassExistsProvider)) {}

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

FAutomationHttpResponse FAutomationApiController::GetActorClasses() {
  try {
    if (!ActorClassListProvider) {
      return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
    }
    FAutomationCommandTicket Ticket = CommandQueue.Enqueue([Provider = ActorClassListProvider]() {
      nlohmann::json Classes = nlohmann::json::array();
      for (const FAutomationActorClassInfo& Info : Provider()) {
        Classes.push_back({{"className", Info.ClassName}, {"isGameMode", Info.bIsGameMode}});
      }
      return MakeAutomationSuccess({{"classes", std::move(Classes)}});
    });
    return WaitForResult(std::move(Ticket));
  } catch (...) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
  }
}

FAutomationHttpResponse FAutomationApiController::GetLevels() {
  try {
    if (!LevelListProvider) {
      return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
    }
    FAutomationCommandTicket Ticket = CommandQueue.Enqueue([Provider = LevelListProvider]() {
      nlohmann::json Levels = nlohmann::json::array();
      for (const FAutomationLevelInfo& Info : Provider()) {
        Levels.push_back(
            {{"sceneId", Info.SceneId},
             {"levelName", std::filesystem::path(Info.LevelPath).stem().string()},
             {"levelPath", Info.LevelPath}}
        );
      }
      return MakeAutomationSuccess({{"levels", std::move(Levels)}});
    });
    return WaitForResult(std::move(Ticket));
  } catch (...) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
  }
}

FAutomationHttpResponse FAutomationApiController::GetActorClassMethods(std::string_view ClassName) {
  if (ClassName.empty() || ClassName.size() > 128) {
    return {
        400, MakeAutomationError(EAutomationErrorCode::InvalidArgument, "The className is invalid.")
    };
  }
  try {
    if (!MethodRegistry || !ActorClassExistsProvider) {
      return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
    }
    FAutomationCommandTicket Ticket =
        CommandQueue.Enqueue([Registry = MethodRegistry,
                              ExistsProvider = ActorClassExistsProvider,
                              ClassNameText = std::string(ClassName)]() {
          if (!ExistsProvider(ClassNameText)) {
            return MakeAutomationError(
                EAutomationErrorCode::ClassNotRegistered, ClassNotRegisteredMessage
            );
          }
          nlohmann::json Methods = nlohmann::json::array();
          for (const FAutomationMethodSnapshot& Snapshot :
               Registry->GetMethodsForClass(ClassNameText)) {
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
              {{"className", ClassNameText}, {"methods", std::move(Methods)}}
          );
        });
    return WaitForResult(std::move(Ticket));
  } catch (...) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
  }
}

FAutomationHttpResponse FAutomationApiController::GetWorldActors(
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

FAutomationHttpResponse FAutomationApiController::GetWorldActorComponents(
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

FAutomationHttpResponse FAutomationApiController::GetWorldActorMethods(std::string_view ActorIdText
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

FAutomationHttpResponse FAutomationApiController::InvokeWorldActorMethod(
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

FAutomationHttpResponse FAutomationApiController::GetWorldActorComponentMethods(
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

FAutomationHttpResponse FAutomationApiController::InvokeWorldActorComponentMethod(
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

FAutomationHttpResponse FAutomationApiController::GetSystemCommands() {
  try {
    if (!SystemCommandRegistry) {
      return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
    }

    FAutomationCommandTicket Ticket = CommandQueue.Enqueue([Registry = SystemCommandRegistry]() {
      nlohmann::json Commands = nlohmann::json::array();
      for (const FAutomationSystemCommandSnapshot& Snapshot : Registry->GetCommands()) {
        if (Snapshot.Permission != EAutomationPermission::SystemMutation) {
          continue;
        }
        Commands.push_back(
            {{"name", Snapshot.Name},
             {"description", Snapshot.Description},
             {"inputSchema", Snapshot.InputSchema},
             {"permission", ToAutomationPermissionString(Snapshot.Permission)}}
        );
      }
      return MakeAutomationSuccess({{"commands", std::move(Commands)}});
    });
    return WaitForResult(std::move(Ticket));
  } catch (...) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
  }
}

FAutomationHttpResponse FAutomationApiController::ExecuteSystemCommand(
    std::string_view CommandName, const nlohmann::json& Body
) {
  if (!IsValidAutomationOperationName(CommandName)) {
    return {
        400, MakeAutomationError(EAutomationErrorCode::InvalidArgument, InvalidCommandNameMessage)
    };
  }
  if (!Body.is_object()) {
    return {
        400,
        MakeAutomationError(
            EAutomationErrorCode::InvalidArgument, "The system command arguments must be an object."
        )
    };
  }

  try {
    if (!SystemCommandRegistry) {
      return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
    }

    FAutomationCommandTicket Ticket = CommandQueue.Enqueue([Registry = SystemCommandRegistry,
                                                            CommandNameText =
                                                                std::string(CommandName),
                                                            Arguments = Body]() {
      const FAutomationSystemCommandDescriptor* Descriptor = Registry->FindCommand(CommandNameText);
      if (!Descriptor) {
        M_LOG(
            Log,
            "Automation system command rejected: command={} "
            "code=COMMAND_NOT_REGISTERED",
            CommandNameText
        );
        return MakeAutomationError(
            EAutomationErrorCode::CommandNotRegistered, CommandNotRegisteredMessage
        );
      }
      if (Descriptor->Permission != EAutomationPermission::SystemMutation) {
        M_LOG(
            Log,
            "Automation system command rejected: command={} "
            "code=PERMISSION_DENIED",
            CommandNameText
        );
        return MakeAutomationError(
            EAutomationErrorCode::PermissionDenied, CommandPermissionDeniedMessage
        );
      }

      FAutomationSchemaValidationError ValidationError;
      if (!FAutomationJsonSchemaValidator::ValidateValue(
              Descriptor->InputSchema, Arguments, ValidationError
          )) {
        M_LOG(
            Log,
            "Automation system command rejected: command={} "
            "code=INVALID_ARGUMENT",
            CommandNameText
        );
        return MakeAutomationError(
            EAutomationErrorCode::InvalidArgument,
            ValidationError.JsonPath + ": " + ValidationError.Message
        );
      }

      nlohmann::json Result = Descriptor->Handler(Arguments);
      M_LOG(Log, "Automation system command completed: command={}", CommandNameText);
      return MakeAutomationSuccess({{"commandName", CommandNameText}, {"result", std::move(Result)}}
      );
    });
    return WaitForResult(std::move(Ticket));
  } catch (...) {
    return {500, MakeAutomationError(EAutomationErrorCode::InternalError, InternalErrorMessage)};
  }
}

FAutomationHttpResponse FAutomationApiController::GetRecentLogs(const FAutomationLogQueryText& Query
) {
  if (Query.bHasUnknownParameter) {
    return {
        400,
        MakeAutomationError(
            EAutomationErrorCode::InvalidRequest, "The log query contains an unknown parameter."
        )
    };
  }
  if (Query.bHasDuplicateParameter) {
    return {
        400,
        MakeAutomationError(
            EAutomationErrorCode::InvalidArgument,
            "Each log query parameter may be specified only once."
        )
    };
  }

  FLogQuery LogQuery;
  if (Query.Limit) {
    uint64_t Limit = 0;
    if (!TryParseUnsigned(*Query.Limit, Limit) || Limit < 1 || Limit > 1000) {
      return {
          400,
          MakeAutomationError(
              EAutomationErrorCode::InvalidArgument,
              "limit must be an unsigned integer between 1 and 1000."
          )
      };
    }
    LogQuery.Limit = static_cast<size_t>(Limit);
  }
  if (Query.Level) {
    LogQuery.MinimumLevel = ParseLogLevel(*Query.Level);
    if (!LogQuery.MinimumLevel) {
      return {
          400,
          MakeAutomationError(
              EAutomationErrorCode::InvalidArgument, "level must be debug, info, warning, or error."
          )
      };
    }
  }
  if (Query.AfterSequence) {
    uint64_t AfterSequence = 0;
    if (!TryParseUnsigned(*Query.AfterSequence, AfterSequence)) {
      return {
          400,
          MakeAutomationError(
              EAutomationErrorCode::InvalidArgument,
              "afterSequence must be an unsigned 64-bit integer."
          )
      };
    }
    LogQuery.AfterSequence = AfterSequence;
  }

  try {
    const FLogQueryResult Result = MLog::GetRecentEntries(LogQuery);
    nlohmann::json Entries = nlohmann::json::array();
    for (const FLogEntry& Entry : Result.Entries) {
      Entries.push_back(
          {{"sequence", Entry.Sequence},
           {"timestamp", MLog::FormatTimestamp(Entry.Timestamp)},
           {"level", std::string(MLog::ToLevelString(Entry.Level))},
           {"category", Entry.Category},
           {"message", Entry.Message}}
      );
    }
    return {
        200,
        MakeAutomationSuccess(
            {{"entries", std::move(Entries)},
             {"count", Result.Entries.size()},
             {"oldestAvailableSequence", Result.OldestAvailableSequence},
             {"latestSequence", Result.LatestSequence},
             {"nextAfterSequence", Result.NextAfterSequence},
             {"historyLost", Result.bHistoryLost},
             {"hasMore", Result.bHasMore}}
        )
    };
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

bool FAutomationApiController::TryParseComponentId(
    std::string_view Text, FComponentId& OutComponentId
) {
  OutComponentId = InvalidComponentId;
  uint64_t ParsedComponentId = 0;
  if (!TryParseUnsigned(Text, ParsedComponentId) || ParsedComponentId == 0) {
    return false;
  }
  OutComponentId = ParsedComponentId;
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
       {{"location", {{"x", LocationX}, {"y", LocationY}}}, {"rotation", Rotation}, {"scale", Scale}
       }}
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
    M_LOG(Log, "Automation request timed out.");
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
  if (ErrorCode == "METHOD_NOT_REGISTERED" || ErrorCode == "COMMAND_NOT_REGISTERED") {
    return 404;
  }
  if (ErrorCode == "PERMISSION_DENIED") {
    return 403;
  }
  if (ErrorCode == "ACTOR_PENDING_DESTROY" || ErrorCode == "CONFLICT") {
    return 409;
  }
  if (ErrorCode == "REQUEST_TIMEOUT") {
    return 504;
  }
  return 500;
}
