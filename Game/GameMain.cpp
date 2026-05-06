#include "SceneManager.h"
#include "Objects/DefaultScene.h"
void SetupGame() {
	SceneManager::GetInstance().OpenScene<ADefaultScene>();
}
