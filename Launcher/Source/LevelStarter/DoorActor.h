#pragma once

#include "Actor.h"
#include "AutomationJsonConverter.h"

class FAutomationMethodRegistry;

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

  static void RegisterAutomationMethods(FAutomationMethodRegistry& Registry);

 private:
  bool bIsOpen = false;
  bool bIsLocked = false;
};
