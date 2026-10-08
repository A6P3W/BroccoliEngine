#include "DoorActor.h"

#include "ControlMacros.h"
#include "DoorAutomationTestComponent.h"

REGISTER_ACTOR(ADoorActor)
REGISTER_CONTROL_CLASS(ADoorActor)

nlohmann::json ADoorActor::DoorStateToJson(const FDoorState& State) {
  return {{"is_open", State.bIsOpen}, {"is_locked", State.bIsLocked}};
}

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
