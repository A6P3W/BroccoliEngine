#include <Windows.h>
#include <imgui.h>

#include <iostream>

#include "Application.h"
#include "EOSCoreManager.h"
#include "LevelStarter/LevelStarterGameMode.h"
#include "SceneManager.h"

namespace {
// ゲームの初期セットアップを行うためのコールバック関数
void SetupGame() {
  // エンジンで管理されている ImGui コンテキストを取得し、ゲーム側の ImGui に設定
  ImGui::SetCurrentContext(static_cast<ImGuiContext*>(Application::GetImGuiContext()));
  // Epic Online Services (EOS) のオンラインサービスを初期化
  EOSCoreManager::Get().InitializeOnlineServices();

  // ゲーム起動時に最初に読み込むレベル（ステージ）ファイルのパスを設定
  SceneManager::GetInstance().SetStartupLevelPath("Resources/LevelStarter.BLevel");
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
