#include "SampleScene01.h"

#include <PlayerController.h>
#include <SpriteComponent.h>

#include "HttpManager.h"
#include "Log.h"
#include "ActorManager.h"
#include "SamplePawn01.h"
#include "UMath.h"
#include "nlohmann/json.hpp"

ASampleScene01::ASampleScene01() {
  DefaultPawnClass = ASamplePawn01::StaticClassName();
  DefaultPlayerControllerClass = APlayerController::StaticClassName();
}

void ASampleScene01::BeginPlay() {}

void ASampleScene01::OnUpdate(float DeltaTime) { AGameModeBase::OnUpdate(DeltaTime); }
