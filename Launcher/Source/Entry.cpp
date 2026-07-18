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

  SceneManager::GetInstance().SetStartupLevelPath("Resources/LevelStarter.BLevel");
}
}  // namespace

int WINAPI
WinMain(HINSTANCE Instance, HINSTANCE PreviousInstance, LPSTR CommandLine, int ShowCommand) {
  Application::SetGameSetupCallback(&SetupGame);
  Application App;
  return App.Run() ? 0 : 1;
}
