#include "GameInstance.h"

#include "SceneManager.h"

void GameInstance::OnSessionDisconnected(ELobbyDisconnectReason Reason) {
  (void)Reason;
  SceneManager::GetInstance().OpenStartupLevel();
}
