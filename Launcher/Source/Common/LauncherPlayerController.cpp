#include "LauncherPlayerController.h"

#include "EnhancedInputComponent.h"
#include "Log.h"
#include "SceneManager.h"

// このプレイヤーコントローラーを ActorRegistry に一般アクターとして自動登録するマクロ
REGISTER_ACTOR(ALauncherPlayerController)

void ALauncherPlayerController::SetupPlayerInputComponent(
    MEnhancedInputComponent* PlayerInputComponent
) {
  // 基底クラス APlayerController のインプットバインド処理を実行
  APlayerController::SetupPlayerInputComponent(PlayerInputComponent);
  if (PlayerInputComponent) {
    // 一時停止/メニュー戻り（Escape）アクションが発生した際（ETriggerEvent::Started）に呼び出される関数をバインド
    // 最後の引数に true を渡すことで、UI操作のみが有効なシーンであっても入力を受け付けるよう設定
    PlayerInputComponent->BindAction(
        InputAction::Pause,
        ETriggerEvent::Started,
        this,
        &ALauncherPlayerController::ReturnToLevelStarter,
        true
    );
  }
}

void ALauncherPlayerController::ReturnToLevelStarter() {
  // 現在読み込まれているレベルのパスを取得
  const std::string& CurrentPath = SceneManager::GetInstance().GetCurrentLevelPath();
  if (CurrentPath == "Resources/LevelStarter.BLevel") {
    return;
  }

  // ログを出力
  M_LOG("Escape pressed: return to LevelStarter.");
  // 初期レベルである LevelStarter を開く
  SceneManager::GetInstance().OpenLevelByPath("Resources/LevelStarter.BLevel");
}
