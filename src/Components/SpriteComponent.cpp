#include "SpriteComponent.h"
#include "Core/OGameObject.h"
#include "Components/TransformComponent.h"
#include "Systems/RenderSystem.h"

SpriteComponent::SpriteComponent(int handle, int priority) : m_handle(handle), m_priority(priority)
{
}

void SpriteComponent::Draw()
{
	auto transform = m_owner ? m_owner->GetComponent<TransformComponent>() : nullptr;
	if (!transform) {
		return;
	}

	const auto& pos = transform->GetPos();

	RenderSystem::GetInstance().SubmitSprite(
		(int)pos.x,
		(int)pos.y,
		(double)transform->GetScale(),
		(double)transform->GetAngle(),
		m_handle,
		RenderSpace::World,
		m_priority);
}


void SpriteComponent::OnMessage(const std::string& message)
{
	if (message == "HIT") {
		// 当たった時に何か演出をするなどの処理
	}
}
