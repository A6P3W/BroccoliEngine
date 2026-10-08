#include "DoorAutomationTestComponent.h"

#include "ControlMacros.h"

REGISTER_CONTROL_CLASS(MDoorAutomationTestComponent)

nlohmann::json MDoorAutomationTestComponent::ActiveToJson(bool Active) {
  return {{"active", Active}};
}

void MDoorAutomationTestComponent::SetActive(bool bInActive) { bActive = bInActive; }

bool MDoorAutomationTestComponent::IsActive() const { return bActive; }
