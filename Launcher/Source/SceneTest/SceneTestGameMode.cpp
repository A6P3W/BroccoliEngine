#include "SceneTestGameMode.h"

#include "../Common/LauncherPlayerController.h"

REGISTER_GAME_MODE(ASceneTestGameMode)

ASceneTestGameMode::ASceneTestGameMode() {
  SetDefaultPlayerControllerClass(ALauncherPlayerController::StaticClassName());
}
