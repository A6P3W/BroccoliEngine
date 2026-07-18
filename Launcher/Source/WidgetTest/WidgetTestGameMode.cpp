#include "WidgetTestGameMode.h"

#include <PlayerController.h>
#include <SpriteComponent.h>

#include "../Common/LauncherPlayerController.h"
#include "ActorManager.h"
#include "DxLibLap/DxLibLap.h"
#include "HttpManager.h"
#include "Log.h"
#include "UIManager.h"
#include "UMath.h"
#include "WidgetTestPawn.h"
#include "WidgetTestUIMain.h"
#include "nlohmann/json.hpp"

REGISTER_GAME_MODE(AWidgetTestGameMode)
AWidgetTestGameMode::AWidgetTestGameMode() {
  SetDefaultPawnClass(AWidgetTestPawn::StaticClassName());
  SetDefaultPlayerControllerClass(ALauncherPlayerController::StaticClassName());
}

void AWidgetTestGameMode::BeginPlay() {
  AGameModeBase::BeginPlay();

  constexpr int ScreenWidth = 800;
  constexpr int ScreenHeight = 600;
  const int ScreenHandle = DxLibLap::MakeScreen(ScreenWidth, ScreenHeight);
  if (ScreenHandle == -1) {
    M_LOG("WidgetTest MakeScreen failed: {}x{}.", ScreenWidth, ScreenHeight);
  } else {
    M_LOG(
        "WidgetTest MakeScreen succeeded: {}x{}, handle={}", ScreenWidth, ScreenHeight, ScreenHandle
    );
    DxLibLap::ReleaseScreen(ScreenHandle);
  }

  auto* mainMenuWidget = GetWorld()->GetActorManager()->SpawnObject<AWidgetTestUIMain>();

  UIManager::GetInstance()->AddWidget(mainMenuWidget);
  UIManager::GetInstance()->SetFocusedWidget(mainMenuWidget);

  M_LOG("WidgetTestGameMode: MUIVerticalBoxComponent sample is available in WidgetTestUIMain.");
}

void AWidgetTestGameMode::OnUpdate(float DeltaTime) { AGameModeBase::OnUpdate(DeltaTime); }
