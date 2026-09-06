#pragma once

#include "BroccoliEngineAPI.h"
#include "World/AutomationDiscoveryTypes.h"

class BROCCOLI_ENGINE_API FAutomationDiscoveryAdapter {
 public:
  FAutomationActorClassListProvider CreateActorClassListProvider();
  FAutomationLevelListProvider CreateLevelListProvider();
  FAutomationActorClassExistsProvider CreateActorClassExistsProvider();
};
