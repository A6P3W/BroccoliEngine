#include "Map.h"
#include <Components/SpriteComponent.h>
#include <Systems/ResourceManager.h>
#include <Systems/RenderSystem.h>
#include <Utils/Umath.h>
AMap::AMap(FVector2D location, FRotator rotation)
{
	SetActorLocation(location);
	SetActorScale(5.0f);
	int handle = ResourceManager::GetInstance().LoadResourceGraph("Image/Asama_Test_Course.png");
	auto sprite = std::make_unique<MSpriteComponent>(handle, -10000);
	sprite->SetParentComponent(this->GetRootComponent());
	AddComponent(std::move(sprite));

}

void AMap::OnDraw()
{
}

