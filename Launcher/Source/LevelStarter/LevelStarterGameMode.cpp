#include "LevelStarterGameMode.h"

#include "LevelStarterWidget.h"
#include "PlayerController.h"
#include "World.h"

// このゲームモードクラスを ActorRegistry にゲームモードとして自動登録するマクロ
REGISTER_GAME_MODE(ALevelStarterGameMode)

ALevelStarterGameMode::ALevelStarterGameMode() = default;

void ALevelStarterGameMode::BeginPlay() {
  // 基底クラス AGameModeBase の BeginPlay 処理を実行
  AGameModeBase::BeginPlay();

  // 現在のアクターが属しているワールド（シーン情報などを持つ管理クラス）を取得
  if (!GetWorld()) {
    return;
  }

  // ローカルプレイヤー用のコントローラーを取得、存在しなければ生成する
  if (APlayerController* controller = GetWorld()->GetOrCreateLocalPlayerController()) {
    // プレイヤーの入力モードを UI 操作のみ（ゲーム内キャラクターの移動・視点操作等は無効）に設定
    controller->SetInputMode(EInputMode::UIOnly);
  }

  // 指定したウィジェットアクター（ALevelStarterWidget）をワールドに生成する
  GetWorld()->SpawnActor<ALevelStarterWidget>();
}