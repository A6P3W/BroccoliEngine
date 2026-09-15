#pragma once

#include <functional>
#include <nlohmann/json.hpp>
#include <string>

#include "AutomationTypes.h"
#include "BroccoliEngineAPI.h"

class AActor;
class MActorComponent;

namespace BroccoliAutomationDetail {
using FAutomationActorHandler = std::function<nlohmann::json(AActor&, const nlohmann::json&)>;
using FAutomationComponentHandler =
    std::function<nlohmann::json(MActorComponent&, const nlohmann::json&)>;

class BROCCOLI_ENGINE_API FAutomationRegistrationContext {
 public:
  FAutomationRegistrationContext(void* InActorRegistry, void* InComponentRegistry);

  void RegisterActorMethod(
      std::string ClassName,
      std::string Name,
      std::string Description,
      nlohmann::json InputSchema,
      EAutomationPermission Permission,
      FAutomationActorHandler Handler
  );
  void RegisterComponentMethod(
      std::string ClassName,
      std::string Name,
      std::string Description,
      nlohmann::json InputSchema,
      EAutomationPermission Permission,
      FAutomationComponentHandler Handler
  );

 private:
  void* ActorRegistry = nullptr;
  void* ComponentRegistry = nullptr;
};

using FAutomationRegistrationCallback = void (*)(FAutomationRegistrationContext&);

class BROCCOLI_ENGINE_API FAutomationRegistrationToken {
 public:
  explicit FAutomationRegistrationToken(FAutomationRegistrationCallback Callback);
};
}  // namespace BroccoliAutomationDetail
