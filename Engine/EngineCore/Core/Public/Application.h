#pragma once

#include <memory>

#include "BroccoliEngineAPI.h"
#include "EngineUpdateControl.h"

class FAutomationSubsystem;

class BROCCOLI_ENGINE_API Application {
 public:
  Application();
  ~Application();

  bool Run();

  static void SetWindowResolution(int Width, int Height);
  static void SetGameSetupCallback(void (*Callback)());
  static void QuitGame();
  /// @brief アプリケーションが保持するエンジン更新制御クラスを取得する。
  static FEngineUpdateControl* GetUpdateControl();
  static void* GetImGuiContext();

 private:
  /// @brief 1フレーム分の各サブシステムおよびWorldの更新処理を行う。
  /// @param DeltaTime
  /// @param ProcessInput 入力デバイスの更新を行うかどうか（ウィンドウリサイズ中はfalse）
  /// @return 更新が正常に行われた場合はtrue
  bool EngineUpdate(float DeltaTime, bool ProcessInput);
  /// @brief Control機能のコマンドキューを処理する。
  void ControlUpdate();
  /// @brief 保留中のシーン切替を実行し、Editor用オフスクリーンバッファを更新する。
  /// @return レベル切替完了後の指定により、直後にエンジン更新を停止する場合はtrue
  bool ProcessSceneChanges();
  /// @brief 通常ループおよびウィンドウリサイズ時の1フレーム進行を統括。
  /// @param DeltaTime
  /// @param ProcessInput 入力デバイスの更新を行うかどうか
  /// @param CompleteFrame フレーム終了時にOSイベント処理・FPS待機まで行うかどうか
  void Tick(float DeltaTime, bool ProcessInput, bool CompleteFrame);
  bool Draw(bool CompleteFrame);
  void Shutdown();
  void InitializeAutomation();
  bool InitOffscreenBuffer(int Width, int Height);

  /// @brief エンジン更新の停止・再開・ステップ実行および非同期操作の状態管理。
  FEngineUpdateControl UpdateControl;
  /// @brief OSのモーダルリサイズループ等によるTickの再入を防ぐガードフラグ。
  bool TickInProgress = false;
  float DeltaTime = 0.0f;
  bool RaylibInitialized = false;
  bool AudioInitialized = false;
  bool ImGuiInitialized = false;
  void* OffscreenBuffer = nullptr;

  std::unique_ptr<FAutomationSubsystem> AutomationSubsystem;
};
