#pragma once

#include <functional>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "ActorId.h"
#include "AutomationCommandQueue.h"
#include "AutomationMethodRegistry.h"
#include "AutomationTypes.h"
#include "BroccoliEngineAPI.h"
#include "UMath.h"

struct FAutomationHttpResponse {
  int StatusCode = 500;
  nlohmann::json Body =
      MakeAutomationError(EAutomationErrorCode::InternalError, "The automation request failed.");
};

using FAutomationStateProvider = std::function<nlohmann::json()>;

struct FAutomationActorSnapshot {
  FActorId ActorId = InvalidActorId;
  std::string InstanceName;
  std::string ClassName;
  FVector2D Location;
  FRotator Rotation;
  FScale Scale;
};

struct FAutomationActorListSnapshot {
  std::string SceneName;
  std::vector<FAutomationActorSnapshot> Actors;
};

enum class EAutomationWorldReadStatus : uint8_t {
  Success,
  WorldNotAvailable,
  ActorNotFound,
  InvalidState
};

using FAutomationActorListProvider =
    std::function<EAutomationWorldReadStatus(FAutomationActorListSnapshot&)>;
using FAutomationActorProvider =
    std::function<EAutomationWorldReadStatus(FActorId, FAutomationActorSnapshot&)>;

enum class EAutomationActorResolveStatus : uint8_t {
  Success,
  WorldNotAvailable,
  ActorNotFound,
  ActorPendingDestroy,
  InvalidState
};

class AActor;
using FAutomationActorResolver = std::function<EAutomationActorResolveStatus(FActorId, AActor*&)>;

enum class EAutomationWorldMutationStatus : uint8_t {
  Success,
  WorldNotAvailable,
  ClassNotRegistered,
  ActorNotFound,
  ActorPendingDestroy,
  InvalidState
};

struct FAutomationSpawnActorRequest {
  std::string ClassName;
  FVector2D Location = FVector2D::ZeroVector();
  FRotator Rotation = FRotator(0.0f);
  FScale Scale = FScale(1.0f);
  std::optional<std::string> InstanceName;
};

struct FAutomationTransformPatch {
  std::optional<FVector2D> Location;
  std::optional<FRotator> Rotation;
  std::optional<FScale> Scale;

  bool HasAnyValue() const;
};

struct FAutomationLogQueryText {
  std::optional<std::string> Limit;
  std::optional<std::string> Level;
  std::optional<std::string> AfterSequence;
  bool bHasUnknownParameter = false;
  bool bHasDuplicateParameter = false;
};

using FAutomationSpawnActorProvider = std::function<
    EAutomationWorldMutationStatus(const FAutomationSpawnActorRequest&, FAutomationActorSnapshot&)>;
using FAutomationDestroyActorProvider = std::function<EAutomationWorldMutationStatus(FActorId)>;
using FAutomationPatchActorTransformProvider = std::function<EAutomationWorldMutationStatus(
    FActorId, const FAutomationTransformPatch&, FAutomationActorSnapshot&
)>;

class BROCCOLI_ENGINE_API FAutomationApiController {
 public:
  FAutomationApiController(
      FAutomationCommandQueue& InCommandQueue,
      const FAutomationConfig& InConfig,
      FAutomationStateProvider InStateProvider,
      FAutomationActorListProvider InActorListProvider = {},
      FAutomationActorProvider InActorProvider = {},
      FAutomationSpawnActorProvider InSpawnActorProvider = {},
      FAutomationDestroyActorProvider InDestroyActorProvider = {},
      FAutomationPatchActorTransformProvider InPatchActorTransformProvider = {},
      FAutomationMethodRegistry* InMethodRegistry = nullptr,
      FAutomationActorResolver InActorResolver = {}
  );

  FAutomationHttpResponse GetState();
  FAutomationHttpResponse GetWorldActors();
  FAutomationHttpResponse GetWorldActor(std::string_view ActorIdText);
  FAutomationHttpResponse CreateWorldActor(const nlohmann::json& Body);
  FAutomationHttpResponse DeleteWorldActor(std::string_view ActorIdText);
  FAutomationHttpResponse PatchWorldActorTransform(
      std::string_view ActorIdText, const nlohmann::json& Body
  );
  FAutomationHttpResponse GetWorldActorMethods(std::string_view ActorIdText);
  FAutomationHttpResponse InvokeWorldActorMethod(
      std::string_view ActorIdText, std::string_view MethodName, const nlohmann::json& Body
  );
  FAutomationHttpResponse GetRecentLogs(const FAutomationLogQueryText& Query);

 private:
  static bool TryParseActorId(std::string_view Text, FActorId& OutActorId);
  static bool TryParseSpawnRequest(
      const nlohmann::json& Body, FAutomationSpawnActorRequest& OutRequest, std::string& OutError
  );
  static bool TryParseTransformPatch(
      const nlohmann::json& Body, FAutomationTransformPatch& OutPatch, std::string& OutError
  );
  static nlohmann::json SerializeActor(const FAutomationActorSnapshot& Actor);
  static nlohmann::json SerializeActorList(const FAutomationActorListSnapshot& Snapshot);
  FAutomationHttpResponse WaitForResult(FAutomationCommandTicket&& Ticket);
  static int GetHttpStatusCode(const nlohmann::json& Body);

  FAutomationCommandQueue& CommandQueue;
  FAutomationConfig Config;
  FAutomationStateProvider StateProvider;
  FAutomationActorListProvider ActorListProvider;
  FAutomationActorProvider ActorProvider;
  FAutomationSpawnActorProvider SpawnActorProvider;
  FAutomationDestroyActorProvider DestroyActorProvider;
  FAutomationPatchActorTransformProvider PatchActorTransformProvider;
  FAutomationMethodRegistry* MethodRegistry = nullptr;
  FAutomationActorResolver ActorResolver;
};
