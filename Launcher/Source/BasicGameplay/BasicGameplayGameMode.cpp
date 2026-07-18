#include "BasicGameplayGameMode.h"

#include "../Common/LauncherPlayerController.h"
#include "BasicGameplayPawn.h"

REGISTER_GAME_MODE(ABasicGameplayGameMode)

ABasicGameplayGameMode::ABasicGameplayGameMode() {
  SetDefaultPawnClass(ABasicGameplayPawn::StaticClassName());
  SetDefaultPlayerControllerClass(ALauncherPlayerController::StaticClassName());
}
