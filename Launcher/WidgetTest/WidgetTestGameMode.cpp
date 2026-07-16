#include "WidgetTestGameMode.h"

#include <PlayerController.h>
#include <SpriteComponent.h>

#include "ActorManager.h"
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
  SetDefaultPlayerControllerClass(APlayerController::StaticClassName());
}

void AWidgetTestGameMode::BeginPlay() {
  AGameModeBase::BeginPlay();
  auto* mainMenuWidget = GetWorld()->GetActorManager()->SpawnObject<AWidgetTestUIMain>();

  UIManager::GetInstance()->AddWidget(mainMenuWidget);
  UIManager::GetInstance()->SetFocusedWidget(mainMenuWidget);

  M_LOG("WidgetTestGameMode: MUIVerticalBoxComponent sample is available in WidgetTestUIMain.");
}

void AWidgetTestGameMode::OnUpdate(float DeltaTime) { AGameModeBase::OnUpdate(DeltaTime); }
