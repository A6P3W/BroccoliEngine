#include <Windows.h>

#include "Application.h"
#include "EOSCoreManager.h"
#include "LevelStarter/LevelStarterGameMode.h"
#include "SceneManager.h"

namespace {
void SetupGame() {
  EOSCoreManager::Get().InitializeOnlineServices();

  SceneManager::GetInstance().SetStartupLevelPath("LevelStarter/LevelStarter.BLevel");
}
}

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE PreviousInstance, LPSTR CommandLine, int ShowCommand) {
  Application::SetGameSetupCallback(&SetupGame);
  Application App;
  return App.Run() ? 0 : 1;
}
