#include "Player.h"
#include "InputMapper.h"
#include "InputManager.h"
#include "ResourceManager.h"
#include "SpriteComponent.h"
#include "CameraComponent.h"
#include "SceneManager.h"
#include "Objects/DefaultScene.h"
#include <DxLib.h>
#include <Utils/Log.h>
#include <CircleCollisionComponent.h>
APlayer::APlayer(FVector2D location, FRotator rotation)
{
	SetActorLocation(location);
	SetActorRotation(rotation);


	int handle = ResourceManager::GetInstance().LoadResourceGraph("BaseFile/texture_Checker_64px.png");
	auto sprite = std::make_unique<MSpriteComponent>(0, RenderSpace::World);
	sprite->SubmitGraph(1.0, handle);
	AddComponent(std::move(sprite));



	auto camera = std::make_unique<MCameraComponent>();
	auto m_camera = camera.get();
	AddComponent(std::move(camera));
	m_camera->SetActiveCamera();


	auto collision = std::make_unique<MCircleCollisionComponent>();
	AddComponent(std::move(collision));
}
void APlayer::OnUpdate(float DeltaTime)
{
	float moveSpeed = 1000.0f;
	float rotationSpeed = 180.0f;
	if (InputMapper::GetInstance().GetKeyPressing(E_INPUT_ACTION::UP)) {
		AddActorLocalOffset({ 0.0f, -moveSpeed * DeltaTime });
	}
	if (InputMapper::GetInstance().GetKeyPressing(E_INPUT_ACTION::DOWN)) {
		AddActorLocalOffset({ 0.0f, moveSpeed * DeltaTime });
	}
	if (InputMapper::GetInstance().GetKeyPressing(E_INPUT_ACTION::LEFT)) {
		AddActorRotation(-rotationSpeed * DeltaTime);
	}
	if (InputMapper::GetInstance().GetKeyPressing(E_INPUT_ACTION::RIGHT)) {
		AddActorRotation(rotationSpeed * DeltaTime);
	}
	if (InputManager::GetInstance().GetKeyPressStart(KEY_INPUT_SPACE)) {
		int handle = ResourceManager::GetInstance().LoadResourceGraph("BaseFile/texture_Checker_64px.png");
		auto sprite = std::make_unique<MSpriteComponent>(0, RenderSpace::World);
		sprite->SubmitGraph(1.0, handle);
		sprite->SetParentComponent(this->GetRootComponent());
		sprite->AddWorldOffset({ 220.0f, -100.0f });
		sprite->SetScale(0.5f);
		AddComponent(std::move(sprite));
	}
	if (InputManager::GetInstance().GetKeyPressStart(KEY_INPUT_Z)) {
		SceneManager::GetInstance().OpenScene<ADefaultScene>();
	}
}

void APlayer::BeginOverlap(AActor* OtherActor)
{
	M_LOG("gg");
}
