#include <EOS_ProductCredentials.h>

#include "EOSCoreManager.h"
#include "NetworkTest/NetworkTestGameMode.h"
#include "SceneManager.h"

void SetupGame() {
  EOSCoreManager::Get().InitializeOnlineServices();

  auto& sceneManager = SceneManager::GetInstance();
  sceneManager.RegisterLevelPath(
      NetworkTestSceneIds::Level1, "../Engine/EngineSide/NetworkTest/NetworkTestLevel.BLevel"
  );
  sceneManager.RegisterLevelPath(
      NetworkTestSceneIds::Level2, "../Engine/EngineSide/NetworkTest/NetworkTestLevel2.BLevel"
  );
  sceneManager.OpenSceneById(NetworkTestSceneIds::Level1);
}
