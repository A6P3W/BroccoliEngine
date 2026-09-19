#include "DoorActor.h"

#include "ControlMacros.h"
#include "DoorAutomationTestComponent.h"

REGISTER_ACTOR(ADoorActor)
CONTROL_METHOD("open_door", "Opens the door when it is unlocked.", &ADoorActor::OpenDoor)
CONTROL_METHOD("close_door", "Closes the door.", &ADoorActor::CloseDoor)
CONTROL_METHOD(
    "set_locked",
    "Sets the door lock state.",
    &ADoorActor::SetLocked,
    CONTROL_PARAMETERS(CONTROL_PARAMETER("locked", "New lock state."))
)
CONTROL_METHOD("is_open", "Returns whether the door is open.", &ADoorActor::IsOpen)
CONTROL_METHOD(
    "get_door_state",
    "Returns the current door state.",
    &ADoorActor::GetDoorState,
    CONTROL_PARAMETERS(),
    ([](const FDoorState& State) {
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
