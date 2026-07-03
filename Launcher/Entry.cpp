#include "EOSCoreManager.h"
#include "NetworkTest/NetworkTestGameMode.h"
#include "SceneManager.h"

void SetupGame() {
  EOSCoreManager::Get().InitializeOnlineServices();

  auto& sceneManager = SceneManager::GetInstance();
  sceneManager.OpenLevelByPath(NetworkTestLevelPaths::Level1);
}
