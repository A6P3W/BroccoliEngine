#include "Objects/SampleA.h"
#include "Dxlib.h"
#include "Systems/ResourceManager.h"
#include "Components/SpriteComponent.h" 
#include <Utils/Umath.h>

ASampleA::ASampleA(FVector2D location, FRotator rotation) : AGameObject()
{
	SetActorLocation(location);
	int handle = ResourceManager::GetInstance().LoadResourceGraph("BaseFile/texture_Checker_64px.png");
    auto sprite = std::make_unique<MSpriteComponent>(handle, 0);
	AddComponent(std::move(sprite));
}

ASampleA::~ASampleA()
{
}

void ASampleA::OnUpdate(float DeltaTime)
{
	float RotationSpeed = 45.0f; 
	float MoveSpeed = 500.0f;
	AddActorRotation(RotationSpeed * DeltaTime); // 毎フレーム回転させる
	AddActorLocalOffset({ MoveSpeed * DeltaTime, 0.0f }); // 毎フレーム右に移動させる
}

void ASampleA::OnDraw()
{
}