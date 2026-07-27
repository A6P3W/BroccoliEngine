#include "DoorActor.h"

#include "AutomationAutoRegistrar.h"
#include "AutomationRegistryHelper.h"
#include "DoorAutomationTestComponent.h"

REGISTER_ACTOR(ADoorActor)
REGISTER_AUTOMATION_METHODS(ADoorActor)

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

void ADoorActor::RegisterAutomationMethods(FAutomationMethodRegistry& Registry) {
  AutomationHelper::RegisterMethod(
      Registry,
      "open_door",
      "Opens the door when it is unlocked.",
      EAutomationPermission::WorldMutation,
      &ADoorActor::OpenDoor
  );
  AutomationHelper::RegisterMethod(
      Registry,
      "close_door",
      "Closes the door.",
      EAutomationPermission::WorldMutation,
      &ADoorActor::CloseDoor
  );
  AutomationHelper::RegisterMethod(
      Registry,
      "set_locked",
      "Sets the door lock state.",
      EAutomationPermission::WorldMutation,
      &ADoorActor::SetLocked,
      AutomationParam<bool>("locked", "New lock state.")
  );
  AutomationHelper::RegisterMethod(
      Registry,
      "is_open",
      "Returns whether the door is open.",
      EAutomationPermission::ReadOnly,
      &ADoorActor::IsOpen
  );
  AutomationHelper::RegisterMethod(
      Registry,
      "get_door_state",
      "Returns the current door state.",
      EAutomationPermission::ReadOnly,
      &ADoorActor::GetDoorState
  );
}
