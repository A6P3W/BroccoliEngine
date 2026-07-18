#pragma once

#include "GameModeBase.h"

// Select this GameMode in BasicGameplay.BLevel.  Place APlayerStart to choose the spawn point.
class ABasicGameplayGameMode : public AGameModeBase {
 public:
  DEFINE_ACTOR_CLASS(ABasicGameplayGameMode)

  ABasicGameplayGameMode();
};
