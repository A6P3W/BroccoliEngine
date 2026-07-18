#include "BasicGameplayGameMode.h"

#include "../Common/LauncherPlayerController.h"
#include "BasicGameplayPawn.h"

// このゲームモードクラスを ActorRegistry にゲームモードとして自動登録するマクロ
REGISTER_GAME_MODE(ABasicGameplayGameMode)

ABasicGameplayGameMode::ABasicGameplayGameMode() {
  // プレイヤーがスポーンした際にデフォルトで操作する Pawnクラスを設定（StaticClassName()でクラス名文字列を取得）
  SetDefaultPawnClass(ABasicGameplayPawn::StaticClassName());
  // プレイヤーの入力を管理・制御するデフォルトの PlayerController クラスを設定
  SetDefaultPlayerControllerClass(ALauncherPlayerController::StaticClassName());
}
