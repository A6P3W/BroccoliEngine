#include "SampleScene01.h"
#include "ObjectManager.h"
#include <Utils/Umath.h>
#include <Utils/Log.h>
#include <SpriteComponent.h>
#include <PlayerController.h>
#include "SamplePawn01.h"
ASampleScene01::ASampleScene01() {
	SpawnPlayer<ASamplePawn01, APlayerController>({ 0,0 }, 0);

}

void ASampleScene01::OnUpdate(float DeltaTime)
{
}
