#include "Animation3DTestGameMode.h"

#include "../Common/LauncherPlayerController.h"

REGISTER_GAME_MODE(AAnimation3DTestGameMode)

AAnimation3DTestGameMode::AAnimation3DTestGameMode() {
  SetDefaultPlayerControllerClass(ALauncherPlayerController::StaticClassName());
}
