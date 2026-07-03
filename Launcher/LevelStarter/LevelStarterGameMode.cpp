#include "LevelStarterGameMode.h"

#include "LevelStarterWidget.h"
#include "PlayerController.h"
#include "World.h"

REGISTER_GAME_MODE(ALevelStarterGameMode)

ALevelStarterGameMode::ALevelStarterGameMode() = default;

void ALevelStarterGameMode::BeginPlay() {
  AGameModeBase::BeginPlay();

  if (!GetWorld()) {
    return;
  }

  if (APlayerController* controller = GetWorld()->GetOrCreateLocalPlayerController()) {
    controller->SetInputMode(EInputMode::UIOnly);
  }

  GetWorld()->SpawnActor<ALevelStarterWidget>();
}