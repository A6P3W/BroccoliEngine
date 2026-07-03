#pragma once

#include "GameModeBase.h"

class ALevelStarterGameMode : public AGameModeBase {
 public:
  DEFINE_ACTOR_CLASS(ALevelStarterGameMode)

  ALevelStarterGameMode();

 protected:
  void BeginPlay() override;
};