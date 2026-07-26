#include <Windows.h>
#include <imgui.h>

#include <iostream>
#include <stdexcept>
#include <utility>

#include "Application.h"
#include "AutomationMethodRegistry.h"
#include "EOSCoreManager.h"
#include "LevelStarter/LevelStarterGameMode.h"
#include "LevelStarter/LevelStarterWidget.h"
#include "SceneManager.h"

namespace {
void RegisterAutomationActorMethods(FAutomationMethodRegistry& Registry) {
  FAutomationMethodDescriptor Descriptor;
  Descriptor.Name = "get_status";
  Descriptor.Description = "Return the LevelStarter widget status for automation verification.";
  Descriptor.Permission = EAutomationPermission::ReadOnly;
  Descriptor.Handler = [](AActor& Actor, const nlohmann::json&) {
    const auto* Widget = dynamic_cast<ALevelStarterWidget*>(&Actor);
    if (!Widget) {
      throw std::runtime_error("The LevelStarter widget is unavailable.");
    }
    return nlohmann::json{{"ready", true}, {"instanceName", Widget->GetInstanceName()}};
  };

  std::string Error;
  if (!Registry.RegisterMethod(
          ALevelStarterWidget::StaticClassName(), std::move(Descriptor), &Error
      )) {
    throw std::runtime_error(Error);
  }
}

// ゲームの初期セットアップを行うためのコールバック関数
void SetupGame() {
  // エンジンで管理されている ImGui コンテキストを取得し、ゲーム側の ImGui に設定
  ImGui::SetCurrentContext(static_cast<ImGuiContext*>(Application::GetImGuiContext()));
  // EOSのオンラインサービスを初期化
  EOSCoreManager::GetInstance().InitializeOnlineServices();

  // ゲーム起動時に最初に読み込むレベル（ステージ）ファイルのパスを設定
  SceneManager::GetInstance().SetStartupLevelPath("Resources/LevelStarter.BLevel");
  Application::SetAutomationMethodRegistrationCallback(&RegisterAutomationActorMethods);
}
}  // namespace

int WINAPI
WinMain(HINSTANCE Instance, HINSTANCE PreviousInstance, LPSTR CommandLine, int ShowCommand) {
  // アプリケーション初期化用のセットアップコールバック関数を登録
  Application::SetGameSetupCallback(&SetupGame);
  // エンジンのメインアプリケーションインスタンスを生成
  Application App;
  // アプリケーションの実行を開始し、メインループを実行（正常終了で true、異常終了で false を返却）
  return App.Run() ? 0 : 1;
}
