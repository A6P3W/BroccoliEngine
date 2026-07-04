#include "WidgetTestGameMode.h"

#include <PlayerController.h>
#include <SpriteComponent.h>

#include "HttpManager.h"
#include "Log.h"
#include "ActorManager.h"
#include "WidgetTestPawn.h"
#include "UMath.h"
#include "nlohmann/json.hpp"

REGISTER_GAME_MODE(AWidgetTestGameMode)
AWidgetTestGameMode::AWidgetTestGameMode() {
  DefaultPawnClass = AWidgetTestPawn::StaticClassName();
  DefaultPlayerControllerClass = APlayerController::StaticClassName();
}

void AWidgetTestGameMode::BeginPlay() {}

void AWidgetTestGameMode::OnUpdate(float DeltaTime) { AGameModeBase::OnUpdate(DeltaTime); }
