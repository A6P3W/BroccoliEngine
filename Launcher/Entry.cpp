#include "SceneManager.h"
#include "NetworkTest/NetworkTestGameMode.h"

void SetupGame() {
	SceneManager::GetInstance().OpenScene<ANetworkTestGameMode>();
}
