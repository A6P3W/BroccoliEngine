#include "LauncherPlayerController.h"

#include "EnhancedInputComponent.h"
#include "Log.h"
#include "SceneManager.h"

REGISTER_ACTOR(ALauncherPlayerController)

void ALauncherPlayerController::SetupPlayerInputComponent(
    MEnhancedInputComponent* PlayerInputComponent
) {
  APlayerController::SetupPlayerInputComponent(PlayerInputComponent);
  if (PlayerInputComponent) {
    // UIOnly samples must also be able to leave with Escape, so this is a UI action.
    PlayerInputComponent->BindAction(
        InputAction::Pause,
        ETriggerEvent::Started,
        this,
        &ALauncherPlayerController::ReturnToLevelStarter,
        true
    );
  }
}

void ALauncherPlayerController::ReturnToLevelStarter() {
  const std::string& CurrentPath = SceneManager::GetInstance().GetCurrentLevelPath();
  if (CurrentPath == "Resources/LevelStarter.BLevel") {
    return;
  }

  M_LOG("Escape pressed: return to LevelStarter.");
  SceneManager::GetInstance().OpenLevelByPath("Resources/LevelStarter.BLevel");
}
