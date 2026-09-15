#pragma once

#include "HttpControllerBase.h"

#include <string_view>

#include "Registry/ActorMethodRegistry.h"
#include "World/DiscoveryTypes.h"

class FAutomationDiscoveryController final : public FAutomationHttpControllerBase {
 public:
  FAutomationDiscoveryController(
      FAutomationHttpRequestExecutor& InExecutor,
      FAutomationCommandQueue& InCommandQueue,
      FAutomationActorMethodRegistry& InMethodRegistry,
      FAutomationActorClassListProvider InActorClassListProvider,
      FAutomationLevelListProvider InLevelListProvider,
      FAutomationActorClassExistsProvider InActorClassExistsProvider
  );

  FAutomationHttpResponse GetActorClasses();
  FAutomationHttpResponse GetLevels();
  FAutomationHttpResponse GetActorClassMethods(std::string_view ClassName);

 private:
  FAutomationCommandQueue& CommandQueue;
  FAutomationActorMethodRegistry* MethodRegistry = nullptr;
  FAutomationActorClassListProvider ActorClassListProvider;
  FAutomationLevelListProvider LevelListProvider;
  FAutomationActorClassExistsProvider ActorClassExistsProvider;
};
