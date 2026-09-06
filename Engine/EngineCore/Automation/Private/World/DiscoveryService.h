#pragma once

#include "BroccoliEngineAPI.h"
#include "World/DiscoveryTypes.h"

class BROCCOLI_ENGINE_API FAutomationDiscoveryService {
 public:
  FAutomationActorClassListProvider CreateActorClassListProvider();
  FAutomationLevelListProvider CreateLevelListProvider();
  FAutomationActorClassExistsProvider CreateActorClassExistsProvider();
};
