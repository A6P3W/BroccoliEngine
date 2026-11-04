#pragma once

#include <nlohmann/json.hpp>

#include "Actor.h"
#include "AutomationAnnotations.h"

struct FDoorState {
  bool bIsOpen = false;
  bool bIsLocked = false;
};

class ADoorActor : public AActor {
 private:
  static nlohmann::json DoorStateToJson(const FDoorState& State);

 public:
  DEFINE_ACTOR_CLASS(ADoorActor)

  ADoorActor();

  CONTROL_METHOD(.Name = "open_door", .Description = "Opens the door when it is unlocked.")
  void OpenDoor();
  CONTROL_METHOD(.Name = "close_door", .Description = "Closes the door.")
  void CloseDoor();
  CONTROL_METHOD(.Name = "set_locked", .Description = "Sets the door lock state.")
  CONTROL_PARAMETER(.Index = 0, .Name = "locked", .Description = "New lock state.")
  void SetLocked(bool bLocked);

  CONTROL_METHOD(.Name = "is_open", .Description = "Returns whether the door is open.")
  bool IsOpen() const;
  CONTROL_METHOD(
          .Name = "get_door_state",
          .Description = "Returns the current door state.",
          .ResultAdapter = ^^ADoorActor::DoorStateToJson
  )
  FDoorState GetDoorState() const;

 private:
  bool bIsOpen = false;
  bool bIsLocked = false;
};
