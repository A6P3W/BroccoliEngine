#include "SpriteComponent.h"
#include "OGameObject.h"
#include "TransformComponent.h"
#include "DxLib.h"

SpriteComponent::SpriteComponent(int handle) : m_handle(handle)
{
}

void SpriteComponent::Draw()
{
	auto transform = m_owner ? m_owner->GetComponent<TransformComponent>() : nullptr;
	if (!transform) {
		return;
	}

	const auto& pos = transform->GetPos();

 DrawRotaGraph((int)pos.x, (int)pos.y, (double)transform->GetScale(), (double)transform->GetAngle(), m_handle, TRUE);
}

void SpriteComponent::OnMessage(const std::string& message)
{
	if (message == "HIT") {
		// 当たった時に何か演出をするなどの処理
	}
}
