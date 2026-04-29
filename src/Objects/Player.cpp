#include "Player.h"
#include "Systems/InputMapper.h"
#include "Systems/ResourceManager.h"
#include "Components/SpriteComponent.h"
Player::Player(float x, float y)
{
	m_transform->SetPos(x, y); // TransformComponent を使って位置を設定
	// 1. リソースマネージャーから画像ハンドルを取得
	int handle = ResourceManager::GetInstance().LoadResourceGraph("BaseFile/texture_Checker_64px.png");

	// 2. SpriteComponent を作成して追加
	auto sprite = std::make_unique<SpriteComponent>(handle, 0);
	AddComponent(std::move(sprite));
}
void Player::OnUpdate(float DeltaTime)
{
	if (InputMapper::GetInstance().GetKeyPressing(E_INPUT_ACTION::UP)) {
		m_transform->AddLocalPos(0.0f, -5.0f); 
	}
	if (InputMapper::GetInstance().GetKeyPressing(E_INPUT_ACTION::DOWN)) {
		m_transform->AddLocalPos(0.0f, 5.0f); 
	}
	if (InputMapper::GetInstance().GetKeyPressing(E_INPUT_ACTION::LEFT)) {
		m_transform->AddAngle(-5.0f); 
	}
	if (InputMapper::GetInstance().GetKeyPressing(E_INPUT_ACTION::RIGHT)) {
		m_transform->AddAngle(5.0f);
	}
}

void Player::OnDraw()
{
}
