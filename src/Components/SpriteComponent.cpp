#include "SpriteComponent.h"
#include "Core/GameObject.h"
#include "Components/TransformComponent.h"
#include "Utils/UMath.h"
#include "Systems/RenderSystem.h"

MSpriteComponent::MSpriteComponent(int handle, int priority) : m_handle(handle), m_priority(priority)
{
}

void MSpriteComponent::Draw()
{
	auto transform = m_owner ? m_owner->GetComponent<MTransformComponent>() : nullptr;
	if (!transform) {
		return;
	}

	const auto& pos = transform->GetPos();

	RenderSystem::GetInstance().SubmitSprite(
		float(pos.x),
		float(pos.y),
		(double)transform->GetScale(),
      (double)UMath::DegToRad(transform->GetAngle()),
		m_handle,
		RenderSpace::World,
		m_priority);
}


void MSpriteComponent::OnMessage(const std::string& message)
{
	if (message == "HIT") {
		// 当たった時に何か演出をするなどの処理
	}
}
