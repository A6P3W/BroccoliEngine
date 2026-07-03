#include "EOSCoreManager.h"
#include "LevelStarter/LevelStarterGameMode.h"
#include "SceneManager.h"

void SetupGame() {
  EOSCoreManager::Get().InitializeOnlineServices();

  SceneManager::GetInstance().OpenGameMode<ALevelStarterGameMode>();
}
