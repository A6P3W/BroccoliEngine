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
#include "AutomationSystemCommandRegistry.h"
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

struct FAutomationActorComponentSnapshot {
  uint32_t Index = 0;
  std::string Name;
  std::string ClassName;
  bool bRegistered = false;
  bool bPendingDestroy = false;
  bool bReplicates = false;
  uint32_t NetworkId = 0;
};

struct FAutomationActorComponentListSnapshot {
  FActorId ActorId = InvalidActorId;
  std::string ClassName;
  std::vector<FAutomationActorComponentSnapshot> Components;
};

struct FAutomationActorListSnapshot {
  std::string SceneName;
  std::vector<FAutomationActorSnapshot> Actors;
};

struct FAutomationActorQuery {
  std::optional<std::string> ClassName;
  std::optional<std::string> InstanceName;
};

struct FAutomationActorQueryText {
  std::optional<std::string> ClassName;
  std::optional<std::string> InstanceName;
  bool bHasUnknownParameter = false;
  bool bHasDuplicateParameter = false;
};

struct FAutomationActorClassInfo {
  std::string ClassName;
  bool bIsGameMode = false;
};

struct FAutomationLevelInfo {
  uint32_t SceneId = 0;
  std::string LevelPath;
};

enum class EAutomationWorldReadStatus : uint8_t {
  Success,
  WorldNotAvailable,
  ActorNotFound,
  InvalidState
};

using FAutomationActorListProvider = std::function<
    EAutomationWorldReadStatus(const FAutomationActorQuery&, FAutomationActorListSnapshot&)>;
using FAutomationActorProvider =
    std::function<EAutomationWorldReadStatus(FActorId, FAutomationActorSnapshot&)>;
using FAutomationActorComponentListProvider =
    std::function<EAutomationWorldReadStatus(FActorId, FAutomationActorComponentListSnapshot&)>;
using FAutomationActorClassListProvider = std::function<std::vector<FAutomationActorClassInfo>()>;
using FAutomationLevelListProvider = std::function<std::vector<FAutomationLevelInfo>()>;
using FAutomationActorClassExistsProvider = std::function<bool(std::string_view)>;

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
      FAutomationActorComponentListProvider InActorComponentListProvider = {},
      FAutomationSpawnActorProvider InSpawnActorProvider = {},
      FAutomationDestroyActorProvider InDestroyActorProvider = {},
      FAutomationPatchActorTransformProvider InPatchActorTransformProvider = {},
      FAutomationMethodRegistry* InMethodRegistry = nullptr,
      FAutomationActorResolver InActorResolver = {},
      FAutomationSystemCommandRegistry* InSystemCommandRegistry = nullptr,
      FAutomationActorClassListProvider InActorClassListProvider = {},
      FAutomationLevelListProvider InLevelListProvider = {},
      FAutomationActorClassExistsProvider InActorClassExistsProvider = {}
  );

  FAutomationHttpResponse GetState();
  FAutomationHttpResponse GetWorldActors(const FAutomationActorQueryText& Query = {});
  FAutomationHttpResponse GetWorldActor(std::string_view ActorIdText);
  FAutomationHttpResponse GetWorldActorComponents(std::string_view ActorIdText);
  FAutomationHttpResponse GetActorClasses();
  FAutomationHttpResponse GetLevels();
  FAutomationHttpResponse GetActorClassMethods(std::string_view ClassName);
  FAutomationHttpResponse CreateWorldActor(const nlohmann::json& Body);
  FAutomationHttpResponse DeleteWorldActor(std::string_view ActorIdText);
  FAutomationHttpResponse PatchWorldActorTransform(
      std::string_view ActorIdText, const nlohmann::json& Body
  );
  FAutomationHttpResponse GetWorldActorMethods(std::string_view ActorIdText);
  FAutomationHttpResponse InvokeWorldActorMethod(
      std::string_view ActorIdText, std::string_view MethodName, const nlohmann::json& Body
  );
  FAutomationHttpResponse GetSystemCommands();
  FAutomationHttpResponse ExecuteSystemCommand(
      std::string_view CommandName, const nlohmann::json& Body
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
  FAutomationActorComponentListProvider ActorComponentListProvider;
  FAutomationSpawnActorProvider SpawnActorProvider;
  FAutomationDestroyActorProvider DestroyActorProvider;
  FAutomationPatchActorTransformProvider PatchActorTransformProvider;
  FAutomationMethodRegistry* MethodRegistry = nullptr;
  FAutomationActorResolver ActorResolver;
  FAutomationSystemCommandRegistry* SystemCommandRegistry = nullptr;
  FAutomationActorClassListProvider ActorClassListProvider;
  FAutomationLevelListProvider LevelListProvider;
  FAutomationActorClassExistsProvider ActorClassExistsProvider;
};
