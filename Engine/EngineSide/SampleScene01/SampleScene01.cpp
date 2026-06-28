#include "SampleScene01.h"
#include "ObjectManager.h"
#include "UMath.h"
#include "HttpManager.h"
#include "Log.h"
#include "nlohmann/json.hpp"
#include <SpriteComponent.h>
#include <PlayerController.h>
#include "SamplePawn01.h"

ASampleScene01::ASampleScene01()
{
	DefaultPawnClass = ASamplePawn01::StaticClassName();
	DefaultPlayerControllerClass = APlayerController::StaticClassName();
}

void ASampleScene01::BeginPlay()
{
}

void ASampleScene01::OnUpdate(float DeltaTime)
{
	AGameModeBase::OnUpdate(DeltaTime);
}
