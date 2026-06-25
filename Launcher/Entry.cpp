#include "SceneManager.h"
#include "NetworkTest/NetworkTestGameMode.h"

void SetupGame() {
	auto& sceneManager = SceneManager::GetInstance();
	sceneManager.RegisterScene<ANetworkTestGameMode>(NetworkTestSceneIds::Level1);
	sceneManager.RegisterScene<ANetworkTestLevel2GameMode>(NetworkTestSceneIds::Level2);
	sceneManager.OpenSceneById(NetworkTestSceneIds::Level1);
}
