#include "WidgetTestGameMode.h"

#include <PlayerController.h>
#include <SpriteComponent.h>

#include "../Common/LauncherPlayerController.h"
#include "ActorManager.h"
#include "CanvasTestActor.h"
#include "DxLibLap/DxLibLap.h"
#include "HttpManager.h"
#include "Log.h"
#include "UIManager.h"
#include "UMath.h"
#include "WidgetTestPawn.h"
#include "WidgetTestUIMain.h"
#include "nlohmann/json.hpp"

// このゲームモードクラスを ActorRegistry にゲームモードとして自動登録するマクロ
REGISTER_GAME_MODE(AWidgetTestGameMode)
AWidgetTestGameMode::AWidgetTestGameMode() {
  // デフォルトでスポーン・操作させる Pawn と PlayerController クラスを指定
  SetDefaultPawnClass(AWidgetTestPawn::StaticClassName());
  SetDefaultPlayerControllerClass(ALauncherPlayerController::StaticClassName());
}

void AWidgetTestGameMode::BeginPlay() {
  AGameModeBase::BeginPlay();

  constexpr int ScreenWidth = 800;
  constexpr int ScreenHeight = 600;
  // 低レベル描画ラッパー（DxLibLap）を使用して、オフスクリーンバッファを一時生成テスト
  const int ScreenHandle = DxLibLap::MakeScreen(ScreenWidth, ScreenHeight);
  if (ScreenHandle == -1) {
    M_LOG("WidgetTest MakeScreen failed: {}x{}.", ScreenWidth, ScreenHeight);
  } else {
    M_LOG(
        "WidgetTest MakeScreen succeeded: {}x{}, handle={}", ScreenWidth, ScreenHeight, ScreenHandle
    );
    // 生成した一時オフスクリーンバッファを解放
    DxLibLap::ReleaseScreen(ScreenHandle);
  }

  // アクターマネージャーを介して、UIウィジェットアクター（AWidgetTestUIMain）を動的に生成
  auto* mainMenuWidget = GetWorld()->GetActorManager()->SpawnObject<AWidgetTestUIMain>();

  // 作成した UI アクターを UI 描画システムに登録し、初期フォーカスを設定
  UIManager::GetInstance()->AddWidget(mainMenuWidget);
  UIManager::GetInstance()->SetFocusedWidget(mainMenuWidget);

  M_LOG("WidgetTestGameMode: MUIVerticalBoxComponent sample is available in WidgetTestUIMain.");

  // キャンバステストアクターを動的にスポーン
  GetWorld()->GetActorManager()->SpawnObject<ACanvasTestActor>();
  M_LOG("WidgetTestGameMode: CanvasTestActor spawned.");
}

void AWidgetTestGameMode::OnUpdate(float DeltaTime) { AGameModeBase::OnUpdate(DeltaTime); }
