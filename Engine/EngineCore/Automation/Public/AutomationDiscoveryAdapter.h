#pragma once

#include "AutomationApiController.h"
#include "BroccoliEngineAPI.h"

class BROCCOLI_ENGINE_API FAutomationDiscoveryAdapter {
 public:
  FAutomationActorClassListProvider CreateActorClassListProvider();
  FAutomationLevelListProvider CreateLevelListProvider();
  FAutomationActorClassExistsProvider CreateActorClassExistsProvider();
};
