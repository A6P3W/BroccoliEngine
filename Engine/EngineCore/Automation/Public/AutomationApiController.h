#pragma once

#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "ActorId.h"
#include "AutomationCommandQueue.h"
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

class BROCCOLI_ENGINE_API FAutomationApiController {
 public:
  FAutomationApiController(
      FAutomationCommandQueue& InCommandQueue,
      const FAutomationConfig& InConfig,
      FAutomationStateProvider InStateProvider,
      FAutomationActorListProvider InActorListProvider = {},
      FAutomationActorProvider InActorProvider = {}
  );

  FAutomationHttpResponse GetState();
  FAutomationHttpResponse GetWorldActors();
  FAutomationHttpResponse GetWorldActor(std::string_view ActorIdText);

 private:
  static bool TryParseActorId(std::string_view Text, FActorId& OutActorId);
  static nlohmann::json SerializeActor(const FAutomationActorSnapshot& Actor);
  static nlohmann::json SerializeActorList(const FAutomationActorListSnapshot& Snapshot);
  FAutomationHttpResponse WaitForResult(FAutomationCommandTicket&& Ticket);
  static int GetHttpStatusCode(const nlohmann::json& Body);

  FAutomationCommandQueue& CommandQueue;
  FAutomationConfig Config;
  FAutomationStateProvider StateProvider;
  FAutomationActorListProvider ActorListProvider;
  FAutomationActorProvider ActorProvider;
};
