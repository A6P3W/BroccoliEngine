#include <Windows.h>
#include <imgui.h>

#include <iostream>

#include "Application.h"
#include "EOSCoreManager.h"
#include "ExamplePlugin/ExamplePlugin.h"
#include "LevelStarter/LevelStarterGameMode.h"
#include "LevelStarter/LevelStarterWidget.h"
#include "Log.h"
#include "PathResolver.h"
#include "SceneManager.h"

namespace {
// ゲームの初期セットアップを行うためのコールバック関数
void SetupGame() {
  // エンジンで管理されている ImGui コンテキストを取得し、ゲーム側の ImGui に設定
  ImGui::SetCurrentContext(static_cast<ImGuiContext*>(Application::GetImGuiContext()));
  // EOSのオンラインサービスを初期化
  EOSCoreManager::GetInstance().InitializeOnlineServices();

  const int ExamplePluginRandomNumber = ExamplePlugin::GetRandomNumber();
  M_LOG(Log, "ExamplePlugin random number: {}", ExamplePluginRandomNumber);
  (void)ExamplePluginRandomNumber;

  // ゲーム起動時に最初に読み込むレベル（ステージ）ファイルのパスを設定
  SceneManager::GetInstance().SetStartupLevelPath("Game/LevelStarter.BLevel");
}
}  // namespace

int WINAPI
WinMain(HINSTANCE Instance, HINSTANCE PreviousInstance, LPSTR CommandLine, int ShowCommand) {
  PathResolver::SetGameName("Launcher");

  // アプリケーション初期化用のセットアップコールバック関数を登録
  Application::SetGameSetupCallback(&SetupGame);
  // エンジンのメインアプリケーションインスタンスを生成
  Application App;
  // アプリケーションの実行を開始し、メインループを実行（正常終了で true、異常終了で false を返却）
  return App.Run() ? 0 : 1;
}
