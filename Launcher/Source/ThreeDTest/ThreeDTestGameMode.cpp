#include "ThreeDTestGameMode.h"

#include "../Common/LauncherPlayerController.h"

REGISTER_GAME_MODE(AThreeDTestGameMode)

AThreeDTestGameMode::AThreeDTestGameMode() {
  SetDefaultPlayerControllerClass(ALauncherPlayerController::StaticClassName());
}
