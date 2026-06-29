#include <eos_connect.h>
#include <eos_lobby.h>
#include <eos_sdk.h>
#include <EOS_ProductCredentials.h>
#include "NetworkTest/NetworkTestGameMode.h"
#include "SceneManager.h"
void SetupGame() {
  auto& sceneManager = SceneManager::GetInstance();
  sceneManager.RegisterLevelPath(
      NetworkTestSceneIds::Level1, "../Engine/EngineSide/NetworkTest/NetworkTestLevel.BLevel"
  );
  sceneManager.RegisterLevelPath(
      NetworkTestSceneIds::Level2, "../Engine/EngineSide/NetworkTest/NetworkTestLevel2.BLevel"
  );
  sceneManager.OpenSceneById(NetworkTestSceneIds::Level1);
}
