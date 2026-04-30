#include "Objects/CameraObject.h"
#include "Systems/RenderSystem.h"
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
		Vector2 target_pos = target_transform_component->GetPos();
		m_transform->SetPos(target_pos);
	}
	else {
		m_transform->SetPos(0, 0);
	}
	
}

void ACameraObject::SetCameraView()
{
	RenderSystem::GetInstance().SetCameraView(this);
}

void ACameraObject::TraceRotation()
{
}

