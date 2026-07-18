#pragma once

#include "PlayerController.h"

// All Launcher sample controllers derive from this class.  It reserves Escape
// as a consistent way to return from a running sample to LevelStarter.
class ALauncherPlayerController : public APlayerController {
 public:
  DEFINE_ACTOR_CLASS(ALauncherPlayerController)

  void SetupPlayerInputComponent(MEnhancedInputComponent* PlayerInputComponent) override;

 private:
  void ReturnToLevelStarter();
};
