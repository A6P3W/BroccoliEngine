#pragma once

#include "BroccoliEngineAPI.h"
#include "World/WorldTypes.h"

class BROCCOLI_ENGINE_API FAutomationWorldService {
 public:
  FAutomationActorListProvider CreateActorListProvider();
  FAutomationActorProvider CreateActorProvider();
  FAutomationActorComponentListProvider CreateActorComponentListProvider();
  FAutomationSpawnActorProvider CreateSpawnActorProvider();
  FAutomationDestroyActorProvider CreateDestroyActorProvider();
  FAutomationPatchActorTransformProvider CreateTransformProvider();
  FAutomationActorResolver CreateActorResolver();
  FAutomationComponentResolver CreateComponentResolver();
};
