#include "DoorAutomationTestComponent.h"

#include "AutomationMacros.h"

REGISTER_AUTOMATION_METHOD(
    "set_active",
    "Sets the DoorActor automation test component active state.",
    EAutomationPermission::WorldMutation,
    &MDoorAutomationTestComponent::SetActive,
    AUTOMATION_PARAMS(AUTOMATION_PARAM("active", "New active state."))
)
REGISTER_AUTOMATION_METHOD(
    "is_active",
    "Returns the DoorActor automation test component active state.",
    EAutomationPermission::ReadOnly,
    &MDoorAutomationTestComponent::IsActive,
    AUTOMATION_PARAMS(),
    ([](const bool Active) { return nlohmann::json{{"active", Active}}; })
)

void MDoorAutomationTestComponent::SetActive(bool bInActive) { bActive = bInActive; }

bool MDoorAutomationTestComponent::IsActive() const { return bActive; }
