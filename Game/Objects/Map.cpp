#include "Map.h"
#include <Components/SpriteComponent.h>
#include <Systems/ResourceManager.h>
#include <Systems/RenderSystem.h>
#include <Utils/Umath.h>
AMap::AMap(FVector2D location, FRotator rotation)
{
	m_transform->SetLocation(location);
	m_transform->SetScale(5);
	int handle = ResourceManager::GetInstance().LoadResourceGraph("Image/Asama_Test_Course.png");
	auto sprite = std::make_unique<MSpriteComponent>(handle, -10000);
	AddComponent(std::move(sprite));
}

void AMap::OnDraw()
{
}

