#include "SceneTestGameMode.h"

#include "../Common/LauncherPlayerController.h"

// このゲームモードクラスを ActorRegistry にゲームモードとして自動登録するマクロ
REGISTER_GAME_MODE(ASceneTestGameMode)

ASceneTestGameMode::ASceneTestGameMode() {
  // サンプル用の共通プレイヤーコントローラー（Escapeでの終了機能付き）をデフォルトに設定
  SetDefaultPlayerControllerClass(ALauncherPlayerController::StaticClassName());
}
