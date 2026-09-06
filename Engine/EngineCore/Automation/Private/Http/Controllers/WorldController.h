#pragma once

#include "HttpControllerBase.h"

class FAutomationWorldController final : public FAutomationHttpControllerBase {
 public:
  FAutomationWorldController(
      FAutomationHttpRequestExecutor& InExecutor,
      FAutomationCommandQueue& InCommandQueue,
      FAutomationStateProvider InStateProvider,
      FAutomationActorListProvider InActorListProvider,
      FAutomationActorProvider InActorProvider,
      FAutomationActorComponentListProvider InActorComponentListProvider,
      FAutomationSpawnActorProvider InSpawnActorProvider,
      FAutomationDestroyActorProvider InDestroyActorProvider,
      FAutomationPatchActorTransformProvider InPatchActorTransformProvider
  );

  FAutomationHttpResponse GetState();
  FAutomationHttpResponse GetWorldActors(const FAutomationActorQueryText& Query = {});
  FAutomationHttpResponse GetWorldActor(std::string_view ActorIdText);
  FAutomationHttpResponse GetWorldActorComponents(std::string_view ActorIdText);
  FAutomationHttpResponse CreateWorldActor(const nlohmann::json& Body);
  FAutomationHttpResponse DeleteWorldActor(std::string_view ActorIdText);
  FAutomationHttpResponse PatchWorldActorTransform(
      std::string_view ActorIdText, const nlohmann::json& Body
  );

 private:
  FAutomationCommandQueue& CommandQueue;
  FAutomationStateProvider StateProvider;
  FAutomationActorListProvider ActorListProvider;
  FAutomationActorProvider ActorProvider;
  FAutomationActorComponentListProvider ActorComponentListProvider;
  FAutomationSpawnActorProvider SpawnActorProvider;
  FAutomationDestroyActorProvider DestroyActorProvider;
  FAutomationPatchActorTransformProvider PatchActorTransformProvider;
};

