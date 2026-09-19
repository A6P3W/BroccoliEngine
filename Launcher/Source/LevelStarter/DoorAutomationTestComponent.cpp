#include "DoorAutomationTestComponent.h"

#include "ControlMacros.h"

CONTROL_METHOD(
    "set_active",
    "Sets the DoorActor automation test component active state.",
    &MDoorAutomationTestComponent::SetActive,
    CONTROL_PARAMETERS(CONTROL_PARAMETER("active", "New active state."))
)
CONTROL_METHOD(
    "is_active",
    "Returns the DoorActor automation test component active state.",
    &MDoorAutomationTestComponent::IsActive,
    CONTROL_PARAMETERS(),
    ([](const bool Active) { return nlohmann::json{{"active", Active}}; })
)

void MDoorAutomationTestComponent::SetActive(bool bInActive) { bActive = bInActive; }

bool MDoorAutomationTestComponent::IsActive() const { return bActive; }
