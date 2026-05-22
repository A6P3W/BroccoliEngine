#include "SampleScene01.h"
#include "ObjectManager.h"
#include <Utils/Umath.h>
#include <Utils/Log.h>
#include <SpriteComponent.h>
#include <PlayerController.h>
#include "SamplePawn01.h"
ASampleScene01::ASampleScene01() {
	auto* Pawn = ObjectManager::GetInstance().SpawnObject<ASamplePawn01>({ 0,0 }, { 0 });
	auto* Controller = ObjectManager::GetInstance().SpawnObject<APlayerController>();
	Controller->Possess(Pawn);

}

void ASampleScene01::OnUpdate(float DeltaTime)
{
}
