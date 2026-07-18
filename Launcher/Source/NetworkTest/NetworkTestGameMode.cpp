#include "NetworkTestGameMode.h"

#include "../Common/LauncherPlayerController.h"
#include "NetworkTestPawn.h"
#include "PlayerController.h"
#include "World.h"

// 各ゲームモードクラスを ActorRegistry にゲームモードとして自動登録するマクロ
REGISTER_GAME_MODE(ANetworkTestGameMode)
REGISTER_GAME_MODE(ANetworkTestLevel2GameMode)

ANetworkTestGameMode::ANetworkTestGameMode() {
  // マルチプレイ検証用の Pawn と PlayerController を初期割り当てに設定
  SetDefaultPawnClass(ANetworkTestPawn::StaticClassName());
  SetDefaultPlayerControllerClass(ALauncherPlayerController::StaticClassName());
}

void ANetworkTestGameMode::BeginPlay() {
  // 一時停止状態などであっても、このアクターの Update 処理を常に実行するよう設定
  SetUpdateableAnytime(true);
}

APlayerController* ANetworkTestGameMode::OnClientConnected(FNetworkConnectionId ConnectionId) {
  // 基底クラス AGameModeBase に接続イベントを渡し、接続したクライアント用の PlayerController を生成
  APlayerController* controller = AGameModeBase::OnClientConnected(ConnectionId);
  if (GetWorld()) {
    // ローカル（サーバー実行側）のプレイヤーコントローラーを取得
    if (APlayerController* localPC = GetWorld()->GetOrCreateLocalPlayerController()) {
      // 操作中の Pawn を取得し、ステータスメッセージに接続通知をセット（画面表示用）
      if (ANetworkTestPawn* localPawn = dynamic_cast<ANetworkTestPawn*>(localPC->GetPawn())) {
        localPawn->SetStatusMessage("Client connected: " + std::to_string(ConnectionId));
      }
    }
  }
  return controller;
}

const char* ANetworkTestGameMode::GetSceneName() const { return "NetworkT1"; }

const char* ANetworkTestGameMode::GetTravelButtonText() const { return "Server Travel to Level 2"; }

FNetworkSceneId ANetworkTestGameMode::GetTravelTargetSceneId() const {
  return NetworkTestSceneIds::Level2;
}

const char* ANetworkTestGameMode::GetTravelTargetLevelPath() const {
  return NetworkTestLevelPaths::Level2;
}

const char* ANetworkTestLevel2GameMode::GetSceneName() const { return "Network2"; }

const char* ANetworkTestLevel2GameMode::GetTravelButtonText() const {
  return "Server Travel to Level 1";
}

FNetworkSceneId ANetworkTestLevel2GameMode::GetTravelTargetSceneId() const {
  return NetworkTestSceneIds::Level1;
}

const char* ANetworkTestLevel2GameMode::GetTravelTargetLevelPath() const {
  return NetworkTestLevelPaths::Level1;
}
