#pragma once

#include "Actor.h"
#include "AutomationJsonConverter.h"

struct FDoorState {
  bool bIsOpen = false;
  bool bIsLocked = false;
};

template <>
struct TAutomationJsonConverter<FDoorState> {
  static nlohmann::json ToJson(const FDoorState& Value) {
    return {{"is_open", Value.bIsOpen}, {"is_locked", Value.bIsLocked}};
  }
};

class ADoorActor : public AActor {
 public:
  DEFINE_ACTOR_CLASS(ADoorActor)

  ADoorActor();

  void OpenDoor();
  void CloseDoor();
  void SetLocked(bool bLocked);

  bool IsOpen() const;
  FDoorState GetDoorState() const;

 private:
  bool bIsOpen = false;
  bool bIsLocked = false;
};
