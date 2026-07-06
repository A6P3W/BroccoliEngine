#include "EOSCoreManager.h"
#include "LevelStarter/LevelStarterGameMode.h"
#include "SceneManager.h"

void SetupGame() {
  EOSCoreManager::Get().InitializeOnlineServices();

  SceneManager::GetInstance().SetStartupLevelPath("LevelStarter/LevelStarter.BLevel");
}
