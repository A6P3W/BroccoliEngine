#include "Objects/OCameraObject.h"
#include "Systems/RenderSystem.h"
#include <vector>
#include <DxLib.h>

void OCameraObject::SetTarget(OGameObject* target)
{
	m_target = target;
}

void OCameraObject::OnUpdate(float DeltaTime)
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

void OCameraObject::SetCameraView()
{
	RenderSystem::GetInstance().SetCameraView(this);
}

void OCameraObject::TraceRotation()
{
}

