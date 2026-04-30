#include "Player.h"
#include "Systems/InputMapper.h"
#include "Systems/ResourceManager.h"
#include "Components/SpriteComponent.h"
APlayer::APlayer(float x, float y)
{
	m_transform->SetPos(x, y); // MTransformComponent を使って位置を設定
	// 1. リソースマネージャーから画像ハンドルを取得
	int handle = ResourceManager::GetInstance().LoadResourceGraph("BaseFile/texture_Checker_64px.png");

	// 2. MSpriteComponent を作成して追加
	auto sprite = std::make_unique<MSpriteComponent>(handle, 0);
	AddComponent(std::move(sprite));
}
void APlayer::OnUpdate(float DeltaTime)
{
	float moveSpeed = 1000.0f; 
	float rotationSpeed = 180.0f;
	if (InputMapper::GetInstance().GetKeyPressing(E_INPUT_ACTION::UP)) {
		m_transform->AddLocalPos(0.0f, -moveSpeed * DeltaTime); 
	}
	if (InputMapper::GetInstance().GetKeyPressing(E_INPUT_ACTION::DOWN)) {
		m_transform->AddLocalPos(0.0f, moveSpeed * DeltaTime); 
	}
	if (InputMapper::GetInstance().GetKeyPressing(E_INPUT_ACTION::LEFT)) {
		m_transform->AddAngle(-rotationSpeed * DeltaTime); 
	}
	if (InputMapper::GetInstance().GetKeyPressing(E_INPUT_ACTION::RIGHT)) {
		m_transform->AddAngle(rotationSpeed * DeltaTime);
	}
}

void APlayer::OnDraw()
{
}
