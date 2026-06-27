#include "SceneManager.h"
#include "NetworkTest/NetworkTestGameMode.h"

void SetupGame() {
	auto& sceneManager = SceneManager::GetInstance();
	sceneManager.RegisterLevelPath(NetworkTestSceneIds::Level1, "../Engine/EngineSide/NetworkTest/NetworkTestLevel.BLevel");
	sceneManager.RegisterLevelPath(NetworkTestSceneIds::Level2, "../Engine/EngineSide/NetworkTest/NetworkTestLevel2.BLevel");
	sceneManager.OpenSceneById(NetworkTestSceneIds::Level1);
}
