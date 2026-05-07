#include "Map.h"
#include "SpriteComponent.h"
#include "ResourceManager.h"
#include "RenderSystem.h"
#include <Utils/Umath.h>
AMap::AMap(FVector2D location, FRotator rotation)
{
	SetActorLocation(location);
	SetActorScale(5.0f);
	int handle = ResourceManager::GetInstance().LoadResourceGraph("Image/Asama_Test_Course.png");
	auto sprite = std::make_unique<MSpriteComponent>(-10000, RenderSpace::World);
	sprite->SubmitGraph(1.0, handle);
	AddComponent(std::move(sprite));

}
