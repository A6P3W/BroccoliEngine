#pragma once

#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>

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
  FAutomationRegistrationContext(
      void* InActorRegistry, void* InComponentRegistry, std::string InModuleOwner = "Static"
  );

  void RegisterActorMethod(
      std::string ClassName,
      std::string Name,
      std::string Description,
      nlohmann::json InputSchema,
      FAutomationActorHandler Handler
  );
  void RegisterComponentMethod(
      std::string ClassName,
      std::string Name,
      std::string Description,
      nlohmann::json InputSchema,
      FAutomationComponentHandler Handler
  );

 private:
  void* ActorRegistry = nullptr;
  void* ComponentRegistry = nullptr;
  std::string ModuleOwner;
};

using FAutomationRegistrationCallback = void (*)(FAutomationRegistrationContext&);

BROCCOLI_ENGINE_API void RegisterCallbackOwned(
    FAutomationRegistrationCallback Callback, std::string ModuleOwner
);
BROCCOLI_ENGINE_API void UnregisterControlModule(std::string_view ModuleOwner);

class BROCCOLI_ENGINE_API FAutomationRegistrationToken {
 public:
  explicit FAutomationRegistrationToken(FAutomationRegistrationCallback Callback);
};
}  // namespace BroccoliAutomationDetail
