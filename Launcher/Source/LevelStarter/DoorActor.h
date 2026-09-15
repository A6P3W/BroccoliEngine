#pragma once

#include "Actor.h"

struct FDoorState {
  bool bIsOpen = false;
  bool bIsLocked = false;
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
