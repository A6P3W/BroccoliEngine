#pragma once

#include "HttpControllerBase.h"

#include <nlohmann/json.hpp>
#include <string_view>

#include "Registry/ActorMethodRegistry.h"
#include "Registry/ComponentMethodRegistry.h"
#include "World/WorldTypes.h"

class FAutomationInvocationController final : public FAutomationHttpControllerBase {
 public:
  FAutomationInvocationController(
      FAutomationHttpRequestExecutor& InExecutor,
      FAutomationCommandQueue& InCommandQueue,
      FAutomationActorMethodRegistry& InMethodRegistry,
      FAutomationActorResolver InActorResolver,
      FAutomationComponentMethodRegistry& InComponentMethodRegistry,
      FAutomationComponentResolver InComponentResolver
  );

  FAutomationHttpResponse GetWorldActorMethods(std::string_view ActorIdText);
  FAutomationHttpResponse InvokeWorldActorMethod(
      std::string_view ActorIdText, std::string_view MethodName, const nlohmann::json& Body
  );
  FAutomationHttpResponse GetWorldActorComponentMethods(
      std::string_view ActorIdText, std::string_view ComponentIdText
  );
  FAutomationHttpResponse InvokeWorldActorComponentMethod(
      std::string_view ActorIdText,
      std::string_view ComponentIdText,
      std::string_view MethodName,
      const nlohmann::json& Body
  );

 private:
  FAutomationCommandQueue& CommandQueue;
  FAutomationActorMethodRegistry* MethodRegistry = nullptr;
  FAutomationActorResolver ActorResolver;
  FAutomationComponentMethodRegistry* ComponentMethodRegistry = nullptr;
  FAutomationComponentResolver ComponentResolver;
};
