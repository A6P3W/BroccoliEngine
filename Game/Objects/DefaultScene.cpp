#include "DefaultScene.h"
#include <string>
#include "ObjectManager.h"
#include "Objects/Player.h"
#include "Objects/SampleA.h"
#include "Objects/Map.h"
#include "ObjectManager.h"
#include <Utils/Umath.h>
#include <Utils/Log.h>
#include <SpriteComponent.h>
#include <CircleCollisionComponent.h>

ADefaultScene::ADefaultScene() {
	//ObjectManager::GetInstance().SpawnObject<AMap>(FVector2D::ZeroVector, 0.0f);
	ObjectManager::GetInstance().SpawnObject<APlayer>({ 0,0 }, { 0 });
	ObjectManager::GetInstance().SpawnObject<ASampleA>({ 0,0 }, { 0 });
	M_LOG("Default scene initialized", 0);
	auto sprite = std::make_unique<MSpriteComponent>(-10000, RenderSpace::World);
	sprite->SubmitCircle(300.0f, 0xFF0000, 1, 128);
	AddComponent(std::move(sprite));
	auto sprite2 = std::make_unique<MSpriteComponent>(-10000, RenderSpace::World);
	sprite2->SubmitText("center", 0xFF0000, -1, 255);
	AddComponent(std::move(sprite2));
	auto collision = std::make_unique<MCircleCollisionComponent>();
	AddComponent(std::move(collision));
}

void ADefaultScene::OnUpdate(float DeltaTime)
{
}
