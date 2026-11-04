#include "GameInstance.h"

#include "SceneManager.h"

void GameInstance::OnSessionDisconnected(ESessionDisconnectReason Reason) {
  (void)Reason;
  SceneManager::GetInstance().OpenStartupLevel();
}
