#include "Player.h"
#include <Systems/InputMapper.h>
#include "Systems/ResourceManager.h"
#include "Components/SpriteComponent.h"
APlayer::APlayer(FVector2D location, FRotator rotation)
{
	m_transform->SetLocation(location);
	int handle = ResourceManager::GetInstance().LoadResourceGraph("BaseFile/texture_Checker_64px.png");
	auto sprite = std::make_unique<MSpriteComponent>(handle, 0);
	AddComponent(std::move(sprite));
}
void APlayer::OnUpdate(float DeltaTime)
{
	float moveSpeed = 1000.0f; 
	float rotationSpeed = 180.0f;
	if (InputMapper::GetInstance().GetKeyPressing(E_INPUT_ACTION::UP)) {
		m_transform->AddLocalLocation(0.0f, -moveSpeed * DeltaTime); 
	}
	if (InputMapper::GetInstance().GetKeyPressing(E_INPUT_ACTION::DOWN)) {
		m_transform->AddLocalLocation(0.0f, moveSpeed * DeltaTime); 
	}
	if (InputMapper::GetInstance().GetKeyPressing(E_INPUT_ACTION::LEFT)) {
		m_transform->AddRotation(-rotationSpeed * DeltaTime); 
	}
	if (InputMapper::GetInstance().GetKeyPressing(E_INPUT_ACTION::RIGHT)) {
		m_transform->AddRotation(rotationSpeed * DeltaTime);
	}
}

void APlayer::OnDraw()
{
}
