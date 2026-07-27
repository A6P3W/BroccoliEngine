#include "DoorAutomationTestComponent.h"

#include "AutomationAutoRegistrar.h"
#include "AutomationRegistryHelper.h"

REGISTER_AUTOMATION_COMPONENT_METHODS(MDoorAutomationTestComponent)

void MDoorAutomationTestComponent::SetActive(bool bInActive) { bActive = bInActive; }

bool MDoorAutomationTestComponent::IsActive() const { return bActive; }

void MDoorAutomationTestComponent::RegisterAutomationMethods(
    FAutomationComponentMethodRegistry& Registry
) {
  AutomationHelper::RegisterComponentMethod(
      Registry,
      "set_active",
      "Sets the DoorActor automation test component active state.",
      EAutomationPermission::WorldMutation,
      &MDoorAutomationTestComponent::SetActive,
      AutomationParam<bool>("active", "New active state.")
  );
  AutomationHelper::RegisterComponentMethod(
      Registry,
      "is_active",
      "Returns the DoorActor automation test component active state.",
      EAutomationPermission::ReadOnly,
      &MDoorAutomationTestComponent::IsActive
  );
}
