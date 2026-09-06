#pragma once

#include "BroccoliEngineAPI.h"
#include "World/AutomationWorldTypes.h"

class BROCCOLI_ENGINE_API FAutomationWorldAdapter {
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
