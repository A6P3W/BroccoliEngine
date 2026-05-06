#include "SpriteComponent.h"
#include "Core/Public/GameObject.h"
#include "Utils/UMath.h"
#include "RenderSystem.h"

MSpriteComponent::MSpriteComponent(int handle, int priority) : m_handle(handle), m_priority(priority)
{
}

void MSpriteComponent::Draw()
{

	const FVector2D& WorldLocation = GetWorldLocation();
	const FRotator& WorldRotation = GetWorldRotation();
	RenderSystem::GetInstance().SubmitGraph(
		float(WorldLocation.X),
		float(WorldLocation.Y),
		(double)GetScale(),
		(double)UMath::DegToRad(WorldRotation.Rotation),
		m_handle,
		RenderSpace::World,
		m_priority);
	RenderSystem::GetInstance().SubmitCircle(
		float(WorldLocation.X),
		float(WorldLocation.Y),
		200.0f,
		(255, 255, 255),
		0,
		RenderSpace::World,
		m_priority + 1);
}


void MSpriteComponent::OnMessage(const std::string& message)
{
	if (message == "HIT") {
		// 当たった時に何か演出をするなどの処理
	}
}
