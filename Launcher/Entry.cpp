#include <Windows.h>
#include <imgui.h>
#include <iostream>

#include "Application.h"
#include "EOSCoreManager.h"
#include "LevelStarter/LevelStarterGameMode.h"
#include "SceneManager.h"

namespace {
void SetupGame() {
  ImGui::SetCurrentContext(static_cast<ImGuiContext*>(Application::GetImGuiContext()));
  EOSCoreManager::Get().InitializeOnlineServices();

  SceneManager::GetInstance().SetStartupLevelPath("LevelStarter/LevelStarter.BLevel");
}
}

void SetupDllSearchPath() {
  SetDllDirectoryA("Engine/Binaries");
}

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE PreviousInstance, LPSTR CommandLine, int ShowCommand) {
  SetupDllSearchPath();
  Application::SetGameSetupCallback(&SetupGame);
  Application App;
  return App.Run() ? 0 : 1;
} 
