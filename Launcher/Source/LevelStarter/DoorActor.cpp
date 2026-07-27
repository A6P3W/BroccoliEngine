#include "DoorActor.h"

#include "AutomationRegistryHelper.h"
#include "DoorAutomationTestComponent.h"

REGISTER_ACTOR(ADoorActor)
REGISTER_AUTOMATION_METHOD(
    "open_door",
    "Opens the door when it is unlocked.",
    EAutomationPermission::WorldMutation,
    &ADoorActor::OpenDoor
)
REGISTER_AUTOMATION_METHOD(
    "close_door", "Closes the door.", EAutomationPermission::WorldMutation, &ADoorActor::CloseDoor
)
REGISTER_AUTOMATION_METHOD(
    "set_locked",
    "Sets the door lock state.",
    EAutomationPermission::WorldMutation,
    &ADoorActor::SetLocked,
    AUTOMATION_PARAMS(AUTOMATION_PARAM("locked", "New lock state."))
)
REGISTER_AUTOMATION_METHOD(
    "is_open",
    "Returns whether the door is open.",
    EAutomationPermission::ReadOnly,
    &ADoorActor::IsOpen
)
REGISTER_AUTOMATION_METHOD(
    "get_door_state",
    "Returns the current door state.",
    EAutomationPermission::ReadOnly,
    &ADoorActor::GetDoorState,
    AUTOMATION_RESULT_ADAPTER([](const FDoorState& State) {
      return nlohmann::json{{"is_open", State.bIsOpen}, {"is_locked", State.bIsLocked}};
    })
)

ADoorActor::ADoorActor() { NewObject<MDoorAutomationTestComponent>(this); }

void ADoorActor::OpenDoor() {
  if (!bIsLocked) {
    bIsOpen = true;
  }
}

void ADoorActor::CloseDoor() { bIsOpen = false; }

void ADoorActor::SetLocked(bool bLockedValue) {
  bIsLocked = bLockedValue;
  if (bIsLocked) {
    bIsOpen = false;
  }
}

bool ADoorActor::IsOpen() const { return bIsOpen; }

FDoorState ADoorActor::GetDoorState() const { return {bIsOpen, bIsLocked}; }
