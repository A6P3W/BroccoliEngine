#include "Objects/Camera.h"
#include <Systems/RenderSystem.h>
#include <vector>
#include <DxLib.h>

void ACameraObject::SetTarget(AGameObject* target)
{
	m_target = target;
}

void ACameraObject::OnUpdate(float DeltaTime)
{
	if (m_target) {
		auto target_transform_component = m_target->GetTransform();
		FVector2D target_pos = target_transform_component->GetLocation();
		m_transform->SetLocation(target_pos);
		m_transform->SetWorldRotation(target_transform_component->GetWorldRotation());
	}
	else {
		m_transform->SetLocation({0,0});
		m_transform->SetWorldRotation(0.0f);
	}

}

void ACameraObject::SetCameraView()
{
	RenderSystem::GetInstance().SetCameraView(this);
}

void ACameraObject::TraceRotation()
{
}

